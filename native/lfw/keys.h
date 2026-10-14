#pragma once

#include <string>
#include <utility>
#include <vector>

#include "lfw/controller/key_status.h"
#include "lfw/core/value.h"

namespace lfw {

class LFW;

// TS `src/LFW/Keys.ts`。
class Keys {
 public:
  static constexpr const char* TAG = "Keys";

  explicit Keys(LFW& lfw);

  // 切片 4CW（ck9）：`IKeysLfw` 退役 ⇒ 直接持真 `LFW*`（本头只前置声明，实现文件 include
  // `lfw.h` / `world.h`）；`time` 就是 TS 的 `lfw.world.lifetime`。
  LFW& lfw() const;
  double time() const;

  // `keys[GK.L]` 这类按下标读写；未知键 ⇒ nullptr（TS 的 undefined）。
  controller::KeyStatus* get(const std::u16string& key);
  // 字段定义顺序（L / R / U / D / a / j / d）。
  const std::vector<std::pair<std::u16string, controller::KeyStatus>>& list() const {
    return _keys;
  }

  void mount();
  void unmount();

 private:
  LFW* _lfw;
  std::vector<std::pair<std::u16string, controller::KeyStatus>> _keys;
};

}  // namespace lfw
