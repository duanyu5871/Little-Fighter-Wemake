#pragma once

#include "lfw/base/fsm.h"

namespace lfw {

class LFW;
class World;

namespace ui {

class UIComponent;
class UINode;

// TS `ui/component/ComponentFSMState.ts`：挂在组件上的 FSM 状态基类（`IState` 的 UI 版）。
// 构造时把 owner 的 `lfw/world/node` 缓存成只读字段（与 TS 一致）。
class ComponentFSMState : public IState {
 public:
  explicit ComponentFSMState(UIComponent& owner);

  UIComponent& owner;
  LFW& lfw;
  World& world;
  UINode& node;
};

}
}
