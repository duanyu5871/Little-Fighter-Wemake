# src/LFW/bot/state

## 规则

BotState（BotState_Base 的子类）不得持有**可变实例字段**（缓存、计数、边界值等）。
运行时状态一律由 BotController 持有，BotState 通过 `this.ctrl` / `this.c` / `this.me` 访问。

允许：

- `readonly key` 等恒定的标识/配置字段（FSM 需要）
- 方法内的局部变量

## 原因

- BotState 实例随控制器长期存活（`fsm` 只构造一次），state 上的字段会跨实体、跨生命周期残留。
- 控制器会被池化复用，`BotController.reset()` 的契约是"与新构造等价"；控制器不持有的状态无法复位。
- 方便做快照

## 配套要求

- 任何迁到 BotController 的字段，必须同时写进它的 `reset()`。
- 新增 BotState 字段前，先问：能否作为 BotController 的字段持有并复位？
