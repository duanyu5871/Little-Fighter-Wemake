// `loader/preprocess_frame` + `utils/read_nums`（外加 `preprocess_frame` 调用 `cook_frame_indicator_info`
// 的返回值 / `make_frame_behavior` 的实参）。
//
// 用例：`cases/loader_frames/all.txt`（`frame` op + `rn` op，共 493 行）。
//
// 有意不覆盖（不可观察或按构造等价）：
//   * `ctx` 不是对象时的 `return false`：harness 的 TS 侧要先往 ctx 上挂 `lfw` / `jobs`，
//     标量 ctx 会在 harness 里就抛，用例表达不出来；
//   * weapon 分支里的 `if (d == nullptr) return false;`：`is_weapon_data(data)` 为真就说明
//     `data` 是对象，这一支不可达；
//   * `each_entry` 里 `Object*` / `Array*` 两个分支的先后：对象和数组在端口里是不同类型；
//   * `breakfall` 里 `edit` 的 `if (o == nullptr) return;`：能命中 id 100/108 的 `j` 一定是对象；
//   * `preprocess_next_frame` 的调用点本身只在「值是含 nullish 的数组」时才有可观察差异
//     （它只往值上挂 `__judger`，而 `__judger` 两端都剥掉），`seqs` 的 `preprocess_next_frame`
//     失败分支同理；
//   * `frame.pics` 的 `arr[i] = preprocess_pic(pic)`：`preprocess_pic` 就地改并返回**同一个**
//     对象（非对象时原样返回），写回是恒等操作；
//   * `fold_aabb` 前那段 `bdy` / `itr` 的 `else { return false; }`：走到这里时 `bdy` / `itr`
//     只可能是假值或数组（上面那个 `?.forEach` 循环已经对非数组返回过 false 了）⇒ 不可达；
//   * `frame.sound` 的加载任务端口不落地（见 DESIGN §65 的偏差）；
//   * `preprocess_ball_frame` 里 `data.base` 缺失时 TS 会抛、端口跳过 —— 这是 V41 遗留的已知
//     偏差，本名单的用例一律给 `base`（见 README 偏差表）。
export default {
  subject: "loader_frames",
  cases: ["all"],
  mutations: [
    // ---------------------------------------------------------------- preprocess_frame：入口与 prefab
    {
      note: "frame: ctx.data 读成 ctx.data_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const Value data = field_or(ctx, u"data");`,
      to: `  const Value data = field_or(ctx, u"data_");`,
    },
    {
      note: "frame: ctx.frame 读成 ctx.frame_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const Value raw_frame = field_or(ctx, u"frame");`,
      to: `  const Value raw_frame = field_or(ctx, u"frame_");`,
    },
    {
      note: "frame: prefab 表用 bdy_prefabs",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const ResolvePrefabResult merged = resolve_prefab(raw_frame, field_or(data, u"frame_prefabs"));`,
      to: `  const ResolvePrefabResult merged = resolve_prefab(raw_frame, field_or(data, u"bdy_prefabs"));`,
    },
    {
      note: "frame: prefab 表从 frame 上取",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const ResolvePrefabResult merged = resolve_prefab(raw_frame, field_or(data, u"frame_prefabs"));`,
      to: `  const ResolvePrefabResult merged = resolve_prefab(raw_frame, field_or(raw_frame, u"frame_prefabs"));`,
    },
    {
      note: "frame: 错误 tag 写成 preprocess_itr",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    error = prefab_error_message(u"preprocess_frame", who, u"frame", merged);`,
      to: `    error = prefab_error_message(u"preprocess_itr", who, u"frame", merged);`,
    },
    {
      note: "frame: 错误里的 what 写成 bdy",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    error = prefab_error_message(u"preprocess_frame", who, u"frame", merged);`,
      to: `    error = prefab_error_message(u"preprocess_frame", who, u"bdy", merged);`,
    },
    {
      note: "frame: 错误里的 id 用 data",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        to_string(field_or(data, u"id")) + u":" + to_string(field_or(raw_frame, u"id"));`,
      to: `        to_string(field_or(data, u"id")) + u":" + to_string(field_or(data, u"id"));`,
    },
    {
      note: "frame: 错误里的分隔符改成 -",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        to_string(field_or(data, u"id")) + u":" + to_string(field_or(raw_frame, u"id"));`,
      to: `        to_string(field_or(data, u"id")) + u"-" + to_string(field_or(raw_frame, u"id"));`,
    },
    {
      note: "frame: prefab 失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!merged.ok) {`,
      to: `  if (merged.ok) {`,
    },
    {
      note: "frame: 用原始帧而不是 merged",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  Value frame = merged.value;`,
      to: `  Value frame = raw_frame;`,
    },
    {
      note: "frame: frame 不是对象也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (f == nullptr) return false;`,
      to: `  if (f == nullptr) return true;`,
    },
    // ---------------------------------------------------------------- processed 门
    {
      note: "frame: processed 用严格比较",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!equals(field_or(data, u"processed"), Value(false))) {`,
      to: `  if (!strict_equals(field_or(data, u"processed"), Value(false))) {`,
    },
    {
      note: "frame: processed 和 true 比",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!equals(field_or(data, u"processed"), Value(false))) {`,
      to: `  if (!equals(field_or(data, u"processed"), Value(true))) {`,
    },
    {
      note: "frame: processed 判断反了",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!equals(field_or(data, u"processed"), Value(false))) {`,
      to: `  if (equals(field_or(data, u"processed"), Value(false))) {`,
    },
    {
      note: "frame: processed 键写错",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!equals(field_or(data, u"processed"), Value(false))) {`,
      to: `  if (!equals(field_or(data, u"processed_"), Value(false))) {`,
    },
    // ---------------------------------------------------------------- ball 分支
    {
      note: "frame: ball 分支用 is_weapon_data 判",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  } else if (entity::is_ball_data(data)) {`,
      to: `  } else if (entity::is_weapon_data(data)) {`,
    },
    {
      note: "frame: 不调用 preprocess_ball_frame",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    preprocess_ball_frame(ctx);`,
      to: `    (void)ctx;`,
    },
    // ---------------------------------------------------------------- weapon 分支
    {
      note: "frame: weapon 分支用 is_fighter_data 判",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  } else if (entity::is_weapon_data(data)) {`,
      to: `  } else if (entity::is_fighter_data(data)) {`,
    },
    {
      note: "frame: indexes 假值时不建对象",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!truthy(indexes)) {`,
      to: `    if (false) {`,
    },
    {
      note: "frame: indexes 挂到 indexes_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      d->set(u"indexes", indexes);`,
      to: `      d->set(u"indexes_", indexes);`,
    },
    {
      note: "frame: indexes.in_the_skys 键写错",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    Value in_the_skys = field_or(indexes, u"in_the_skys");`,
      to: `    Value in_the_skys = field_or(indexes, u"in_the_skys_");`,
    },
    {
      note: "frame: indexes.throwings 键写错",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    Value throwings = field_or(indexes, u"throwings");`,
      to: `    Value throwings = field_or(indexes, u"throwings_");`,
    },
    {
      note: "frame: indexes.on_hands 键写错",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    Value on_hands = field_or(indexes, u"on_hands");`,
      to: `    Value on_hands = field_or(indexes, u"on_hands_");`,
    },
    {
      note: "frame: in_the_skys 假值时不兜底成数组",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!truthy(in_the_skys)) in_the_skys = Value(std::make_shared<Array>(Array()));`,
      to: `    if (false) in_the_skys = Value(std::make_shared<Array>(Array()));`,
    },
    {
      note: "frame: throwings 假值时不兜底成数组",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!truthy(throwings)) throwings = Value(std::make_shared<Array>(Array()));`,
      to: `    if (false) throwings = Value(std::make_shared<Array>(Array()));`,
    },
    {
      note: "frame: on_hands 假值时不兜底成数组",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!truthy(on_hands)) on_hands = Value(std::make_shared<Array>(Array()));`,
      to: `    if (false) on_hands = Value(std::make_shared<Array>(Array()));`,
    },
    {
      note: "frame: frame_id 取 frame.id_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    const Value frame_id = field_or(frame, u"id");`,
      to: `    const Value frame_id = field_or(frame, u"id_");`,
    },
    {
      note: "frame: 状态从 state_ 取",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    const Value state = field_or(frame, u"state");
    const auto push_id = [&](Array* a) {`,
      to: `    const Value state = field_or(frame, u"state_");
    const auto push_id = [&](Array* a) {`,
    },
    {
      note: "frame: 已有值不是数组时也当成功",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      if (a == nullptr) return false;  // 已有的值不是数组 ⇒ TS 的 \`push\` 抛`,
      to: `      if (a == nullptr) return true;  // 已有的值不是数组 ⇒ TS 的 \`push\` 抛`,
    },
    {
      note: "frame: push 的是 undefined 而不是 frame.id",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      a->push_back(frame_id);`,
      to: `      a->push_back(Value());`,
    },
    {
      note: "frame: Weapon_InTheSky 的 push 给了 throwings",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (state_is(state, StateEnum::Weapon_InTheSky)) {
      if (!push_id(skys)) return false;`,
      to: `    if (state_is(state, StateEnum::Weapon_InTheSky)) {
      if (!push_id(thrs)) return false;`,
    },
    {
      note: "frame: Weapon_InTheSky 分支漏了 set_hit_flag",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (state_is(state, StateEnum::Weapon_InTheSky)) {
      if (!push_id(skys)) return false;
      if (!each_set_hit_flag_both(field_or(frame, u"bdy"))) return false;`,
      to: `    if (state_is(state, StateEnum::Weapon_InTheSky)) {
      if (!push_id(skys)) return false;
      if (false) return false;`,
    },
    {
      note: "frame: 清 itr 的状态判断写成 Weapon_OnHand",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    } else if (state_is(state, StateEnum::Weapon_Rebounding) ||
               state_is(state, StateEnum::HeavyWeapon_JustOnGround)) {`,
      to: `    } else if (state_is(state, StateEnum::Weapon_Rebounding) ||
               state_is(state, StateEnum::Weapon_OnHand)) {`,
    },
    {
      note: "frame: HeavyWeapon_JustOnGround 不再命中",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    } else if (state_is(state, StateEnum::Weapon_Rebounding) ||
               state_is(state, StateEnum::HeavyWeapon_JustOnGround)) {`,
      to: `    } else if (state_is(state, StateEnum::Weapon_Rebounding) ||
               state_is(state, StateEnum::HeavyWeapon_OnGround)) {`,
    },
    {
      note: "frame: itr 清成 undefined 但键写错",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      f->set(u"itr", Value());`,
      to: `      f->set(u"itr_", Value());`,
    },
    {
      note: "frame: 清的是 bdy 而不是 itr",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      f->set(u"itr", Value());`,
      to: `      f->set(u"bdy", Value());`,
    },
    {
      note: "frame: Weapon_Throwing 的 push 给了 in_the_skys",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    } else if (state_is(state, StateEnum::Weapon_Throwing)) {
      if (!push_id(thrs)) return false;`,
      to: `    } else if (state_is(state, StateEnum::Weapon_Throwing)) {
      if (!push_id(skys)) return false;`,
    },
    {
      note: "frame: Weapon_Throwing 分支漏了 set_hit_flag",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    } else if (state_is(state, StateEnum::Weapon_Throwing)) {
      if (!push_id(thrs)) return false;
      if (!each_set_hit_flag_both(field_or(frame, u"bdy"))) return false;`,
      to: `    } else if (state_is(state, StateEnum::Weapon_Throwing)) {
      if (!push_id(thrs)) return false;
      if (false) return false;`,
    },
    {
      note: "frame: HeavyWeapon_InTheSky 只 push 一处",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      if (!push_id(skys)) return false;
      if (!push_id(thrs)) return false;
      if (!each_set_hit_flag_both(field_or(frame, u"bdy"))) return false;`,
      to: `      if (!push_id(skys)) return false;
      if (false) return false;
      if (!each_set_hit_flag_both(field_or(frame, u"bdy"))) return false;`,
    },
    {
      note: "frame: HeavyWeapon_InTheSky 分支漏了 set_hit_flag",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      if (!push_id(skys)) return false;
      if (!push_id(thrs)) return false;
      if (!each_set_hit_flag_both(field_or(frame, u"bdy"))) return false;`,
      to: `      if (!push_id(skys)) return false;
      if (!push_id(thrs)) return false;
      if (false) return false;`,
    },
    {
      note: "frame: OnHand 的 push 给了 in_the_skys",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `               state_is(state, StateEnum::HeavyWeapon_OnHand)) {
      if (!push_id(hands)) return false;`,
      to: `               state_is(state, StateEnum::HeavyWeapon_OnHand)) {
      if (!push_id(skys)) return false;`,
    },
    {
      note: "frame: Weapon_OnHand 不再命中",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    } else if (state_is(state, StateEnum::Weapon_OnHand) ||
               state_is(state, StateEnum::HeavyWeapon_OnHand)) {`,
      to: `    } else if (state_is(state, StateEnum::Weapon_Rebounding) ||
               state_is(state, StateEnum::HeavyWeapon_OnHand)) {`,
    },
    {
      note: "frame: HeavyWeapon_OnHand 不再命中",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    } else if (state_is(state, StateEnum::Weapon_OnHand) ||
               state_is(state, StateEnum::HeavyWeapon_OnHand)) {`,
      to: `    } else if (state_is(state, StateEnum::Weapon_OnHand) ||
               state_is(state, StateEnum::Weapon_Rebounding)) {`,
    },
    {
      note: "frame: in_the_skys 空数组也挂回去",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (skys != nullptr && !skys->empty()) ix->set(u"in_the_skys", in_the_skys);`,
      to: `    if (skys != nullptr && skys->empty()) ix->set(u"in_the_skys", in_the_skys);`,
    },
    {
      note: "frame: in_the_skys 挂到 in_the_skys_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (skys != nullptr && !skys->empty()) ix->set(u"in_the_skys", in_the_skys);`,
      to: `    if (skys != nullptr && !skys->empty()) ix->set(u"in_the_skys_", in_the_skys);`,
    },
    {
      note: "frame: throwings 空数组也挂回去",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (thrs != nullptr && !thrs->empty()) ix->set(u"throwings", throwings);`,
      to: `    if (thrs != nullptr && thrs->empty()) ix->set(u"throwings", throwings);`,
    },
    {
      note: "frame: throwings 挂到 throwings_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (thrs != nullptr && !thrs->empty()) ix->set(u"throwings", throwings);`,
      to: `    if (thrs != nullptr && !thrs->empty()) ix->set(u"throwings_", throwings);`,
    },
    {
      note: "frame: on_hands 空数组也挂回去",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (hands != nullptr && !hands->empty()) ix->set(u"on_hands", on_hands);`,
      to: `    if (hands != nullptr && hands->empty()) ix->set(u"on_hands", on_hands);`,
    },
    {
      note: "frame: on_hands 挂到 on_hands_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (hands != nullptr && !hands->empty()) ix->set(u"on_hands", on_hands);`,
      to: `    if (hands != nullptr && !hands->empty()) ix->set(u"on_hands_", on_hands);`,
    },
    // ---------------------------------------------------------------- fighter 的两张表
    {
      note: "frame: fighter 分支用 is_weapon_data 判",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (entity::is_fighter_data(data)) {`,
      to: `  if (entity::is_weapon_data(data)) {`,
    },
    {
      note: "frame: stat_recover 的豁免表把 Falling 换成 Standing",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!(state_is(state, StateEnum::Falling) || state_is(state, StateEnum::Caught) ||`,
      to: `    if (!(state_is(state, StateEnum::Standing) || state_is(state, StateEnum::Caught) ||`,
    },
    {
      note: "frame: stat_recover 的豁免表把 Caught 换成 Standing",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!(state_is(state, StateEnum::Falling) || state_is(state, StateEnum::Caught) ||`,
      to: `    if (!(state_is(state, StateEnum::Falling) || state_is(state, StateEnum::Standing) ||`,
    },
    {
      note: "frame: stat_recover 的豁免表把 Injured 换成 Walking",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `          state_is(state, StateEnum::Injured) || state_is(state, StateEnum::Frozen) ||`,
      to: `          state_is(state, StateEnum::Walking) || state_is(state, StateEnum::Frozen) ||`,
    },
    {
      note: "frame: stat_recover 的豁免表把 Frozen 换成 Running",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `          state_is(state, StateEnum::Injured) || state_is(state, StateEnum::Frozen) ||`,
      to: `          state_is(state, StateEnum::Injured) || state_is(state, StateEnum::Running) ||`,
    },
    {
      note: "frame: stat_recover 的豁免表把 Burning 换成 Jump",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `          state_is(state, StateEnum::Burning))) {`,
      to: `          state_is(state, StateEnum::Jump))) {`,
    },
    {
      note: "frame: stat_recover 的值写成 2",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      set_if_nullish(*f, u"stat_recover", n(1));`,
      to: `      set_if_nullish(*f, u"stat_recover", n(2));`,
    },
    {
      note: "frame: stat_recover 的键写成 stat_recover_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      set_if_nullish(*f, u"stat_recover", n(1));`,
      to: `      set_if_nullish(*f, u"stat_recover_", n(1));`,
    },
    {
      note: "frame: toughness_recover 的表把 Standing 换成 Defend",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (state_is(state, StateEnum::Standing) || state_is(state, StateEnum::Walking) ||`,
      to: `    if (state_is(state, StateEnum::Defend) || state_is(state, StateEnum::Walking) ||`,
    },
    {
      note: "frame: toughness_recover 的表把 Walking 换成 Defend",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (state_is(state, StateEnum::Standing) || state_is(state, StateEnum::Walking) ||`,
      to: `    if (state_is(state, StateEnum::Standing) || state_is(state, StateEnum::Defend) ||`,
    },
    {
      note: "frame: toughness_recover 的表把 Running 换成 Defend",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        state_is(state, StateEnum::Running) || state_is(state, StateEnum::Jump) ||`,
      to: `        state_is(state, StateEnum::Defend) || state_is(state, StateEnum::Jump) ||`,
    },
    {
      note: "frame: toughness_recover 的表把 Jump 换成 Defend",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        state_is(state, StateEnum::Running) || state_is(state, StateEnum::Jump) ||`,
      to: `        state_is(state, StateEnum::Running) || state_is(state, StateEnum::Defend) ||`,
    },
    {
      note: "frame: toughness_recover 的表把 Dash 换成 Defend",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        state_is(state, StateEnum::Dash) || state_is(state, StateEnum::Lying) ||`,
      to: `        state_is(state, StateEnum::Defend) || state_is(state, StateEnum::Lying) ||`,
    },
    {
      note: "frame: toughness_recover 的表把 Lying 换成 Defend",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        state_is(state, StateEnum::Dash) || state_is(state, StateEnum::Lying) ||`,
      to: `        state_is(state, StateEnum::Dash) || state_is(state, StateEnum::Defend) ||`,
    },
    {
      note: "frame: toughness_recover 的表把 Rowing 换成 Defend",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        state_is(state, StateEnum::Rowing)) {`,
      to: `        state_is(state, StateEnum::Defend)) {`,
    },
    {
      note: "frame: toughness_recover 的值写成 0",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      set_if_nullish(*f, u"toughness_recover", n(1));`,
      to: `      set_if_nullish(*f, u"toughness_recover", n(0));`,
    },
    {
      note: "frame: toughness_recover 的键写成 toughness_recover_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      set_if_nullish(*f, u"toughness_recover", n(1));`,
      to: `      set_if_nullish(*f, u"toughness_recover_", n(1));`,
    },
    // ---------------------------------------------------------------- width / height / indicator
    {
      note: "frame: frame.pic 读成 frame.pic_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const Value frame_pic = field_or(frame, u"pic");`,
      to: `  const Value frame_pic = field_or(frame, u"pic_");`,
    },
    {
      note: "frame: width 从 frame.w 取",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  set_if_nullish(*f, u"width", or_nullish(field_or(frame_pic, u"w"), n(0.0)));`,
      to: `  set_if_nullish(*f, u"width", or_nullish(field_or(frame, u"w"), n(0.0)));`,
    },
    {
      note: "frame: width 读 pic.h",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  set_if_nullish(*f, u"width", or_nullish(field_or(frame_pic, u"w"), n(0.0)));`,
      to: `  set_if_nullish(*f, u"width", or_nullish(field_or(frame_pic, u"h"), n(0.0)));`,
    },
    {
      note: "frame: width 的兜底值写成 1",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  set_if_nullish(*f, u"width", or_nullish(field_or(frame_pic, u"w"), n(0.0)));`,
      to: `  set_if_nullish(*f, u"width", or_nullish(field_or(frame_pic, u"w"), n(1.0)));`,
    },
    {
      note: "frame: height 的兜底值写成 1",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  set_if_nullish(*f, u"height", or_nullish(field_or(frame_pic, u"h"), n(0.0)));`,
      to: `  set_if_nullish(*f, u"height", or_nullish(field_or(frame_pic, u"h"), n(1.0)));`,
    },
    {
      note: "frame: height 读 pic.w",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  set_if_nullish(*f, u"height", or_nullish(field_or(frame_pic, u"h"), n(0.0)));`,
      to: `  set_if_nullish(*f, u"height", or_nullish(field_or(frame_pic, u"w"), n(0.0)));`,
    },
    {
      note: "frame: 不跑 cook_frame_indicator_info",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!dat_translator::cook_frame_indicator_info(frame)) return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "frame: make_frame_behavior 只对 weapon 调",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (entity::is_weapon_data(data) || entity::is_ball_data(data)) {`,
      to: `  if (entity::is_weapon_data(data)) {`,
    },
    {
      note: "frame: make_frame_behavior 的 oid 取 data.id_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    dat_translator::make_frame_behavior(frame, to_string(field_or(data, u"id")));`,
      to: `    dat_translator::make_frame_behavior(frame, to_string(field_or(data, u"id_")));`,
    },
    // ---------------------------------------------------------------- seqs
    {
      note: "frame: seqs 读成 seqs_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const Value seqs = field_or(frame, u"seqs");`,
      to: `  const Value seqs = field_or(frame, u"seqs_");`,
    },
    {
      note: "frame: seqs 假值也建 __seq_map",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (truthy(seqs)) {`,
      to: `  if (false) {`,
    },
    {
      note: "frame: __seq_map 键写错",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"__seq_map", seq_map);`,
      to: `    f->set(u"__seq_map_", seq_map);`,
    },
    {
      note: "frame: seqs 回调不跳过假值",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      if (!truthy(v)) return true;
      if (!preprocess_next_frame(v)) return false;`,
      to: `      if (false) return true;
      if (!preprocess_next_frame(v)) return false;`,
    },
    {
      note: "frame: __seq_map 的键加后缀",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      as_mut(seq_map)->set(key, v);`,
      to: `      as_mut(seq_map)->set(key + u"_", v);`,
    },
    {
      note: "frame: seqs 遍历失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!ok) return false;
  }

  // 按键受身的限制`,
      to: `    if (false) return false;
  }

  // 按键受身的限制`,
    },
    // ---------------------------------------------------------------- breakfall
    {
      note: "frame: breakfall 判的是 Burning 而不是 Falling",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (state_is(field_or(frame, u"state"), StateEnum::Falling)) {`,
      to: `  if (state_is(field_or(frame, u"state"), StateEnum::Burning)) {`,
    },
    {
      note: "frame: hit.j 读成 hit.j_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    const Value j = field_or(field_or(frame, u"hit"), u"j");`,
      to: `    const Value j = field_or(field_or(frame, u"hit"), u"j_");`,
    },
    {
      note: "frame: hit 读成 hit_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    const Value j = field_or(field_or(frame, u"hit"), u"j");`,
      to: `    const Value j = field_or(field_or(frame, u"hit_"), u"j");`,
    },
    {
      note: "frame: 只有 id 100 换表达式",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        if (id == u"100" || id == u"108") o->set(u"expression", expr);`,
      to: `        if (id == u"100") o->set(u"expression", expr);`,
    },
    {
      note: "frame: 只有 id 108 换表达式",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        if (id == u"100" || id == u"108") o->set(u"expression", expr);`,
      to: `        if (id == u"108") o->set(u"expression", expr);`,
    },
    {
      note: "frame: breakfall 的键写成 expression_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        if (id == u"100" || id == u"108") o->set(u"expression", expr);`,
      to: `        if (id == u"100" || id == u"108") o->set(u"expression_", expr);`,
    },
    {
      note: "frame: breakfall 的 id 取 id_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `        const std::u16string id = to_string(field_or(nf, u"id"));`,
      to: `        const std::u16string id = to_string(field_or(nf, u"id_"));`,
    },
    {
      note: "frame: j 是数组时不处理",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      if (const Array* const a = as_array(list)) {
        for (size_t i = 0; i < a->size(); ++i) edit(a->at(i));
      } else {
        edit(list);
      }`,
      to: `      if (const Array* const a = as_array(list)) {
        for (size_t i = 0; i < a->size(); ++i) edit(a->at(i));
      } else {
      }`,
    },
    // ---------------------------------------------------------------- hit / hold / key_down / key_up
    {
      note: "frame: hit 读成 hit_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  Value hit_map = field_or(frame, u"hit");`,
      to: `  Value hit_map = field_or(frame, u"hit_");`,
    },
    {
      note: "frame: hold 读成 hold_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  Value hold_map = field_or(frame, u"hold");`,
      to: `  Value hold_map = field_or(frame, u"hold_");`,
    },
    {
      note: "frame: key_down 读成 key_down_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  Value key_down_map = field_or(frame, u"key_down");`,
      to: `  Value key_down_map = field_or(frame, u"key_down_");`,
    },
    {
      note: "frame: key_up 读成 key_up_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  Value key_up_map = field_or(frame, u"key_up");`,
      to: `  Value key_up_map = field_or(frame, u"key_up_");`,
    },
    {
      note: "frame: hit 遍历失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!each_next_frame(hit_map)) return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "frame: hold 遍历失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!each_next_frame(hold_map)) return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "frame: key_down 遍历失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!each_next_frame(key_down_map)) return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "frame: key_up 遍历失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!each_next_frame(key_up_map)) return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "frame: each_next_frame 不跳过假值",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!truthy(v)) return true;
    return preprocess_next_frame(v);`,
      to: `    if (false) return true;
    return preprocess_next_frame(v);`,
    },
    {
      note: "frame: traversal 给字符串也当没键",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (const std::u16string* const text = std::get_if<std::u16string>(&r)) return text->empty();`,
      to: `  if (std::get_if<std::u16string>(&r) != nullptr) return true;`,
    },
    // ---------------------------------------------------------------- next / on_dead / ...
    {
      note: "frame: next 键写成 next_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {`,
      to: `  for (const char16_t* const key : {u"next_", u"on_dead", u"on_exhaustion", u"on_landing"}) {`,
    },
    {
      note: "frame: on_dead 键写成 on_dead_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {`,
      to: `  for (const char16_t* const key : {u"next", u"on_dead_", u"on_exhaustion", u"on_landing"}) {`,
    },
    {
      note: "frame: on_exhaustion 键写成 on_exhaustion_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {`,
      to: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion_", u"on_landing"}) {`,
    },
    {
      note: "frame: on_landing 键写成 on_landing_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {`,
      to: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing_"}) {`,
    },
    {
      note: "frame: next 等假值也处理",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;`,
      to: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {
    Value nf = field_or(frame, key);
    if (false) continue;`,
    },
    {
      note: "frame: next 等处理失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;
    if (!preprocess_next_frame(nf)) return false;`,
      to: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;
    if (false) return false;`,
    },
    {
      note: "frame: on_restrict 等假值也处理",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;`,
      to: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {
    Value nf = field_or(frame, key);
    if (false) continue;`,
    },
    {
      note: "frame: on_restrict 等处理失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;
    if (!preprocess_next_frame(nf)) return false;`,
      to: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;
    if (false) return false;`,
    },
    {
      note: "frame: next 等的键加后缀",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;
    if (!preprocess_next_frame(nf)) return false;
    f->set(std::u16string(key), nf);`,
      to: `  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;
    if (!preprocess_next_frame(nf)) return false;
    f->set(std::u16string(key) + u"_", nf);`,
    },
    {
      note: "frame: on_restrict 等的键加后缀",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;
    if (!preprocess_next_frame(nf)) return false;
    f->set(std::u16string(key), nf);`,
      to: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;
    if (!preprocess_next_frame(nf)) return false;
    f->set(std::u16string(key) + u"_", nf);`,
    },
    // ---------------------------------------------------------------- on_*_restrict 自动补
    {
      note: "frame: on_x_restrict 的存在判断读错键",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!truthy(field_or(frame, u"on_x_restrict")) &&`,
      to: `  if (!truthy(field_or(frame, u"on_x_restrict_")) &&`,
    },
    {
      note: "frame: 自动补按 Fighter 类型判",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Ball))) &&`,
      to: `      equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Fighter))) &&`,
    },
    {
      note: "frame: 类型用严格比较",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Ball))) &&`,
      to: `      strict_equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Ball))) &&`,
    },
    {
      note: "frame: itr 长度改读 bdy",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      (length_truthy(field_or(frame, u"itr")) || length_truthy(field_or(frame, u"bdy"))) &&`,
      to: `      (length_truthy(field_or(frame, u"bdy")) || length_truthy(field_or(frame, u"bdy"))) &&`,
    },
    {
      note: "frame: 不再看 bdy 的长度",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      (length_truthy(field_or(frame, u"itr")) || length_truthy(field_or(frame, u"bdy"))) &&`,
      to: `      (length_truthy(field_or(frame, u"itr")) || false) &&`,
    },
    {
      note: "frame: 状态白名单里的 Ball_Flying 换成 Ball_Hitting",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      (equals(field_or(frame, u"state"), n(static_cast<double>(StateEnum::Ball_Flying))) ||`,
      to: `      (equals(field_or(frame, u"state"), n(static_cast<double>(StateEnum::Ball_Hitting))) ||`,
    },
    {
      note: "frame: 状态白名单里去掉 3005",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `       equals(field_or(frame, u"state"), n(static_cast<double>(StateEnum::Ball_3005))) ||`,
      to: `       false ||`,
    },
    {
      note: "frame: 状态白名单里去掉 3006",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `       equals(field_or(frame, u"state"), n(static_cast<double>(StateEnum::Ball_3006))))) {`,
      to: `       false)) {`,
    },
    {
      note: "frame: base 读成 base_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    const Value base = field_or(data, u"base");`,
      to: `    const Value base = field_or(data, u"base_");`,
    },
    {
      note: "frame: base 缺失也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (is_nullish(base)) return false;`,
      to: `    if (false) return false;`,
    },
    {
      note: "frame: hit_sounds 读成 hit_sounds_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    const Value sounds = field_or(base, u"hit_sounds");`,
      to: `    const Value sounds = field_or(base, u"hit_sounds_");`,
    },
    {
      note: "frame: on_x_restrict 的 id 写成 21",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"on_x_restrict", make_obj({{u"id", s(u"20")}, {u"sound", sounds}}));`,
      to: `    f->set(u"on_x_restrict", make_obj({{u"id", s(u"21")}, {u"sound", sounds}}));`,
    },
    {
      note: "frame: on_x_restrict 的键写成 on_x_restrict_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"on_x_restrict", make_obj({{u"id", s(u"20")}, {u"sound", sounds}}));`,
      to: `    f->set(u"on_x_restrict_", make_obj({{u"id", s(u"20")}, {u"sound", sounds}}));`,
    },
    {
      note: "frame: on_x_restrict 的 sound 取错键",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"on_x_restrict", make_obj({{u"id", s(u"20")}, {u"sound", sounds}}));`,
      to: `    f->set(u"on_x_restrict", make_obj({{u"id", s(u"20")}, {u"sound", field_or(base, u"hit_sounds_")}}));`,
    },
    {
      note: "frame: on_y_restrict 的 id 写成 21",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"on_y_restrict", make_obj({{u"id", s(u"20")}, {u"sound", sounds}}));`,
      to: `    f->set(u"on_y_restrict", make_obj({{u"id", s(u"21")}, {u"sound", sounds}}));`,
    },
    {
      note: "frame: on_y_restrict 的键写成 on_y_restrict_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"on_y_restrict", make_obj({{u"id", s(u"20")}, {u"sound", sounds}}));`,
      to: `    f->set(u"on_y_restrict_", make_obj({{u"id", s(u"20")}, {u"sound", sounds}}));`,
    },
    {
      note: "frame: on_y_restrict 的 sound 取成 undefined",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"on_y_restrict", make_obj({{u"id", s(u"20")}, {u"sound", sounds}}));`,
      to: `    f->set(u"on_y_restrict", make_obj({{u"id", s(u"20")}, {u"sound", Value()}}));`,
    },
    {
      note: "frame: on_restrict 的键写成 on_restrict_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {`,
      to: `  for (const char16_t* const key : {u"on_restrict_", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {`,
    },
    {
      note: "frame: on_x_restrict 的键写成 on_x_restrict_（遍历）",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {`,
      to: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict_", u"on_y_restrict",
                                    u"on_z_restrict"}) {`,
    },
    {
      note: "frame: on_y_restrict 的键写成 on_y_restrict_（遍历）",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {`,
      to: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict_",
                                    u"on_z_restrict"}) {`,
    },
    {
      note: "frame: on_z_restrict 的键写成 on_z_restrict_（遍历）",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {`,
      to: `  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict_"}) {`,
    },
    // ---------------------------------------------------------------- bdy / itr 与 __hit_ground_*
    {
      note: "frame: bdy 的键写成 bdy_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"bdy", u"itr"}) {`,
      to: `  for (const char16_t* const key : {u"bdy_", u"itr"}) {`,
    },
    {
      note: "frame: itr 的键写成 itr_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  for (const char16_t* const key : {u"bdy", u"itr"}) {`,
      to: `  for (const char16_t* const key : {u"bdy", u"itr_"}) {`,
    },
    {
      note: "frame: bdy / itr 是 null 也当失败",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (is_nullish(list)) continue;
    Array* const a = as_array(list);`,
      to: `    if (is_nullish(list)) return false;
    Array* const a = as_array(list);`,
    },
    {
      note: "frame: bdy / itr 非数组也当成功",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (a == nullptr) return false;  // TS 的 \`?.forEach\` 对非数组会抛
    const size_t count = a->size();`,
      to: `    if (a == nullptr) return true;  // TS 的 \`?.forEach\` 对非数组会抛
    const size_t count = a->size();`,
    },
    {
      note: "frame: bdy / itr 循环体不执行",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    const size_t count = a->size();
    for (size_t i = 0; i < count; ++i) {`,
      to: `    const size_t count = a->size();
    for (size_t i = 0; i < count && false; ++i) {`,
    },
    {
      note: "frame: 子 ctx 里不放 bdy / itr 项",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      as_mut(sub)->set(std::u16string(key), a->at(i));`,
      to: `      as_mut(sub)->set(std::u16string(key), Value());`,
    },
    {
      note: "frame: 子 ctx 不放 index",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      if (!(is_bdy ? preprocess_bdy(sub, error) : preprocess_itr(sub, error))) return false;`,
      to: `      if (!(is_bdy ? true : preprocess_itr(sub, error))) return false;`,
    },
    {
      note: "frame: bdy 的分派写成 itr",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    const bool is_bdy = std::u16string(key) == u"bdy";`,
      to: `    const bool is_bdy = std::u16string(key) == u"itr";`,
    },
    {
      note: "frame: bdy / itr 的处理失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      if (!(is_bdy ? preprocess_bdy(sub, error) : preprocess_itr(sub, error))) return false;
      const Value item = field_or(sub, key);`,
      to: `      if (false) return false;
      const Value item = field_or(sub, key);`,
    },
    {
      note: "frame: bdy / itr 的处理结果不写回数组（保留子 ctx 的结果）",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      a->at(i) = item;`,
      to: `      (void)item;`,
    },
    {
      note: "frame: __hit_ground_bdys 的键写成 __hit_ground_itrs",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      const char16_t* const bucket_key = is_bdy ? u"__hit_ground_bdys" : u"__hit_ground_itrs";`,
      to: `      const char16_t* const bucket_key = is_bdy ? u"__hit_ground_itrs" : u"__hit_ground_itrs";`,
    },
    {
      note: "frame: on_hit_ground 判断读错键",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      if (!truthy(field_or(item, u"on_hit_ground"))) continue;`,
      to: `      if (!truthy(field_or(item, u"on_hit_ground_"))) continue;`,
    },
    {
      note: "frame: __hit_ground_* 假值时不建数组",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      if (!truthy(bucket)) {`,
      to: `      if (false) {`,
    },
    {
      note: "frame: __hit_ground_* 不 push",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      bucket_array->push_back(item);`,
      to: `      (void)bucket_array;`,
    },
    {
      note: "frame: __hit_ground_* 非数组也当成功",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      if (bucket_array == nullptr) return false;  // TS 的 \`?.push\` 对非数组会抛`,
      to: `      if (bucket_array == nullptr) return true;  // TS 的 \`?.push\` 对非数组会抛`,
    },
    // ---------------------------------------------------------------- Burning / BurnRun
    {
      note: "frame: Burning 判成 BurnRun",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (state_is(state, StateEnum::Burning) &&`,
      to: `  if (state_is(state, StateEnum::BurnRun) &&`,
    },
    {
      note: "frame: JulianBall2 例外去掉",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      !equals(field_or(data, u"id"), s(oid::kJulianBall2))) {`,
      to: `      true) {`,
    },
    {
      note: "frame: JulianBall2 例外判成 JulianBall",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      !equals(field_or(data, u"id"), s(oid::kJulianBall2))) {`,
      to: `      !equals(field_or(data, u"id"), s(oid::kJulianBall))) {`,
    },
    {
      note: "frame: 烟的类型参数 1 写成 2",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(1), next)) return false;
    f->set(u"opoint", next);
  } else if (state_is(state, StateEnum::BurnRun)) {`,
      to: `    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(2), next)) return false;
    f->set(u"opoint", next);
  } else if (state_is(state, StateEnum::BurnRun)) {`,
    },
    {
      note: "frame: BurnRun 的烟类型参数 2 写成 1",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(2), next)) return false;`,
      to: `    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(1), next)) return false;`,
    },
    {
      note: "frame: opoint 读成 opoint_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    Value opoint = field_or(frame, u"opoint");
    Value next;
    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(1), next)) return false;`,
      to: `    Value opoint = field_or(frame, u"opoint_");
    Value next;
    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(1), next)) return false;`,
    },
    {
      note: "frame: ensure 的结果键写成 opoint_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(1), next)) return false;
    f->set(u"opoint", next);`,
      to: `    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(1), next)) return false;
    f->set(u"opoint_", next);`,
    },
    {
      note: "frame: BurnRun 的结果键写成 opoint_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(2), next)) return false;
    f->set(u"opoint", next);`,
      to: `    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(2), next)) return false;
    f->set(u"opoint_", next);`,
    },
    {
      note: "frame: ensure 失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(1), next)) return false;`,
      to: `    if (false) return false;`,
    },
    // ---------------------------------------------------------------- center / read_nums
    {
      note: "frame: center 读成 center_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const Value center = field_or(frame, u"center");`,
      to: `  const Value center = field_or(frame, u"center_");`,
    },
    {
      note: "frame: center 假值也读",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (truthy(center)) {`,
      to: `  if (true) {`,
    },
    {
      note: "frame: read_nums 失败也继续",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (!read_nums(center, 2, Value(), nums, &error)) return false;`,
      to: `    if (false) return false;`,
    },
    {
      note: "frame: centerx / centery 写反",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"centerx", nums[0]);
    f->set(u"centery", nums[1]);`,
      to: `    f->set(u"centerx", nums[1]);
    f->set(u"centery", nums[0]);`,
    },
    {
      note: "frame: centerx 的键写成 centery",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"centerx", nums[0]);`,
      to: `    f->set(u"centery", nums[0]);`,
    },
    // ---------------------------------------------------------------- pic / pics / landable / behavior
    {
      note: "frame: 不写回 pic",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  f->set(u"pic", preprocess_frame_pic(frame));`,
      to: `  (void)preprocess_frame_pic(frame);`,
    },
    {
      note: "frame: pic 的键写成 pic_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  f->set(u"pic", preprocess_frame_pic(frame));`,
      to: `  f->set(u"pic_", preprocess_frame_pic(frame));`,
    },
    {
      note: "frame: pics 读成 pics_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  Value pics = field_or(frame, u"pics");`,
      to: `  Value pics = field_or(frame, u"pics_");`,
    },
    {
      note: "frame: pics 是 null 也当失败",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (!is_nullish(pics)) {`,
      to: `  if (true) {`,
    },
    {
      note: "frame: pics 非数组也当成功",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    if (a == nullptr) return false;  // TS 的 \`?.forEach\` 对非数组会抛
    for (size_t i = 0; i < a->size(); ++i) {`,
      to: `    if (a == nullptr) return true;  // TS 的 \`?.forEach\` 对非数组会抛
    for (size_t i = 0; i < a->size(); ++i) {`,
    },
    {
      note: "frame: landable 只在 undefined 时补（写成总是补）",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (is_undefined(field_or(frame, u"landable"))) {`,
      to: `  if (true) {`,
    },
    {
      note: "frame: landable 的键写成 landable_",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"landable",`,
      to: `    f->set(u"landable_",`,
    },
    {
      note: "frame: landable 的球判断改成 Fighter",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `           Value(strict_equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Ball)))`,
      to: `           Value(strict_equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Fighter)))`,
    },
    {
      note: "frame: landable 的 0 / 1 写反",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `                     ? 0.0
                     : 1.0));`,
      to: `                     ? 1.0
                     : 0.0));`,
    },
    {
      note: "frame: behavior 用松散比较",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  if (strict_equals(field_or(frame, u"behavior"),`,
      to: `  if (equals(field_or(frame, u"behavior"),`,
    },
    {
      note: "frame: behavior 的常量写成 Bat",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `                    n(static_cast<double>(FrameBehavior::Boomerang))) &&`,
      to: `                    n(static_cast<double>(FrameBehavior::Bat))) &&`,
    },
    {
      note: "frame: facing 的存在判断读错键",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      is_undefined(field_or(frame, u"facing")) &&`,
      to: `      is_undefined(field_or(frame, u"facing_")) &&`,
    },
    {
      note: "frame: facing 的类型判断改成 Fighter",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      strict_equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Ball)))) {
    f->set(u"facing", n(static_cast<double>(FacingFlag::VX)));`,
      to: `      strict_equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Fighter)))) {
    f->set(u"facing", n(static_cast<double>(FacingFlag::VX)));`,
    },
    {
      note: "frame: facing 写成 AntiVX",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `    f->set(u"facing", n(static_cast<double>(FacingFlag::VX)));`,
      to: `    f->set(u"facing", n(static_cast<double>(FacingFlag::AntiVX)));`,
    },
    // ---------------------------------------------------------------- fold_aabb
    {
      note: "frame: fold_aabb 的 x 默认值写成 1",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      to_number(destructure_or(any, u"x", n(0.0))) - to_number(field_or(frame, u"centerx"));`,
      to: `      to_number(destructure_or(any, u"x", n(1.0))) - to_number(field_or(frame, u"centerx"));`,
    },
    {
      note: "frame: fold_aabb 的 x 读成 w",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      to_number(destructure_or(any, u"x", n(0.0))) - to_number(field_or(frame, u"centerx"));`,
      to: `      to_number(destructure_or(any, u"w", n(0.0))) - to_number(field_or(frame, u"centerx"));`,
    },
    {
      note: "frame: fold_aabb 减 centerx 改加",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      to_number(destructure_or(any, u"x", n(0.0))) - to_number(field_or(frame, u"centerx"));`,
      to: `      to_number(destructure_or(any, u"x", n(0.0))) + to_number(field_or(frame, u"centerx"));`,
    },
    {
      note: "frame: fold_aabb 读 centerx 改成 centery",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `      to_number(destructure_or(any, u"x", n(0.0))) - to_number(field_or(frame, u"centerx"));`,
      to: `      to_number(destructure_or(any, u"x", n(0.0))) - to_number(field_or(frame, u"centery"));`,
    },
    {
      note: "frame: fold_aabb 的 w 用 + 变成直接 to_number",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const double x2 = to_number(js_add(Value(x1), destructure_or(any, u"w", n(0.0))));`,
      to: `  const double x2 = x1 + to_number(destructure_or(any, u"w", n(0.0)));`,
    },
    {
      note: "frame: fold_aabb 的 z 默认值符号取反",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const Value z = destructure_or(any, u"z", n(-dzl / 2));`,
      to: `  const Value z = destructure_or(any, u"z", n(dzl / 2));`,
    },
    {
      note: "frame: fold_aabb 的 l 默认值写成 0",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const double z2 = to_number(js_add(z, destructure_or(any, u"l", n(dzl))));`,
      to: `  const double z2 = to_number(js_add(z, destructure_or(any, u"l", n(0.0))));`,
    },
    {
      note: "frame: fold_aabb 的 z1 用 max",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  frame.set(u"__aabb_z1",
            Value(min(to_number(or_nullish(field_or(frame, u"__aabb_z1"), Value(z1))), z1)));`,
      to: `  frame.set(u"__aabb_z1",
            Value(max(to_number(or_nullish(field_or(frame, u"__aabb_z1"), Value(z1))), z1)));`,
    },
    {
      note: "frame: fold_aabb 的 x1 键写成 __aabb_x2",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  frame.set(u"__aabb_x1",
            Value(min(to_number(or_nullish(field_or(frame, u"__aabb_x1"), Value(x1))), x1)));`,
      to: `  frame.set(u"__aabb_x2",
            Value(min(to_number(or_nullish(field_or(frame, u"__aabb_x1"), Value(x1))), x1)));`,
    },
    {
      note: "frame: fold_aabb 的 x1 回落值不用旧的",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  frame.set(u"__aabb_x1",
            Value(min(to_number(or_nullish(field_or(frame, u"__aabb_x1"), Value(x1))), x1)));`,
      to: `  frame.set(u"__aabb_x1", Value(min(x1, x1)));`,
    },
    {
      note: "frame: fold_aabb 的 x2 用 min",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  frame.set(u"__aabb_x2",
            Value(max(to_number(or_nullish(field_or(frame, u"__aabb_x2"), Value(x2))), x2)));`,
      to: `  frame.set(u"__aabb_x2",
            Value(min(to_number(or_nullish(field_or(frame, u"__aabb_x2"), Value(x2))), x2)));`,
    },
    {
      note: "frame: fold_aabb 的 z2 用 min",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  frame.set(u"__aabb_z2",
            Value(max(to_number(or_nullish(field_or(frame, u"__aabb_z2"), Value(z2))), z2)));`,
      to: `  frame.set(u"__aabb_z2",
            Value(min(to_number(or_nullish(field_or(frame, u"__aabb_z2"), Value(z2))), z2)));`,
    },
    {
      note: "frame: bdy 不参与 fold_aabb",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  } else if (Array* const a = as_array(bdy_list)) {
    for (size_t i = 0; i < a->size(); ++i) fold_aabb(*f, a->at(i), dzl);`,
      to: `  } else if (Array* const a = as_array(bdy_list)) {
    (void)a;`,
    },
    {
      note: "frame: itr 不参与 fold_aabb",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  } else if (Array* const a = as_array(itr_list)) {
    for (size_t i = 0; i < a->size(); ++i) fold_aabb(*f, a->at(i), dzl);`,
      to: `  } else if (Array* const a = as_array(itr_list)) {
    (void)a;`,
    },
    {
      note: "frame: fold_aabb 的 cube 长度取 POW2 那个常量",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  const double dzl = defines::num(u"Defines.DAFUALT_QUBE_LENGTH");`,
      to: `  const double dzl = defines::num(u"Defines.DAFUALT_QUBE_LENGTH_POW2");`,
    },
    {
      note: "frame: itr 也读成 bdy（fold_aabb）",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  Value itr_list = field_or(frame, u"itr");`,
      to: `  Value itr_list = field_or(frame, u"bdy");`,
    },
    {
      note: "frame: 最后不写回 ctx.frame",
      file: "native/lfw/loader/preprocess_frame.cpp",
      from: `  ctx_o->set(u"frame", frame);`,
      to: `  ctx_o->set(u"frame_", frame);`,
    },
    // ---------------------------------------------------------------- read_nums
    {
      note: "read_nums: fallbacks 是数字时不包成数组",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (is_number(fallbacks)) fallbacks = as_array_of(fallbacks);`,
      to: `  if (false) fallbacks = as_array_of(fallbacks);`,
    },
    {
      note: "read_nums: undefined 的 fallbacks 不走默认值",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (is_undefined(fallbacks)) fallbacks = Value(std::make_shared<Array>(Array()));`,
      to: `  if (false) fallbacks = Value(std::make_shared<Array>(Array()));`,
    },
    {
      note: "read_nums: 默认 fallbacks 用 [1]",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (is_undefined(fallbacks)) fallbacks = Value(std::make_shared<Array>(Array()));`,
      to: `  if (is_undefined(fallbacks)) fallbacks = as_array_of(Value(1.0));`,
    },
    {
      note: "read_nums: fallbacks 不校验",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (!is_num_arr(fallbacks)) return fail(u"fallbacks must be number[]", fallbacks);`,
      to: `  if (false) return fail(u"fallbacks must be number[]", fallbacks);`,
    },
    {
      note: "read_nums: fallbacks 的错误文本写成 src 的",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (!is_num_arr(fallbacks)) return fail(u"fallbacks must be number[]", fallbacks);`,
      to: `  if (!is_num_arr(fallbacks)) return fail(u"src must be string or number[]", fallbacks);`,
    },
    {
      note: "read_nums: 错误文本少了 failed 前缀",
      file: "native/lfw/utils/read_nums.cpp",
      from: `      *error = u"[read_nums] failed, " + std::u16string(tail) + u", but got " + to_string(got);`,
      to: `      *error = u"[read_nums] " + std::u16string(tail) + u", but got " + to_string(got);`,
    },
    {
      note: "read_nums: 错误文本少了 failed 后的逗号",
      file: "native/lfw/utils/read_nums.cpp",
      from: `      *error = u"[read_nums] failed, " + std::u16string(tail) + u", but got " + to_string(got);`,
      to: `      *error = u"[read_nums] failed, " + std::u16string(tail) + u" but got " + to_string(got);`,
    },
    {
      note: "read_nums: len < 1 的判断写成 <= 0 之外的 1",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (len < 1) return true;`,
      to: `  if (len < 2) return true;`,
    },
    {
      note: "read_nums: 先补长再校验 len",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (!is_num_arr(fallbacks)) return fail(u"fallbacks must be number[]", fallbacks);
  if (len < 1) return true;`,
      to: `  if (len < 1) return true;
  if (!is_num_arr(fallbacks)) return fail(u"fallbacks must be number[]", fallbacks);`,
    },
    {
      note: "read_nums: fallbacks 的补长用 undefined",
      file: "native/lfw/utils/read_nums.cpp",
      from: `    fb->push_back(truthy(last) ? last : Value(0.0));`,
      to: `    fb->push_back(last);`,
    },
    {
      note: "read_nums: fallbacks 的补长从第一项取（空数组越界）",
      file: "native/lfw/utils/read_nums.cpp",
      from: `    const Value last = fb->empty() ? Value() : fb->at(fb->size() - 1);`,
      to: `    const Value last = fb->empty() ? Value() : fb->at(0);`,
    },
    {
      note: "read_nums: src 是假值时返回空而不是 fallbacks",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (!truthy(src)) {
    out = fb->items();
    return true;
  }`,
      to: `  if (!truthy(src)) {
    return true;
  }`,
    },
    {
      note: "read_nums: src 假值时按 undefined 补",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (!truthy(src)) {
    out = fb->items();
    return true;
  }`,
      to: `  if (!truthy(src)) {
    for (size_t idx = 0; idx < static_cast<size_t>(len); ++idx) out.push_back(Value());
    return true;
  }`,
    },
    {
      note: "read_nums: 字符串不按逗号切",
      file: "native/lfw/utils/read_nums.cpp",
      from: `      const size_t comma = s.find(u',', start);`,
      to: `      const size_t comma = std::u16string::npos;`,
    },
    {
      note: "read_nums: 字符串不先去掉空白",
      file: "native/lfw/utils/read_nums.cpp",
      from: `    if (!is_str_white_space(c)) t.push_back(c);`,
      to: `    t.push_back(c);`,
    },
    {
      note: "read_nums: 转换后的 src 不校验数字数组",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (!is_num_arr(values)) return fail(u"src must be string or number[]", values);`,
      to: `  if (false) return fail(u"src must be string or number[]", values);`,
    },
    {
      note: "read_nums: 转换后的 src 校验用原 src",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (!is_num_arr(values)) return fail(u"src must be string or number[]", values);`,
      to: `  if (!is_num_arr(src)) return fail(u"src must be string or number[]", values);`,
    },
    {
      note: "read_nums: src 的错误文本用原 src",
      file: "native/lfw/utils/read_nums.cpp",
      from: `  if (!is_num_arr(values)) return fail(u"src must be string or number[]", values);`,
      to: `  if (!is_num_arr(values)) return fail(u"src must be string or number[]", src);`,
    },
    {
      note: "read_nums: is_num_arr 只看是数组",
      file: "native/lfw/utils/read_nums.cpp",
      from: `    if (d != nullptr && std::isnan(*d)) return false;`,
      to: `    (void)d;`,
    },
    {
      note: "read_nums: is_num_arr 认为非数字元素不算数",
      file: "native/lfw/utils/read_nums.cpp",
      from: `    if (d != nullptr && std::isnan(*d)) return false;`,
      to: `    if (d == nullptr) return false;`,
    },
    {
      note: "read_nums: 越界判断用 >=（少了 undefined 这个洞）",
      file: "native/lfw/utils/read_nums.cpp",
      from: `    if (idx > list.size()) out.push_back(fb->at(idx));`,
      to: `    if (idx >= list.size()) out.push_back(fb->at(idx));`,
    },
    {
      note: "read_nums: 越界时也给 undefined",
      file: "native/lfw/utils/read_nums.cpp",
      from: `    if (idx > list.size()) out.push_back(fb->at(idx));`,
      to: `    if (idx > list.size()) out.push_back(Value());`,
    },
    {
      note: "read_nums: 越界判断写成 >= 且取 src",
      file: "native/lfw/utils/read_nums.cpp",
      from: `    else out.push_back(idx < list.size() ? list.at(idx) : Value());`,
      to: `    else out.push_back(idx >= list.size() ? list.at(idx) : Value());`,
    },
  ],
};
