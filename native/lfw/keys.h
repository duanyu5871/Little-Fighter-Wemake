#pragma once

#include <string>
#include <utility>
#include <vector>

#include "lfw/controller/key_status.h"
#include "lfw/core/value.h"

namespace lfw {

class Keys;

// TS `Keys` 依赖的 `lfw` 面（`LFW` 未移植 ⇒ 宿主缝）。
class IKeysLfw {
 public:
  virtual ~IKeysLfw() = default;
  // `lfw.world.lifetime`
  virtual double lifetime() = 0;
  // `lfw.regist_keys(this)` / `lfw.recycle_keys(this)`
  virtual void regist_keys(Keys& keys) = 0;
  virtual void recycle_keys(Keys& keys) = 0;
};

// TS `src/LFW/Keys.ts`。
class Keys {
 public:
  static constexpr const char* TAG = "Keys";

  explicit Keys(IKeysLfw& lfw);

  IKeysLfw& lfw() const { return *_lfw; }
  double time() const { return _lfw->lifetime(); }

  // `keys[GK.L]` 这类按下标读写；未知键 ⇒ nullptr（TS 的 undefined）。
  controller::KeyStatus* get(const std::u16string& key);
  // 字段定义顺序（L / R / U / D / a / j / d）。
  const std::vector<std::pair<std::u16string, controller::KeyStatus>>& list() const {
    return _keys;
  }

  void mount();
  void unmount();

 private:
  IKeysLfw* _lfw;
  std::vector<std::pair<std::u16string, controller::KeyStatus>> _keys;
};

}  // namespace lfw
