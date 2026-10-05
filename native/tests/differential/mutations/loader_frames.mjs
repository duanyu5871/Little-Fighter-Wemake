// `loader/preprocess_bdy` + `loader/preprocess_itr`（外加 `preprocess_action` 的 `tester`）。
//
// 用例：`cases/loader_frames/all.txt`（220 行）。`__tester` / `action.tester` 的值两端不可比
// （TS 存编译对象、端口存源串），差分里两边都剥掉，只比较“键在不在、值真不真”的探针 ——
// 所以探针本身也是本名单的一部分目标（见 `preprocess_action` 那几条）。
//
// 有意不覆盖（等价或不可观察）：
//   * `ctx` 不是对象时的 `return false`（用例总是给对象 ctx，TS 侧也表达不出来）；
//   * `bdy` / `itr` 是**数组**时的行为（TS 能往数组上挂 `__tester`，端口的 `Array` 没有键位）：
//     `itr` 没有分支会命中所以两边一致，`bdy` 则端口按失败处理 —— 这类数据没意义，用例里不写；
//   * `set_default` 的 `is_nullish` → `!truthy` 这一类：对 `motionless` / `shaking` / `dvx` 这些
//     默认值本身就是 `0` 的键，两种判法结果相同（只有 `vrest ??= 1` 能区分，已单独列）；
//   * CondMaker **每组首项**的 `.add` ↔ `.and_` / `.or_` ↔ `.and_` / `wrap` ↔ `add`：
//     `bdy` 的两组首项、`itr` 的 MFire2 / Pick 首项、Whirlwind 外层 `wrap`——它们生成的串
//     在“空 maker / 空组”上完全相同，属于按构造等价，不列（组内**第二项起**的这些改写都列了）。
export default {
  subject: "loader_frames",
  cases: ["all"],
  mutations: [
    // ---------------------------------------------------------------- preprocess_bdy：取值
    {
      note: "bdy: ctx.bdy 读成 ctx.bdy_",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  Value bdy = field_or(ctx, u"bdy");`,
      to: `  Value bdy = field_or(ctx, u"bdy_");`,
    },
    {
      note: "bdy: ctx.data 读成 ctx.data_",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  const Value data = field_or(ctx, u"data");`,
      to: `  const Value data = field_or(ctx, u"data_");`,
    },
    {
      note: "bdy: ctx.frame 读成 ctx.data",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  const Value frame = field_or(ctx, u"frame");`,
      to: `  const Value frame = field_or(ctx, u"data");`,
    },
    {
      note: "bdy: prefab 表用 itr_prefabs",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  const ResolvePrefabResult merged = resolve_prefab(bdy, field_or(data, u"bdy_prefabs"));`,
      to: `  const ResolvePrefabResult merged = resolve_prefab(bdy, field_or(data, u"itr_prefabs"));`,
    },
    {
      note: "bdy: 错误 tag 写成 preprocess_itr",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    error = prefab_error_message(u"preprocess_bdy", to_string(field_or(data, u"id")), u"bdy", merged);`,
      to: `    error = prefab_error_message(u"preprocess_itr", to_string(field_or(data, u"id")), u"bdy", merged);`,
    },
    {
      note: "bdy: 错误里的 what 写成 itr",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    error = prefab_error_message(u"preprocess_bdy", to_string(field_or(data, u"id")), u"bdy", merged);`,
      to: `    error = prefab_error_message(u"preprocess_bdy", to_string(field_or(data, u"id")), u"itr", merged);`,
    },
    {
      note: "bdy: 错误里的 id 取成常量",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    error = prefab_error_message(u"preprocess_bdy", to_string(field_or(data, u"id")), u"bdy", merged);`,
      to: `    error = prefab_error_message(u"preprocess_bdy", to_string(Value(u"?")), u"bdy", merged);`,
    },
    {
      note: "bdy: bdy 不是对象时不抛（直接成功）",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  Object* const b = as_mut(bdy);
  if (b == nullptr) return false;`,
      to: `  Object* const b = as_mut(bdy);
  if (b == nullptr) return true;`,
    },
    {
      note: "bdy: kind 读成 kind_name",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  const Value kind = field_or(bdy, u"kind");`,
      to: `  const Value kind = field_or(bdy, u"kind_name");`,
    },

    // ---------------------------------------------------------------- preprocess_bdy：Caught + hit_flag
    {
      note: "bdy: kind===Normal 改成宽松比较",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  if (is_num_eq(kind, static_cast<double>(BdyKind::Normal)) &&`,
      to: `  if (equals(kind, Value(static_cast<double>(BdyKind::Normal))) &&`,
    },
    {
      note: "bdy: kind===Normal 写成 Criminal",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  if (is_num_eq(kind, static_cast<double>(BdyKind::Normal)) &&`,
      to: `  if (is_num_eq(kind, static_cast<double>(BdyKind::Criminal)) &&`,
    },
    {
      note: "bdy: 条件的第一项改成 ||",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  if (is_num_eq(kind, static_cast<double>(BdyKind::Normal)) &&`,
      to: `  if (is_num_eq(kind, static_cast<double>(BdyKind::Normal)) ||`,
    },
    {
      note: "bdy: frame.state===Caught 改成宽松比较",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `      is_num_eq(state_of(frame), static_cast<double>(StateEnum::Caught)) &&`,
      to: `      equals(state_of(frame), Value(static_cast<double>(StateEnum::Caught))) &&`,
    },
    {
      note: "bdy: frame.state===Caught 写成 Falling",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `      is_num_eq(state_of(frame), static_cast<double>(StateEnum::Caught)) &&`,
      to: `      is_num_eq(state_of(frame), static_cast<double>(StateEnum::Falling)) &&`,
    },
    {
      note: "bdy: 条件的第二项改成 ||",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `      is_num_eq(state_of(frame), static_cast<double>(StateEnum::Caught)) &&`,
      to: `      is_num_eq(state_of(frame), static_cast<double>(StateEnum::Caught)) ||`,
    },
    {
      note: "bdy: hit_flag 判空改成“取反真值”",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `      is_nullish(field_or(bdy, u"hit_flag"))) {`,
      to: `      !truthy(field_or(bdy, u"hit_flag"))) {`,
    },
    {
      note: "bdy: caught 时给 AllEnemy",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    dat_translator::set_hit_flag(*b, n(static_cast<double>(HitFlag::AllBoth)));`,
      to: `    dat_translator::set_hit_flag(*b, n(static_cast<double>(HitFlag::AllEnemy)));`,
    },

    // ---------------------------------------------------------------- preprocess_bdy：老式 goto
    {
      note: "bdy: goto 下界用 >",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  if (ge(kind, n(kGotoMin)) && le(kind, n(kGotoMax))) {`,
      to: `  if (gt(kind, n(kGotoMin)) && le(kind, n(kGotoMax))) {`,
    },
    {
      note: "bdy: goto 上界用 <",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  if (ge(kind, n(kGotoMin)) && le(kind, n(kGotoMax))) {`,
      to: `  if (ge(kind, n(kGotoMin)) && lt(kind, n(kGotoMax))) {`,
    },
    {
      note: "bdy: 两个边界改成 ||",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  if (ge(kind, n(kGotoMin)) && le(kind, n(kGotoMax))) {`,
      to: `  if (ge(kind, n(kGotoMin)) || le(kind, n(kGotoMax))) {`,
    },
    {
      note: "bdy: kGotoMin 写成 999",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `constexpr double kGotoMin = 1000;`,
      to: `constexpr double kGotoMin = 999;`,
    },
    {
      note: "bdy: kGotoMax 写成 2000",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `constexpr double kGotoMax = 1999;`,
      to: `constexpr double kGotoMax = 2000;`,
    },
    {
      note: "bdy: goto 的 kind 写成 Defend",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    dat_translator::set_bdy_kind(*b, n(static_cast<double>(BdyKind::Criminal)));`,
      to: `    dat_translator::set_bdy_kind(*b, n(static_cast<double>(BdyKind::Defend)));`,
    },
    {
      note: "bdy: goto 的第一组用 != 0",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `      c.add(s(collision_val::kSameTeam), u"==", n(0));
      c.and_(s(collision_val::kAttackerType), u"==", n(static_cast<double>(EntityEnum::Fighter)));`,
      to: `      c.add(s(collision_val::kSameTeam), u"!=", n(0));
      c.and_(s(collision_val::kAttackerType), u"==", n(static_cast<double>(EntityEnum::Fighter)));`,
    },
    {
      note: "bdy: goto 的第一组 AttackerType 用 Weapon",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `      c.and_(s(collision_val::kAttackerType), u"==", n(static_cast<double>(EntityEnum::Fighter)));
      c.and_(s(collision_val::kItrKind), u"==", n(static_cast<double>(ItrKind::Normal)));`,
      to: `      c.and_(s(collision_val::kAttackerType), u"==", n(static_cast<double>(EntityEnum::Weapon)));
      c.and_(s(collision_val::kItrKind), u"==", n(static_cast<double>(ItrKind::Normal)));`,
    },
    {
      note: "bdy: 第二组（weapon 分支）的字段换成 VictimType",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `      c.and_(s(collision_val::kAttackerType), u"==", n(static_cast<double>(EntityEnum::Weapon)));`,
      to: `      c.and_(s(collision_val::kVictimType), u"==", n(static_cast<double>(EntityEnum::Weapon)));`,
    },
    {
      note: "bdy: 第二组的 EntityEnum 用 Fighter",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `      c.and_(s(collision_val::kAttackerType), u"==", n(static_cast<double>(EntityEnum::Weapon)));`,
      to: `      c.and_(s(collision_val::kAttackerType), u"==", n(static_cast<double>(EntityEnum::Fighter)));`,
    },
    {
      note: "bdy: 第二组的 ItrKind 用 Catch",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `      c.and_(s(collision_val::kItrKind), u"==", n(static_cast<double>(ItrKind::Normal)));
      c.and_([&](CondMaker& c2) {
        c2.or_(s(collision_val::kAttackerState), u"==",`,
      to: `      c.and_(s(collision_val::kItrKind), u"==", n(static_cast<double>(ItrKind::Catch)));
      c.and_([&](CondMaker& c2) {
        c2.or_(s(collision_val::kAttackerState), u"==",`,
    },
    {
      note: "bdy: 内层 AttackerState 用 BrokenDefend",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `               n(static_cast<double>(StateEnum::Weapon_OnHand)));`,
      to: `               n(static_cast<double>(StateEnum::BrokenDefend)));`,
    },
    {
      note: "bdy: 内层 HenryArrow1 写成 HenryArrow2",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `        c2.or_(s(collision_val::kAttackerOID), u"==", s(oid::kHenryArrow1));`,
      to: `        c2.or_(s(collision_val::kAttackerOID), u"==", s(oid::kHenryArrow2));`,
    },
    {
      note: "bdy: 内层 RudolfWeapon 写成 Rudolf",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `        c2.or_(s(collision_val::kAttackerOID), u"==", s(oid::kRudolfWeapon));`,
      to: `        c2.or_(s(collision_val::kAttackerOID), u"==", s(oid::kRudolf));`,
    },
    {
      note: "bdy: 第二组（or）改成 and",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    cm.or_([&](CondMaker& c) {
      c.add(s(collision_val::kSameTeam), u"==", n(0));`,
      to: `    cm.and_([&](CondMaker& c) {
      c.add(s(collision_val::kSameTeam), u"==", n(0));`,
    },
    {
      note: "bdy: test 不写进 bdy（丢掉整条）",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    b->set(u"test", Value(cm.done()));`,
      to: `    b->set(u"test", Value(u""));`,
    },
    {
      note: "bdy: 生成的 test 用旧值（不覆盖已有 test）",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    b->set(u"test", Value(cm.done()));`,
      to: `    if (!truthy(field_or(bdy, u"test"))) b->set(u"test", Value(cm.done()));`,
    },
    {
      note: "bdy: goto 的 id 用 kind + 1000",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `                  {u"data", make_obj({{u"id", Value(to_string(Value(to_number(kind) - 1000)))}})}}),`,
      to: `                  {u"data", make_obj({{u"id", Value(to_string(Value(to_number(kind) + 1000)))}})}}),`,
    },
    {
      note: "bdy: goto 的 id 用 kind - 1001",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `                  {u"data", make_obj({{u"id", Value(to_string(Value(to_number(kind) - 1000)))}})}}),`,
      to: `                  {u"data", make_obj({{u"id", Value(to_string(Value(to_number(kind) - 1001)))}})}}),`,
    },
    {
      note: "bdy: goto 的 id 不做数字转换（直接 to_string(kind)）",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `                  {u"data", make_obj({{u"id", Value(to_string(Value(to_number(kind) - 1000)))}})}}),`,
      to: `                  {u"data", make_obj({{u"id", Value(to_string(kind))}})}}),`,
    },
    {
      note: "bdy: 第一个动作类型用 V_TURN_TEAM",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `        make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},`,
      to: `        make_obj({{u"type", s(action_type::kV_TURN_TEAM)},`,
    },
    {
      note: "bdy: 第二个动作类型用 V_NEXT_FRAME",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `        make_obj({{u"type", s(action_type::kV_TURN_TEAM)}, {u"data", make_obj({{u"team", s(u"")}})}})};`,
      to: `        make_obj({{u"type", s(action_type::kV_NEXT_FRAME)}, {u"data", make_obj({{u"team", s(u"")}})}})};`,
    },
    {
      note: "bdy: 第二个动作的 team 写成 \"1\"",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `        make_obj({{u"type", s(action_type::kV_TURN_TEAM)}, {u"data", make_obj({{u"team", s(u"")}})}})};`,
      to: `        make_obj({{u"type", s(action_type::kV_TURN_TEAM)}, {u"data", make_obj({{u"team", s(u"1")}})}})};`,
    },
    {
      note: "bdy: 两个动作顺序互换",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `        make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},
                  {u"data", make_obj({{u"id", Value(to_string(Value(to_number(kind) - 1000)))}})}}),
        make_obj({{u"type", s(action_type::kV_TURN_TEAM)}, {u"data", make_obj({{u"team", s(u"")}})}})};`,
      to: `        make_obj({{u"type", s(action_type::kV_TURN_TEAM)}, {u"data", make_obj({{u"team", s(u"")}})}}),
        make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},
                  {u"data", make_obj({{u"id", Value(to_string(Value(to_number(kind) - 1000)))}})}})};`,
    },
    {
      note: "bdy: ensure 的 non-array 护栏去掉",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    if (truthy(acts) && as_array(acts) == nullptr) return false;`,
      to: `    (void)0;`,
    },
    {
      note: "bdy: ensure 丢掉已有的 actions（不是追加）",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    b->set(u"actions", ensure(acts, items));`,
      to: `    Value fresh;
    b->set(u"actions", ensure(fresh, items));`,
    },

    // ---------------------------------------------------------------- preprocess_bdy：__tester 与 actions
    {
      note: "bdy: __tester 永远存 undefined",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  b->set(u"__tester", truthy(test) ? test : Value());`,
      to: `  b->set(u"__tester", Value());`,
    },
    {
      note: "bdy: __tester 的三目写反",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  b->set(u"__tester", truthy(test) ? test : Value());`,
      to: `  b->set(u"__tester", truthy(test) ? Value() : test);`,
    },
    {
      note: "bdy: __tester 的键名写成 tester",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  b->set(u"__tester", truthy(test) ? test : Value());`,
      to: `  b->set(u"tester", truthy(test) ? test : Value());`,
    },
    {
      note: "bdy: test 读成 test_",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  const Value test = field_or(bdy, u"test");`,
      to: `  const Value test = field_or(bdy, u"test_");`,
    },
    {
      note: "bdy: actions 为真值才遍历的判据取反",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  if (truthy(actions)) {
    Array* const a = as_array(actions);`,
      to: `  if (!truthy(actions)) {
    Array* const a = as_array(actions);`,
    },
    {
      note: "bdy: actions 不是数组时直接成功",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    Array* const a = as_array(actions);
    if (a == nullptr) return false;`,
      to: `    Array* const a = as_array(actions);
    if (a == nullptr) return true;`,
    },
    {
      note: "bdy: 遍历 actions 从第二项开始",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `    for (size_t i = 0; i < n; ++i) {
      Value item = a->at(i);
      if (!preprocess_action(item)) return false;
    }`,
      to: `    for (size_t i = 1; i < n; ++i) {
      Value item = a->at(i);
      if (!preprocess_action(item)) return false;
    }`,
    },
    {
      note: "bdy: preprocess_action 的失败被忽略",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `      if (!preprocess_action(item)) return false;`,
      to: `      preprocess_action(item);`,
    },
    {
      note: "bdy: 结果不回写 ctx.bdy",
      file: "native/lfw/loader/preprocess_bdy.cpp",
      from: `  ctx_o->set(u"bdy", bdy);`,
      to: `  ctx_o->set(u"bdy", field_or(ctx, u"bdy"));`,
    },

    // ---------------------------------------------------------------- preprocess_itr：取值与 prefab
    {
      note: "itr: ctx.itr 读成 ctx.bdy",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  Value itr = field_or(ctx, u"itr");`,
      to: `  Value itr = field_or(ctx, u"bdy");`,
    },
    {
      note: "itr: 丢掉 nullish 检查（直接往下走）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  // TS 的 \`resolve_prefab\` 一开始就读 \`obj.ref\`：\`null\` / \`undefined\` 时会抛 TypeError。
  if (is_nullish(itr)) return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "itr: prefab 表用 bdy_prefabs",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  const ResolvePrefabResult merged = resolve_prefab(itr, field_or(data, u"itr_prefabs"));`,
      to: `  const ResolvePrefabResult merged = resolve_prefab(itr, field_or(data, u"bdy_prefabs"));`,
    },
    {
      note: "itr: 错误 tag 写成 preprocess_bdy",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    error = prefab_error_message(u"preprocess_itr", to_string(field_or(data, u"id")), u"itr", merged);`,
      to: `    error = prefab_error_message(u"preprocess_bdy", to_string(field_or(data, u"id")), u"itr", merged);`,
    },
    {
      note: "itr: 错误里的 what 写成 bdy",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    error = prefab_error_message(u"preprocess_itr", to_string(field_or(data, u"id")), u"itr", merged);`,
      to: `    error = prefab_error_message(u"preprocess_itr", to_string(field_or(data, u"id")), u"bdy", merged);`,
    },
    {
      note: "itr: 标量 itr 不再回写",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    ctx_o->set(u"itr", itr);
    return true;`,
      to: `    ctx_o->set(u"itr", Value());
    return true;`,
    },

    // ---------------------------------------------------------------- preprocess_itr：catchingact / caughtact
    {
      note: "itr: catchingact 读成 caughtact",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  const Value catchingact = field_or(itr, u"catchingact");`,
      to: `  const Value catchingact = field_or(itr, u"caughtact");`,
    },
    {
      note: "itr: catchingact 为真值才处理的判据取反",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  if (truthy(catchingact)) {`,
      to: `  if (!truthy(catchingact)) {`,
    },
    {
      note: "itr: catchingact 的失败被忽略",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    Value local = catchingact;
    if (!preprocess_next_frame(local)) return false;`,
      to: `    Value local = catchingact;
    preprocess_next_frame(local);`,
    },
    {
      note: "itr: caughtact 的失败被忽略",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    Value local = caughtact;
    if (!preprocess_next_frame(local)) return false;`,
      to: `    Value local = caughtact;
    preprocess_next_frame(local);`,
    },

    // ---------------------------------------------------------------- preprocess_itr：共享助手
    {
      note: "itr: set_default 的判据改成“取反真值”（vrest ??= 1 可见）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    if (is_nullish(field_or(itr, key))) it->set(std::u16string(key), Value(v));`,
      to: `    if (!truthy(field_or(itr, key))) it->set(std::u16string(key), Value(v));`,
    },
    {
      note: "itr: set_default 不写回",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    if (is_nullish(field_or(itr, key))) it->set(std::u16string(key), Value(v));`,
      to: `    (void)key;
    (void)v;`,
    },
    {
      note: "itr: test 判空改成“取反真值”",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  const auto test_is_nullish = [&]() { return is_nullish(field_or(itr, u"test")); };`,
      to: `  const auto test_is_nullish = [&]() { return !truthy(field_or(itr, u"test")); };`,
    },
    {
      note: "itr: test ??= 每次都覆盖（不管有没有 test）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  const auto test_is_nullish = [&]() { return is_nullish(field_or(itr, u"test")); };`,
      to: `  const auto test_is_nullish = [&]() { return true; };`,
    },
    {
      note: "itr: test 从不覆盖",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  const auto test_is_nullish = [&]() { return is_nullish(field_or(itr, u"test")); };`,
      to: `  const auto test_is_nullish = [&]() { return false; };`,
    },
    {
      note: "itr: hit_flag 一律写 AllBoth（忽略已有的）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    dat_translator::set_hit_flag(*it,
                                 is_nullish(hf) ? n(static_cast<double>(HitFlag::AllBoth)) : hf);`,
      to: `    dat_translator::set_hit_flag(*it, n(static_cast<double>(HitFlag::AllBoth)));`,
    },
    {
      note: "itr: hit_flag 一律写原值（缺省时不补 AllBoth）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    dat_translator::set_hit_flag(*it,
                                 is_nullish(hf) ? n(static_cast<double>(HitFlag::AllBoth)) : hf);`,
      to: `    dat_translator::set_hit_flag(*it, hf);`,
    },
    {
      note: "itr: hit_flag 默认值换成 AllEnemy",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `                                 is_nullish(hf) ? n(static_cast<double>(HitFlag::AllBoth)) : hf);`,
      to: `                                 is_nullish(hf) ? n(static_cast<double>(HitFlag::AllEnemy)) : hf);`,
    },
    {
      note: "itr: vrest→arest 的判据改成“非空”",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    const Value vrest = field_or(itr, u"vrest");
    if (truthy(vrest)) {`,
      to: `    const Value vrest = field_or(itr, u"vrest");
    if (!is_nullish(vrest)) {`,
    },
    {
      note: "itr: vrest→arest 时不删 vrest",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    if (truthy(vrest)) {
      it->set(u"arest", vrest);
      it->remove(u"vrest");
    }`,
      to: `    if (truthy(vrest)) {
      it->set(u"arest", vrest);
    }`,
    },
    {
      note: "itr: vrest→arest 写成 arest = 1",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      it->set(u"arest", vrest);`,
      to: `      it->set(u"arest", n(1));`,
    },
    {
      note: "itr: 尾部的 __tester 无条件写",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  if (truthy(test)) it->set(u"__tester", test);`,
      to: `  it->set(u"__tester", test);`,
    },
    {
      note: "itr: 尾部的 __tester 永远不写",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  if (truthy(test)) it->set(u"__tester", test);`,
      to: `  if (false) it->set(u"__tester", test);`,
    },
    {
      note: "itr: 尾部的 test 读成 kind",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  const Value test = field_or(itr, u"test");
  if (truthy(test)) it->set(u"__tester", test);`,
      to: `  const Value test = field_or(itr, u"kind");
  if (truthy(test)) it->set(u"__tester", test);`,
    },
    {
      note: "itr: actions 不是数组时直接成功",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    Array* const a = as_array(actions);
    if (a == nullptr) return false;`,
      to: `    Array* const a = as_array(actions);
    if (a == nullptr) return true;`,
    },
    {
      note: "itr: preprocess_action 的失败被忽略",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      if (!preprocess_action(item)) return false;`,
      to: `      preprocess_action(item);`,
    },
    {
      note: "itr: 结果不回写 ctx.itr",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  if (truthy(test)) it->set(u"__tester", test);

  ctx_o->set(u"itr", itr);`,
      to: `  if (truthy(test)) it->set(u"__tester", test);

  ctx_o->set(u"itr", field_or(ctx, u"itr"));`,
    },

    // ---------------------------------------------------------------- preprocess_itr：kind 分派
    {
      note: "itr: kind 的严格比较改成宽松（Catch 分派）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  if (is_num_eq(kind, ItrKind::Catch)) {`,
      to: `  if (equals(kind, num(ItrKind::Catch))) {`,
    },
    {
      note: "itr: Catch 分派写成 Pick",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  if (is_num_eq(kind, ItrKind::Catch)) {`,
      to: `  if (is_num_eq(kind, ItrKind::Pick)) {`,
    },
    {
      note: "itr: kind 读成 effect",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  const Value kind = field_or(itr, u"kind");
  if (is_num_eq(kind, ItrKind::Catch)) {`,
      to: `  const Value kind = field_or(itr, u"effect");
  if (is_num_eq(kind, ItrKind::Catch)) {`,
    },
    {
      note: "itr: ForceCatch 分派写成 Catch",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::ForceCatch)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::Catch)) {`,
    },
    {
      note: "itr: Normal 分派写成 Catch",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::Normal)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::Catch)) {`,
    },
    {
      note: "itr: Pick 分派写成 PickSecretly",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::Pick)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::PickSecretly)) {`,
    },
    {
      note: "itr: PickSecretly 分派写成 Pick",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::PickSecretly)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::Pick)) {`,
    },
    {
      note: "itr: SuperPunchMe 分派写成 Pick",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::SuperPunchMe)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::Pick)) {`,
    },
    {
      note: "itr: MagicFlute 只认 MagicFlute（丢掉 MagicFlute2）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::MagicFlute) || is_num_eq(kind, ItrKind::MagicFlute2)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::MagicFlute) || is_num_eq(kind, ItrKind::MagicFlute)) {`,
    },
    {
      note: "itr: MagicFlute2 写成 Block",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::MagicFlute) || is_num_eq(kind, ItrKind::MagicFlute2)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::MagicFlute) || is_num_eq(kind, ItrKind::Block)) {`,
    },
    {
      note: "itr: Block 分派写成 JohnShield",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::Block)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::JohnShield)) {`,
    },
    {
      note: "itr: JohnShield 分派写成 Block",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::JohnShield)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::Block)) {`,
    },
    {
      note: "itr: Heal 分派写成 Freeze",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::Heal)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::Freeze)) {`,
    },
    {
      note: "itr: Freeze 分派写成 Heal",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::Freeze)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::Heal)) {`,
    },
    {
      note: "itr: Whirlwind 分派写成 Freeze",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::Whirlwind)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::Freeze)) {`,
    },
    {
      note: "itr: CharacterThrew 分派写成 ForceCatch",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::CharacterThrew)) {`,
      to: `  } else if (is_num_eq(kind, ItrKind::ForceCatch)) {`,
    },

    // ---------------------------------------------------------------- preprocess_itr：Normal × effect
    {
      note: "itr: effect 读成 kind",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    const Value effect = field_or(itr, u"effect");`,
      to: `    const Value effect = field_or(itr, u"kind");`,
    },
    {
      note: "itr: Fire 分支写成 MFire1",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    if (is_num_eq(effect, ItrEffect::Fire)) {`,
      to: `    if (is_num_eq(effect, ItrEffect::MFire1)) {`,
    },
    {
      note: "itr: MFire1 分支写成 MFire2",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    } else if (is_num_eq(effect, ItrEffect::MFire1)) {`,
      to: `    } else if (is_num_eq(effect, ItrEffect::MFire2)) {`,
    },
    {
      note: "itr: MFire2 分支写成 Through",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    } else if (is_num_eq(effect, ItrEffect::MFire2)) {`,
      to: `    } else if (is_num_eq(effect, ItrEffect::Through)) {`,
    },
    {
      note: "itr: Through 分支写成 Ice2",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    } else if (is_num_eq(effect, ItrEffect::Through)) {`,
      to: `    } else if (is_num_eq(effect, ItrEffect::Ice2)) {`,
    },
    {
      note: "itr: Ice2 分支写成 Fire",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    } else if (is_num_eq(effect, ItrEffect::Ice2)) {`,
      to: `    } else if (is_num_eq(effect, ItrEffect::Fire)) {`,
    },
    {
      note: "itr: Fire 的 VictimState 用 Frozen",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `        cm.add(s(collision_val::kVictimState), u"!=", num(StateEnum::Burning));
        cm.or_(s(collision_val::kAttackerState), u"!=", num(StateEnum::BurnRun));`,
      to: `        cm.add(s(collision_val::kVictimState), u"!=", num(StateEnum::Frozen));
        cm.or_(s(collision_val::kAttackerState), u"!=", num(StateEnum::BurnRun));`,
    },
    {
      note: "itr: Fire 的 or_ 改成 and_",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `        cm.or_(s(collision_val::kAttackerState), u"!=", num(StateEnum::BurnRun));`,
      to: `        cm.and_(s(collision_val::kAttackerState), u"!=", num(StateEnum::BurnRun));`,
    },
    {
      note: "itr: MFire1 的 VictimType 用 Weapon",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `        cm.and_(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
        cm.and_(s(collision_val::kVictimState), u"!=", num(StateEnum::BurnRun));`,
      to: `        cm.and_(s(collision_val::kVictimType), u"==", num(EntityEnum::Weapon));
        cm.and_(s(collision_val::kVictimState), u"!=", num(StateEnum::BurnRun));`,
    },
    {
      note: "itr: Through 用 == Fighter",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `        cm.add(s(collision_val::kVictimType), u"!=", num(EntityEnum::Fighter));`,
      to: `        cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));`,
    },
    {
      note: "itr: Ice2 的第二个词用 VictimState",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `        cm.and_(s(collision_val::kVictimFrameId), u"!=",
                s(collision_val::kVictimFrameIndex_ICE));`,
      to: `        cm.and_(s(collision_val::kVictimState), u"!=",
                s(collision_val::kVictimFrameIndex_ICE));`,
    },

    // ---------------------------------------------------------------- preprocess_itr：Catch / ForceCatch
    {
      note: "itr: Catch 的 VictimState 用 Frozen",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::Tired));`,
      to: `      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::Frozen));`,
    },
    {
      note: "itr: Catch 的 AClosingSpeedX 用 >=",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.and_(s(collision_val::kAClosingSpeedX), u">", n(0));`,
      to: `      cm.and_(s(collision_val::kAClosingSpeedX), u">=", n(0));`,
    },
    {
      note: "itr: Catch 的 VictimType 用 Weapon",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::Tired));`,
      to: `      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Weapon));
      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::Tired));`,
    },
    {
      note: "itr: ForceCatch 的 Falling 判成 ==（原为 !=）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.and_(s(collision_val::kVictimState), u"!=", num(StateEnum::Falling));`,
      to: `      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::Falling));`,
    },
    {
      note: "itr: ForceCatch 少一个 and_（丢掉 VictimType）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.and_(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
      cm.and_(s(collision_val::kVictimState), u"!=", num(StateEnum::Falling));`,
      to: `      cm.and_(s(collision_val::kVictimState), u"!=", num(StateEnum::Falling));`,
    },
    {
      note: "itr: Catch 少一个 set_default（不写 motionless）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    set_default(u"motionless", 0);
    set_default(u"shaking", 0);
    vrest_to_arest();
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));`,
      to: `    set_default(u"shaking", 0);
    vrest_to_arest();
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));`,
    },
    {
      note: "itr: Catch 不调 vrest_to_arest",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    set_default(u"motionless", 0);
    set_default(u"shaking", 0);
    vrest_to_arest();
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));`,
      to: `    set_default(u"motionless", 0);
    set_default(u"shaking", 0);
    (void)0;
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));`,
    },

    // ---------------------------------------------------------------- preprocess_itr：Pick / PickSecretly
    {
      note: "itr: Pick 的 and_one_of 只留一个状态",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `                    {num(StateEnum::Weapon_OnGround), num(StateEnum::HeavyWeapon_OnGround)});`,
      to: `                    {num(StateEnum::Weapon_OnGround)});`,
    },
    {
      note: "itr: Pick 用的字段换成 VictimState",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.add(s(collision_val::kAttackerHasHolding), u"==", n(0));`,
      to: `      cm.add(s(collision_val::kVictimState), u"==", n(0));`,
    },
    {
      note: "itr: Pick 的轻/重武器判据互换",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    light.and_(s(collision_val::kVictimBaseType), u"!=", num(WeaponEnum::Heavy));
    CondMaker heavy;
    heavy.and_(s(collision_val::kVictimBaseType), u"==", num(WeaponEnum::Heavy));`,
      to: `    light.and_(s(collision_val::kVictimBaseType), u"==", num(WeaponEnum::Heavy));
    CondMaker heavy;
    heavy.and_(s(collision_val::kVictimBaseType), u"!=", num(WeaponEnum::Heavy));`,
    },
    {
      note: "itr: Pick 的轻武器帧 id 写成 116",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `                  {u"data", make_obj({{u"id", s(u"115")}})}}),`,
      to: `                  {u"data", make_obj({{u"id", s(u"116")}})}}),`,
    },
    {
      note: "itr: Pick 的重武器帧 id 写成 116",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `                  {u"data", make_obj({{u"id", s(u"117")}})}})};`,
      to: `                  {u"data", make_obj({{u"id", s(u"116")}})}})};`,
    },
    {
      note: "itr: Pick 的 pretest 写成 false（第一个动作）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `                  {u"desc", s(u"picking_light 捡起轻型武器")},
                  {u"pretest", Value(true)},`,
      to: `                  {u"desc", s(u"picking_light 捡起轻型武器")},
                  {u"pretest", Value(false)},`,
    },
    {
      note: "itr: Pick 的第一个动作 desc 写成 picking_heavy",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    const std::vector<Value> items = {
        make_obj({{u"type", s(action_type::kA_NEXT_FRAME)},
                  {u"desc", s(u"picking_light 捡起轻型武器")},`,
      to: `    const std::vector<Value> items = {
        make_obj({{u"type", s(action_type::kA_NEXT_FRAME)},
                  {u"desc", s(u"picking_heavy 捡起重型武器")},`,
    },
    {
      note: "itr: Pick 的 ensure non-array 护栏去掉",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    if (truthy(acts) && as_array(acts) == nullptr) return false;
    it->set(u"actions", ensure(acts, items));
  } else if (is_num_eq(kind, ItrKind::PickSecretly)) {`,
      to: `    it->set(u"actions", ensure(acts, items));
  } else if (is_num_eq(kind, ItrKind::PickSecretly)) {`,
    },
    {
      note: "itr: PickSecretly 的 AHitAttack 用 == 1 改成 == 0",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.and_(s(collision_val::kAHitAttack), u"==", n(1));`,
      to: `      cm.and_(s(collision_val::kAHitAttack), u"==", n(0));`,
    },
    {
      note: "itr: PickSecretly 的 Weapon_OnGround 用 HeavyWeapon_OnGround",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::Weapon_OnGround));
      cm.and_(s(collision_val::kAHitAttack), u"==", n(1));`,
      to: `      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::HeavyWeapon_OnGround));
      cm.and_(s(collision_val::kAHitAttack), u"==", n(1));`,
    },
    {
      note: "itr: PickSecretly 的 VictimHasHolder 写成 AttackerHasHolder",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.and_(s(collision_val::kVictimHasHolder), u"==", n(0));
      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::Weapon_OnGround));`,
      to: `      cm.and_(s(collision_val::kAttackerHasHolder), u"==", n(0));
      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::Weapon_OnGround));`,
    },

    // ---------------------------------------------------------------- preprocess_itr：其余分支
    {
      note: "itr: SuperPunchMe 的 VictimType 用 Weapon",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::MagicFlute) || is_num_eq(kind, ItrKind::MagicFlute2)) {`,
      to: `      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Weapon));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::MagicFlute) || is_num_eq(kind, ItrKind::MagicFlute2)) {`,
    },
    {
      note: "itr: SuperPunchMe / MagicFlute 不写 motionless",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `  } else if (is_num_eq(kind, ItrKind::SuperPunchMe)) {
    set_default(u"motionless", 0);
    set_default(u"shaking", 0);`,
      to: `  } else if (is_num_eq(kind, ItrKind::SuperPunchMe)) {
    set_default(u"shaking", 0);`,
    },
    {
      note: "itr: MagicFlute 的内层用 or_（原为 and_）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.or_([&](CondMaker& c) {
        c.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Weapon));
        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kHenryArrow1));`,
      to: `      cm.or_([&](CondMaker& c) {
        c.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Weapon));
        c.or_(s(collision_val::kVictimOID), u"!=", s(oid::kHenryArrow1));`,
    },
    {
      note: "itr: MagicFlute 的 RudolfWeapon 写成 Rudolf",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kRudolfWeapon));`,
      to: `        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kRudolf));`,
    },
    {
      note: "itr: MagicFlute 的 VictimType 改成 AttackerType",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.or_([&](CondMaker& c) {
        c.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Weapon));
        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kHenryArrow1));`,
      to: `      cm.or_([&](CondMaker& c) {
        c.add(s(collision_val::kAttackerType), u"==", num(EntityEnum::Weapon));
        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kHenryArrow1));`,
    },
    {
      note: "itr: Block 的 BdyKind 用 Criminal",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.add(s(collision_val::kBdyKind), u"==", n(static_cast<double>(BdyKind::Normal)));`,
      to: `      cm.add(s(collision_val::kBdyKind), u"==", n(static_cast<double>(BdyKind::Criminal)));`,
    },
    {
      note: "itr: JohnShield 的 SameTeam 用 0",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.or_(s(collision_val::kSameTeam), u"!=", n(1));`,
      to: `      cm.or_(s(collision_val::kSameTeam), u"!=", n(0));`,
    },
    {
      note: "itr: JohnShield 的 or_ 改成 and_",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.or_(s(collision_val::kSameTeam), u"!=", n(1));`,
      to: `      cm.and_(s(collision_val::kSameTeam), u"!=", n(1));`,
    },
    {
      note: "itr: Heal 的 dvx 判据取反",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    const Value dvx = field_or(itr, u"dvx");
    if (truthy(dvx)) {`,
      to: `    const Value dvx = field_or(itr, u"dvx");
    if (!truthy(dvx)) {`,
    },
    {
      note: "itr: Heal 的 dvx 读成 dvy",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    const Value dvx = field_or(itr, u"dvx");`,
      to: `    const Value dvx = field_or(itr, u"dvy");`,
    },
    {
      note: "itr: Heal 的动作类型用 V_NEXT_FRAME",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `          make_obj({{u"type", s(action_type::kA_NEXT_FRAME)},
                    {u"data", dat_translator::get_next_frame_by_raw_id(dvx, u"frame")}})};`,
      to: `          make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},
                    {u"data", dat_translator::get_next_frame_by_raw_id(dvx, u"frame")}})};`,
    },
    {
      note: "itr: Heal 的 get_next_frame_by_raw_id 用 repeat",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `                    {u"data", dat_translator::get_next_frame_by_raw_id(dvx, u"frame")}})};`,
      to: `                    {u"data", dat_translator::get_next_frame_by_raw_id(dvx, u"repeat")}})};`,
    },
    {
      note: "itr: Heal 的 ensure non-array 护栏去掉",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      if (truthy(acts) && as_array(acts) == nullptr) return false;
      const std::vector<Value> items = {`,
      to: `      const std::vector<Value> items = {`,
    },
    {
      note: "itr: Heal 的 VictimType 用 Weapon",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.and_(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::Freeze)) {`,
      to: `      cm.and_(s(collision_val::kVictimType), u"==", num(EntityEnum::Weapon));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::Freeze)) {`,
    },
    {
      note: "itr: Freeze 不写 dvz（少一个 set_default）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    set_default(u"dvx", 0);
    set_default(u"dvy", 0);
    set_default(u"dvz", 0);
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));`,
      to: `    set_default(u"dvx", 0);
    set_default(u"dvy", 0);
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));`,
    },
    {
      note: "itr: Freeze 的 Frozen 判成 Burning",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `        c.or_(s(collision_val::kVictimState), u"==", num(StateEnum::Frozen));
        return &c;
      });
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::Whirlwind)) {`,
      to: `        c.or_(s(collision_val::kVictimState), u"==", num(StateEnum::Burning));
        return &c;
      });
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::Whirlwind)) {`,
    },
    {
      note: "itr: Freeze 内层的 SameTeam 用 1",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `        c.add(s(collision_val::kSameTeam), u"==", n(0));
        c.or_(s(collision_val::kVictimState), u"==", num(StateEnum::Frozen));`,
      to: `        c.add(s(collision_val::kSameTeam), u"==", n(1));
        c.or_(s(collision_val::kVictimState), u"==", num(StateEnum::Frozen));`,
    },
    {
      note: "itr: Whirlwind 的 vrest 默认值写成 2",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    set_default(u"vrest", 1);`,
      to: `    set_default(u"vrest", 2);`,
    },
    {
      note: "itr: Whirlwind 的 injury 判空改成“取反真值”",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    if (is_nullish(field_or(itr, u"injury"))) it->set(u"injury", Value());`,
      to: `    it->set(u"injury", Value());`,
    },
    {
      note: "itr: Whirlwind 的 injury 不写（缺键）",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `    if (is_nullish(field_or(itr, u"injury"))) it->set(u"injury", Value());`,
      to: `    (void)0;`,
    },
    {
      note: "itr: Whirlwind 的 Rudolf 写成 RudolfWeapon",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kRudolf));`,
      to: `        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kRudolfWeapon));`,
    },
    {
      note: "itr: Whirlwind 的外层 or_ 改成 and_",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.or_([&](CondMaker& c) {
        c.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));`,
      to: `      cm.and_([&](CondMaker& c) {
        c.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));`,
    },
    {
      note: "itr: Whirlwind 内层 SameTeam 用 1",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `          c2.add(s(collision_val::kSameTeam), u"==", n(0));`,
      to: `          c2.add(s(collision_val::kSameTeam), u"==", n(1));`,
    },
    {
      note: "itr: CharacterThrew 的 AttackerThrew 用 0",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.add(s(collision_val::kAttackerThrew), u"==", n(1));`,
      to: `      cm.add(s(collision_val::kAttackerThrew), u"==", n(0));`,
    },
    {
      note: "itr: CharacterThrew 的 AttackerType 用 Weapon",
      file: "native/lfw/loader/preprocess_itr.cpp",
      from: `      cm.add(s(collision_val::kAttackerThrew), u"==", n(1));
      cm.and_(s(collision_val::kAttackerType), u"==", num(EntityEnum::Fighter));`,
      to: `      cm.add(s(collision_val::kAttackerThrew), u"==", n(1));
      cm.and_(s(collision_val::kAttackerType), u"==", num(EntityEnum::Weapon));`,
    },

    // ---------------------------------------------------------------- preprocess_action：tester
    {
      note: "action: tester 永远存 undefined",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  a->set(u"tester", truthy(test) ? test : Value());`,
      to: `  a->set(u"tester", Value());`,
    },
    {
      note: "action: tester 的三目写反",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  a->set(u"tester", truthy(test) ? test : Value());`,
      to: `  a->set(u"tester", truthy(test) ? Value() : test);`,
    },
    {
      note: "action: tester 的键名写成 __tester",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  a->set(u"tester", truthy(test) ? test : Value());`,
      to: `  a->set(u"__tester", truthy(test) ? test : Value());`,
    },
    {
      note: "action: test 读成 type",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  const Value test = field_of(action, u"test");`,
      to: `  const Value test = field_of(action, u"type");`,
    },
    {
      note: "action: 不写 tester",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  a->set(u"tester", truthy(test) ? test : Value());`,
      to: `  (void)test;`,
    },
    {
      note: "action: 类型不做大写化",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  const std::u16string type = to_upper_ascii(std::get<std::u16string>(*tv));`,
      to: `  const std::u16string type = std::get<std::u16string>(*tv);`,
    },
    {
      note: "action: 类型不做大写化但仍写回",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  a->set(u"type", Value(type));`,
      to: `  a->set(u"type", Value(std::get<std::u16string>(*tv)));`,
    },
    {
      note: "action: A_SOUND 不再走 path 校验",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `bool is_sound_type(const std::u16string& t) { return t == u"A_SOUND" || t == u"V_SOUND"; }`,
      to: `bool is_sound_type(const std::u16string& t) { return t == u"A_SOUND"; }`,
    },
    {
      note: "action: V_SOUND 不再走 path 校验",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `bool is_sound_type(const std::u16string& t) { return t == u"A_SOUND" || t == u"V_SOUND"; }`,
      to: `bool is_sound_type(const std::u16string& t) { return t == u"V_SOUND"; }`,
    },
    {
      note: "action: path 判空改成“直接通过”",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `bool sound_path_iterable(const Value& data) {
  if (is_nullish(data)) return false;`,
      to: `bool sound_path_iterable(const Value& data) {
  if (is_nullish(data)) return true;`,
    },
    {
      note: "action: path 只认字符串（不认数组）",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  return is_str(path) || as_array(path) != nullptr;`,
      to: `  return is_str(path);`,
    },
    {
      note: "action: next_frame 类型列表里 A_NEXT_FRAME 写成 A_DEFEND",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  return t == u"A_NEXT_FRAME" || t == u"V_NEXT_FRAME" || t == u"A_DEFEND" ||`,
      to: `  return t == u"A_DEFEND" || t == u"V_NEXT_FRAME" || t == u"A_DEFEND" ||`,
    },
    {
      note: "action: next_frame 类型列表里丢掉 V_BROKEN_DEFEND",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `         t == u"V_DEFEND" || t == u"A_BROKEN_DEFEND" || t == u"V_BROKEN_DEFEND";`,
      to: `         t == u"V_DEFEND" || t == u"A_BROKEN_DEFEND";`,
    },
    {
      note: "action: 非 next_frame 类型也走 preprocess_next_frame",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  if (!is_next_frame_type(type)) return true;
  Value nf = data;
  return preprocess_next_frame(nf);`,
      to: `  Value nf = data;
  return preprocess_next_frame(nf);`,
    },
    {
      note: "action: next_frame 类型不处理 data（直接成功）",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  if (!is_next_frame_type(type)) return true;
  Value nf = data;
  return preprocess_next_frame(nf);`,
      to: `  if (!is_next_frame_type(type)) return true;
  return true;`,
    },
    {
      note: "action: 缺 type 时不抛（当成成功）",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  if (tv == nullptr || !is_str(*tv)) return false;`,
      to: `  if (tv == nullptr || !is_str(*tv)) return true;`,
    },
    {
      note: "action: action 不是对象时当成成功",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  Object* a = as_object(action);
  if (a == nullptr) return false;`,
      to: `  Object* a = as_object(action);
  if (a == nullptr) return true;`,
    },
  ],
};
