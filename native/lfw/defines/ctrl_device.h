#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class CtrlDevice : int {
  Keyboard = 0,
  Gamepad_1 = 1,
  Gamepad_2 = 2,
  Gamepad_3 = 3,
  Gamepad_4 = 4,
  TouchScreen = 5,
};

inline const std::vector<EnumNumberEntry>& ctrl_device_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Keyboard", static_cast<double>(CtrlDevice::Keyboard)},
    {u"Gamepad_1", static_cast<double>(CtrlDevice::Gamepad_1)},
    {u"Gamepad_2", static_cast<double>(CtrlDevice::Gamepad_2)},
    {u"Gamepad_3", static_cast<double>(CtrlDevice::Gamepad_3)},
    {u"Gamepad_4", static_cast<double>(CtrlDevice::Gamepad_4)},
    {u"TouchScreen", static_cast<double>(CtrlDevice::TouchScreen)},
  };
  return e;
}

inline const char16_t* ctrl_device_name_of(int v) {
  switch (v) {
    case static_cast<int>(CtrlDevice::Keyboard): return u"Keyboard";
    case static_cast<int>(CtrlDevice::Gamepad_1): return u"Gamepad_1";
    case static_cast<int>(CtrlDevice::Gamepad_2): return u"Gamepad_2";
    case static_cast<int>(CtrlDevice::Gamepad_3): return u"Gamepad_3";
    case static_cast<int>(CtrlDevice::Gamepad_4): return u"Gamepad_4";
    case static_cast<int>(CtrlDevice::TouchScreen): return u"TouchScreen";
  }
  return nullptr;
}

}
