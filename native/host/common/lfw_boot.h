// 宿主无关的「装配 + 摘要」：把原来在 `host/lfw_run.cpp`（CLI 宿主）与
// `host/wasm/lfw_wasm.cpp`（WASM 宿主）里各写一遍的那段台面逻辑收成一份。
//
// 这一份只做**与宿主无关**的事：选舞台/背景 → 建 `players` 个 bot 战士（vs 或闯关口径）
// → `change_stage`；外加一帧的可复现摘要。IO、日志文本和"建不起来要不要停手"都留在宿主侧
// ——由 `on_spawn` / `on_failure` 把数据递回去。
//
// ⚠ 不许依赖 emscripten（没有 JS、没有 `wasm` 的纯 C++ 宿主也要能直接调）。
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace lfw {
class LFW;
class World;
}  // namespace lfw

namespace lfw::host {

// 一个战士出场的全部事实。日志的格式各宿主自己定（`lfw_run` 打坐标+队，wasm 只打 id），
// 所以这里只递数据、不打字。
struct SpawnInfo {
  std::u16string player_id;
  std::u16string fighter_id;
  bool vs = false;
  double x = 0;
  double y = 0;
  double z = 0;
  std::u16string team;
};

// 没建起来的战士。
struct BootFailure {
  std::u16string fighter_id;
  bool create_failed = false;  // false = 数据包里没有这个角色
};

struct BootOptions {
  std::vector<std::u16string> fighters;  // 显式角色 id；空 = 取数据包里的前 `players` 个
  std::u16string stage;                  // 指定 stage id；空 = 用数据里 `is_starting` 的那条
  int players = 2;
  bool vs = false;  // true = TS `GamePrepareLogic.start_game` 的对战模式分支
  // 可选：每个建成的战士（在 `attach()` 之后调，位置与原来打日志的地方一致）。
  std::function<void(const SpawnInfo&)> on_spawn;
  // 可选：建不起来时。返回 false = 立刻停手（wasm 的"有错就中止"口径）；返回 true 或不装 =
  // 记账后继续（`lfw_run` 的口径）。
  std::function<bool(const BootFailure&)> on_failure;
};

struct BootResult {
  // false = 数据包里没有可用角色 —— 此时除了 `change_bg` 什么都没做（也没建任何东西）。
  bool ok = false;
  std::u16string stage_id;  // 实际生效的 stage id（可能由数据回填）
  std::u16string bg_id;
};

// 装好一场对局：选 stage/bg → 造战士 → `change_stage`（+ vs 的 `dataset` 设置）。
BootResult boot_scene(lfw::LFW& lfw, const BootOptions& options);

// 一帧的可复现摘要：实体表（按 id 排序）的 id / 位置 / 血量 / 蓝 / 队 / 状态（FNV-1a）。
std::string frame_digest(const lfw::World& world, int frame);

// 实体表当前的血量总和（摘要行用）。
double total_hp(const lfw::World& world);

}  // namespace lfw::host
