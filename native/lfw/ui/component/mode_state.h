#pragma once

namespace lfw {
namespace ui {

// TS `ui/component/ModeState.ts`（字符串枚举）：模式（Stage/Vs/Demo）FSM 的状态键。
enum class ModeState { Invalid, Running, BeforeEnd, End, Restart };

inline const char16_t* mode_state_str(ModeState v) {
  switch (v) {
    case ModeState::Running:
      return u"Running";
    case ModeState::BeforeEnd:
      return u"BeforeEnd";
    case ModeState::End:
      return u"End";
    case ModeState::Restart:
      return u"Restart";
    case ModeState::Invalid:
    default:
      return u"";
  }
}

}
}
