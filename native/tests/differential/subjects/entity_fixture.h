#pragma once

// 切片 4BY 台面：`buff` 系统以 `lfw::Entity` 为准入（`buff::IBuffEntity` 已退役）⇒
// 台面假实体不再实现窄接口，而是**真 `Entity`**（或其派生）。这里放两件共享品：
//   * `fx::Host`：最小 `IEntityHost`（`new_id` 脚本化，其余走基类默认）；
//   * `fx::StubEntity`：只把 `dataset` 转给回调的最小 `Entity` 派生 —— `buff` 侧对
//     宿主唯一的读面就是 `dataset`，够用的台面直接用它。
// 需要逐字复刻 TS 侧日志的台面（`buff` 家族）各自派生 `lfw::Entity` 并覆写实体面。

#include <functional>
#include <string>
#include <utility>

#include "lfw/buff/buff.h"
#include "lfw/entity/entity.h"

namespace fx {

// 最小宿主：`new_id` 由 `make_id` 脚本化（缺省给空串），其余全部走 `IEntityHost` 默认。
class Host : public lfw::IEntityHost {
 public:
  Host() = default;
  explicit Host(std::function<std::u16string()> make_id) : _make_id(std::move(make_id)) {}

  std::u16string new_id() override { return _make_id ? _make_id() : std::u16string(); }
  std::u16string new_team() const override { return u"1"; }
  double game_time() const override { return 0.0; }
  void add_entities(lfw::Entity& e) override {
    if (on_add_entities) on_add_entities(e);
  }

  std::function<void(lfw::Entity&)> on_add_entities;

 private:
  std::function<std::u16string()> _make_id;
};

// 进程内共享的最简宿主（Meyers 单例：静态初始化顺序安全）。
//
// 名字带 `shared_` 前缀是刻意的：`lfw::Entity` 自己有一个成员 `host()`（`IEntityHost&
// host() const { return *host_; }`），在 `Entity` 派生类的构造初始化列表里写裸 `host()`
// 会解析到那个成员（在基类构造完成前读 `host_`）——踩过一次，别再踩。
inline lfw::IEntityHost& shared_host() {
  static Host h;
  return h;
}

// 一个真 `Entity`：构造后把 `id` 钉成调用方给的值（`Entity::reset` 会用宿主的
// `new_id` 覆盖它），`dataset` 转给回调（缺省空值）。
class StubEntity : public lfw::Entity {
 public:
  explicit StubEntity(std::u16string id, lfw::Value data = lfw::Value())
      : lfw::Entity(shared_host(), std::move(data)) {
    this->id = std::move(id);
  }

  std::function<lfw::Value(const std::u16string&)> on_dataset;

  lfw::Value dataset(const std::u16string& key) const override;
};

inline lfw::Value StubEntity::dataset(const std::u16string& key) const {
  return on_dataset ? on_dataset(key) : lfw::Value();
}

}  // namespace fx
