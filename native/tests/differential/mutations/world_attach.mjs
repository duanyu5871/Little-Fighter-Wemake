export default {
  subject: "world",
  cases: ["attach"],
  // 不可观察 / 有意不覆盖的项：
  //   - 视图层已随 4CE/4CF 退役（`EntityCollisionView` / `EntityStateView` / `EntityViewBase`
  //     整文件删除）：帧几何三缝现在就是 `Entity::frame_centery` / `frame_height` /
  //     `frame_pic_h`（`entity/entity.cpp`），`attach` 的转发只剩实体侧那句
  //     `world().add_entities(*this)` 与世界侧幽灵路由（`world.cpp` 的 `add_entities`）。
  //   - `attach` 的「ghost 标记取反」原锚在 `EntityViewBase::attach` 的转发行上，该行随视图层
  //     消失；同一语义（`_ghosted = truthy(ghost) ? 0 : 1`）已由 `entity.mjs` 的
  //     「attach：ghost 标记取反」锁定 ⇒ 本档删除该条，不重复。
  //   - 台面探针（`subjects/world.cpp` 的 `wgeom` / `wattach`）已改成直接在实体面上读
  //     `frame_centery()/frame_height()/frame_pic_h()` 与 `attach(Value(on))`；台面不在
  //     `mutate.mjs` 允许的变异目标里（只许 `native/lfw/**`）⇒ 锚点不落在探针上。
  //   - `Buff::place_effect_center` 的 `height != 0` 与 TS `height || pic?.h || 0` 只在
  //     `height` 为 `NaN` 时才有差，台面造不出 `NaN` 帧字段 ⇒ 保留现状（数值等价）。
  mutations: [
    { note: "centery 回退 0", file: "native/lfw/entity/entity.cpp",
      from: `  const Value v = field_or(frame, u"centery");\n  return entity_frame_missing(v) ? 0.0 : to_number(v);`,
      to: `  return 0.0;` },
    { note: "centery 缺省丢", file: "native/lfw/entity/entity.cpp",
      from: `  const Value v = field_or(frame, u"centery");\n  return entity_frame_missing(v) ? 0.0 : to_number(v);`,
      to: `  const Value v = field_or(frame, u"centery");\n  return to_number(v);` },
    { note: "centery 取 height", file: "native/lfw/entity/entity.cpp",
      from: `  const Value v = field_or(frame, u"centery");`,
      to: `  const Value v = field_or(frame, u"height");` },
    { note: "height 回退 0", file: "native/lfw/entity/entity.cpp",
      from: `  const Value v = field_or(frame, u"height");\n  return entity_frame_missing(v) ? 0.0 : to_number(v);`,
      to: `  return 0.0;` },
    { note: "height 缺省丢", file: "native/lfw/entity/entity.cpp",
      from: `  const Value v = field_or(frame, u"height");\n  return entity_frame_missing(v) ? 0.0 : to_number(v);`,
      to: `  const Value v = field_or(frame, u"height");\n  return to_number(v);` },
    { note: "pic_h 回退 0", file: "native/lfw/entity/entity.cpp",
      from: `  const Value v = field_or(field_or(frame, u"pic"), u"h");\n  return entity_frame_missing(v) ? 0.0 : to_number(v);`,
      to: `  return 0.0;` },
    { note: "pic_h 缺省丢", file: "native/lfw/entity/entity.cpp",
      from: `  const Value v = field_or(field_or(frame, u"pic"), u"h");\n  return entity_frame_missing(v) ? 0.0 : to_number(v);`,
      to: `  const Value v = field_or(field_or(frame, u"pic"), u"h");\n  return to_number(v);` },
    { note: "pic_h 取 w", file: "native/lfw/entity/entity.cpp",
      from: `  const Value v = field_or(field_or(frame, u"pic"), u"h");`,
      to: `  const Value v = field_or(field_or(frame, u"pic"), u"w");` },
    { note: "attach 不转发", file: "native/lfw/entity/entity.cpp",
      from: `  world().add_entities(*this);`,
      to: `  (void)0;` },
    { note: "attach ghost 丢", file: "native/lfw/world.cpp",
      from: `  if (truthy(Value(e.ghosted()))) {`,
      to: `  if (truthy(Value())) {` },
  ],
};
