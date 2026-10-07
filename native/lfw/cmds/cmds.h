#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

class World;

namespace cmds {

class CMDS;

// TS `ICMDHandler`：`(ctx: CMDS) => unknown`（返回值没人用 ⇒ 端口给 `void`）。
using CmdHandler = void (*)(CMDS&);

// `'-'` 开头的词是不是命名参数（TS 的 `part.startsWith('--')` / `token.startsWith('--')`）。
inline bool starts_with_dash(const std::u16string& s) {
  return s.size() >= 2 && s[0] == u'-' && s[1] == u'-';
}

// Mirrors `src/LFW/cmds/CMDS.ts`：命令调度器（词法解析 + 静态注册表）。
//
// 形状差异：
//   * TS 在模块加载期用 `CMDS.register(...)` 登记 32 条命令；端口没有加载期 ⇒ 首次用到
//     注册表时一次性登记**已移植**的命令（表在 `cmds.cpp`，次序照 `index.ts`）。未移植的
//     命令（SPAWN / SET_PUPPET / cheat / KEY_EVENT / POINTER_EVENTS / F4 / F8）查不到
//     handler（`handler()` 给 `nullptr`）——TS 那边它们是登记着的，差分台面不碰这些词。
//   * `handle` 里的 `this.inst.words[0].toLowerCase()`：命令全是空白时 TS 会 `TypeError`
//     （`words[0]` 是 `undefined`）——端口无异常，直接跳过这一步。
//   * `args` 是 JS 对象字面量：取不存在的键给 `undefined`；`constructor` 这种撞原型的键
//     TS 会给一个函数（端口给 `nullopt`）——数据里不可达，不建形（记在 DESIGN）。
class CMDS {
 public:
  // TS `static handler(key)`：查表（键统一 `toLowerCase`）。
  static CmdHandler handler(const std::u16string& key);
  // TS `static register(key, help, handler)`。
  static void register_cmd(const std::u16string& key, const std::u16string& help, CmdHandler fn);
  // TS `static handle(world, cmds)`：`inst` 按 `world` 缓存（同 world 复用）。
  static void handle(World& world, const std::vector<std::u16string>& cmds);

  World& world() const { return *world_; }
  const std::u16string& cmd() const { return cmd_; }
  const std::vector<std::u16string>& words() const { return words_; }
  const std::vector<std::u16string>& positionals() const { return positionals_; }

  void set_cmd(const std::u16string& cmd);
  // TS `str(index)`：越界给 `undefined`。
  std::optional<std::u16string> str(size_t index) const;
  // TS `num(index)`：`Number(str)`（空串是 0、非数是 NaN）。
  std::optional<double> num(size_t index) const;
  // TS `nums(index)`：`str.split(',').map(Number)`（**保留空段**：`"1,,2"` → `[1,0,2]`）。
  std::optional<std::vector<double>> nums(size_t index) const;
  std::optional<std::u16string> str_arg(const std::u16string& name) const;
  std::optional<double> num_arg(const std::u16string& name) const;
  std::optional<std::vector<double>> nums_arg(const std::u16string& name) const;

 private:
  explicit CMDS(World& world);

  const std::map<std::u16string, std::u16string>& args() const;
  std::optional<std::u16string> arg_value(const std::u16string& name) const;

  World* world_;
  std::u16string cmd_;
  std::vector<std::u16string> words_;
  std::vector<std::u16string> positionals_;
  mutable std::optional<std::map<std::u16string, std::u16string>> args_;
};

}
}
