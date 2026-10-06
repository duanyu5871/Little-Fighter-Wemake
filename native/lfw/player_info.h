#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/core/value.h"

namespace lfw {

class Entity;

// TS `ICacheData` 里 `PlayerInfo::load` 会读的字段（`id` / `create_date` / `type` 没读）。
// 「字节」在端口里是 `std::vector<uint8_t>`（TS 是 `Uint8Array` / `ArrayBuffer`）。
struct PlayerInfoCacheEntry {
  // `await Ditto.Cache.get(name)` 回 `undefined`。
  bool missing = false;
  // `Ditto.Cache.get` 自己抛（宿主缝的失败面）。
  bool get_threw = false;

  // TS `data?: Uint8Array | null`：假值（`undefined` / `null`）⇒ `kFalsy`；字节 ⇒ `kBytes`；
  // 别的真值 ⇒ `kOther`（`decodeUTF8` 会抛）。
  enum class DataKind { kFalsy, kBytes, kOther };
  DataKind data_kind = DataKind::kFalsy;
  std::vector<uint8_t> data;
  Value data_other;

  // TS `blob?: IBlob | null`：假值 ⇒ `kFalsy`；`await blob.arrayBuffer()` 给字节 ⇒ `kBytes`；
  // 那次调用抛（含 `blob.arrayBuffer` 不是函数）⇒ `kThrows`。
  enum class BlobKind { kFalsy, kBytes, kThrows };
  BlobKind blob_kind = BlobKind::kFalsy;
  std::vector<uint8_t> blob;
};

// TS `Ditto.Cache.put({ name, type, version, data })` 的参数。
struct PlayerInfoCachePut {
  std::u16string name;
  std::u16string type;
  double version = 0;
  std::vector<uint8_t> data;
};

// `Ditto` 的宿主面（TS 里是全局单例 `Ditto.Cache` / `Ditto.warn`）。
class IPlayerInfoHost {
 public:
  virtual ~IPlayerInfoHost() = default;
  // `await Ditto.Cache.get(name)`。
  virtual void cache_get(const std::u16string& name, PlayerInfoCacheEntry& out) = 0;
  // `await Ditto.Cache.del(name)`；抛 ⇒ `error` 非空。
  virtual bool cache_del(const std::u16string& name, std::u16string& error) = 0;
  // `Ditto.Cache.put(...)`：TS 侧没 `await` ⇒ 端口也不给失败面。
  virtual void cache_put(const PlayerInfoCachePut& data) = 0;
  // `Ditto.warn(...)`：只保留**第一个**参数（TS 其余参数是日志上下文）。
  virtual void warn(const std::u16string& text) = 0;
};

// TS `PlayerInfo`。`load()` 在 TS 里是 `async`（构造函数里 `this.loaded = this.load()`），
// 端口把它收成同步 `bool`：
//   * 构造函数**不**立刻跑，只挂起一次「构造函数那次 load」；
//   * 第一次 `loaded()` 或 `load()` 时先把它落地，再算本次调用。
// 这样「构造之后立刻注册的监听者也能收到 load 期间的回调」与 TS 的 async 时序一致（TS 里那次
// load 真正生效在第一个 `await` 之后）。
class PlayerInfo {
 public:
  static constexpr const char16_t* kTag = u"PlayerInfo";
  static constexpr const char16_t* kDataType = u"PlayerInfo";
  static constexpr double kDataVersion = 1;

  // TS：`constructor(id, name = id, local = true, mine = true)`。`name` / `local` / `mine` 都是
  // `Value` —— 好把「缺省（`undefined`）走默认值」与「显式 `null` / `""` / `false` 照存」分开。
  PlayerInfo(IPlayerInfoHost* host, const std::u16string& id, const Value& name, const Value& local,
             const Value& mine);

  Callbacks callbacks;

  const Value& local() const { return _local; }
  const Value& mine() const { return _mine; }
  // 对应 TS 的 `await this.loaded`：会先把构造函数那次挂起的 `load()` 落地。
  bool loaded();

  // TS 的 `_info` 是 `{ id, name, keys, version, ctrl }`（插入序就是字面量顺序）。
  const Value& info() const { return _info; }
  std::u16string id() const;
  std::u16string storage_key() const;
  Value name() const;
  Value keys() const;
  bool is_com() const { return _is_com; }
  Value ctrl() const;
  Entity* fighter() const { return _fighter; }

  void set_name(const Value& name, bool emit);
  void set_ctrl(const Value& ctrl, bool emit);
  void set_is_com(bool is_com, bool emit);
  // TS 在严格模式下可能抛（`keys` 是 nullish 时读、`keys` 不是对象时写、`key` 不是字符串时
  // `key.toLowerCase()`）⇒ 端口给失败通道。
  bool set_key(const Value& name, const Value& key, bool emit, std::u16string& error);
  bool get_key(const Value& name, Value& out, std::u16string& error) const;
  void set_fighter(Entity* v) { _fighter = v; }

  bool load();
  void save();

 private:
  Value field(const char16_t* key) const;
  void set_field(const char16_t* key, const Value& v);
  void warn(const std::u16string& text);
  bool load_impl();
  void flush_pending_load();

  IPlayerInfoHost* _host = nullptr;
  Value _info;
  Value _local;
  Value _mine;
  bool _loaded = false;
  bool _load_pending = false;
  bool _is_com = false;
  Entity* _fighter = nullptr;
};

}
