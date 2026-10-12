export default {
  subject: "world",
  cases: ["attach"],
  // 不可观察 / 有意不覆盖的项：
  //   - 帧几何与 `attach` 已统一到 `EntityViewBase`（切片 4BX）：`world` 用例经
  //     `EntityCollisionView` 的继承实现走到同一份代码，变异锚点落在 `EntityViewBase` 上
  //     （原 `EntityStateView` 侧“无调用者、无变异”的说明随视图面合一作废）。
  //   - `Buff::place_effect_center` 的 `height != 0` 与 TS `height || pic?.h || 0` 只在
  //     `height` 为 `NaN` 时才有差，台面造不出 `NaN` 帧字段 ⇒ 保留现状（数值等价）。
  mutations: [
    { note: "centery 回退 0", file: "native/lfw/entity/entity_view_base.cpp",
      from: `  const Value v = field_or(_e.frame, u"centery");\n  return is_missing(v) ? 0.0 : to_number(v);`,
      to: `  return 0.0;` },
    { note: "centery 缺省丢", file: "native/lfw/entity/entity_view_base.cpp",
      from: `  const Value v = field_or(_e.frame, u"centery");\n  return is_missing(v) ? 0.0 : to_number(v);`,
      to: `  const Value v = field_or(_e.frame, u"centery");\n  return to_number(v);` },
    { note: "centery 取 height", file: "native/lfw/entity/entity_view_base.cpp",
      from: `  const Value v = field_or(_e.frame, u"centery");`,
      to: `  const Value v = field_or(_e.frame, u"height");` },
    { note: "height 回退 0", file: "native/lfw/entity/entity_view_base.cpp",
      from: `  const Value v = field_or(_e.frame, u"height");\n  return is_missing(v) ? 0.0 : to_number(v);`,
      to: `  return 0.0;` },
    { note: "height 缺省丢", file: "native/lfw/entity/entity_view_base.cpp",
      from: `  const Value v = field_or(_e.frame, u"height");\n  return is_missing(v) ? 0.0 : to_number(v);`,
      to: `  const Value v = field_or(_e.frame, u"height");\n  return to_number(v);` },
    { note: "pic_h 回退 0", file: "native/lfw/entity/entity_view_base.cpp",
      from: `  const Value v = field_or(field_or(_e.frame, u"pic"), u"h");\n  return is_missing(v) ? 0.0 : to_number(v);`,
      to: `  return 0.0;` },
    { note: "pic_h 缺省丢", file: "native/lfw/entity/entity_view_base.cpp",
      from: `  const Value v = field_or(field_or(_e.frame, u"pic"), u"h");\n  return is_missing(v) ? 0.0 : to_number(v);`,
      to: `  const Value v = field_or(field_or(_e.frame, u"pic"), u"h");\n  return to_number(v);` },
    { note: "pic_h 取 w", file: "native/lfw/entity/entity_view_base.cpp",
      from: `  const Value v = field_or(field_or(_e.frame, u"pic"), u"h");`,
      to: `  const Value v = field_or(field_or(_e.frame, u"pic"), u"w");` },
    { note: "attach 不转发", file: "native/lfw/entity/entity_view_base.cpp",
      from: `void EntityViewBase::attach(bool on) { _e.attach(Value(on)); }`,
      to: `void EntityViewBase::attach(bool on) { (void)on; }` },
    { note: "attach ghost 反", file: "native/lfw/entity/entity_view_base.cpp",
      from: `void EntityViewBase::attach(bool on) { _e.attach(Value(on)); }`,
      to: `void EntityViewBase::attach(bool on) { _e.attach(Value(!on)); }` },
    { note: "attach ghost 丢", file: "native/lfw/entity/entity_view_base.cpp",
      from: `void EntityViewBase::attach(bool on) { _e.attach(Value(on)); }`,
      to: `void EntityViewBase::attach(bool on) { _e.attach(Value()); }` },
  ],
};
