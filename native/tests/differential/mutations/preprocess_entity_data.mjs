// `loader/preprocess_entity_data`（外加 `utils/container_help/traversal` 的值版字符串分支、
// `utils/type_check` 的 `is_non_blank_str`）。
//
// 用例：`cases/loader_entity/all.txt`（`ed` op）。
//
// 有意不覆盖（不可观察或按构造等价）：
//   * `if (is_nullish(ctx)) return false;` / `if (is_nullish(data)) return false;`：两个分支去掉后
//     会在后面同一批「读不到字段 ⇒ 失败」的地方兜住，且这段时间里没有任何写操作，失败时渲染出来
//     的 `data` 完全一样；
//   * `if (data.processed != false) { }`：TS 里是空分支（`undefined != false` 为真），端口没有对应
//     代码；
//   * `pre_hitkeys` / `post_hitkeys` 的 `truthy(...)` 门：去掉后 `build_hitkeys_map` 对假值照样
//     不遍历、不失败；
//   * `build_hitkeys_map` 字符串分支里 `if (!truthy(v)) continue;` 与
//     `if (!preprocess_next_frame(v)) return false;`：`v` 是单个字符（恒为真值、恒没有
//     `expression`），这两支不可达；
//   * `on_dead` / `on_exhaustion` 的写回、`data.base.bot` 的写回、`make_entity_special(data)`：
//     `preprocess_next_frame` / `preprocess_bot_data` 就地改并返回同一个对象，写回是恒等操作，
//     而 `make_entity_special` 两端都是空函数；
//   * `traversal` 值版字符串分支里那个「值是那个字符」的 `item`：本 subject 里只有
//     `base.files` 用它，而 `files` 的回调不看值；
//   * `value_length` 对 `null` / `undefined` 返回 `undefined` 还是 `0`：两处用途
//     （`jobs.length`、`pics`）后面都只做真值判断，`undefined` 与 `0` 同真假；
//   * `spread_assign` 的方向（`{ itr, ...ctx }` 写成 `{ ...ctx, itr }`）：用例的 ctx 里没有
//     `itr` / `bdy` / `frame` 这几个键，两种写法给 `preprocess_*` 的字段一样；
//   * `portraits` 的写回、`data.xml`、加载任务（`images.load_*` / `sounds.load`）与 `Ditto.warn`：
//     见 DESIGN §66 的偏差；
//   * `make_fighter_special` 内部 `ensure(data.base.group, …)` 在 `group` 是非数组真值时会抛，
//     端口没有失败通道（用例一律给数组或省略）—— V42 遗留偏差。
//   * `value_length` 的**字符串**分支：两处用途都看不见它 —— `jobs` 是字符串时
//     `if (jobs.length)` 走 `is_str` 那一支（真真假假都成功），而 `frame.pics` 只要是字符串就
//     先被 `preprocess_frame` 里的 `pics?.forEach` 抛掉了；
//   * `make_ball_special` 对稀疏输入的抛（如 `id "220"` 但缺 `frames`）：TS 读
//     `data.frames[50]` 抛，而 3t 的端口把 `frames` 缺失当「跳过」（`void` 返回，没有失败通道）
//     —— **3t 遗留偏差**，本刀不在 `preprocess_entity_data` 里修，用例挑表里能跑通的 id。
export default {
  subject: "loader_entity",
  cases: ["all"],
  mutations: [
    // ---------------------------------------------------------------- 入口与四路 special
    {
      note: "ed: ctx.data 读成 ctx.data_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  Value data = field_or(ctx, u"data");`,
      to: `  Value data = field_or(ctx, u"data_");`,
    },
    {
      note: "ed: ball 分支换成 is_fighter_data",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (entity::is_ball_data(data)) {`,
      to: `  if (entity::is_fighter_data(data)) {`,
    },
    {
      note: "ed: weapon 分支换成 is_ball_data",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  } else if (entity::is_weapon_data(data)) {`,
      to: `  } else if (entity::is_ball_data(data)) {`,
    },
    {
      note: "ed: fighter 分支换成 is_entity_data",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  } else if (entity::is_fighter_data(data)) {`,
      to: `  } else if (entity::is_entity_data(data)) {`,
    },
    {
      note: "ed: 不再调 make_ball_special",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    dat_translator::make_ball_special(data);\n`,
      to: ``,
    },
    {
      note: "ed: 不再调 make_weapon_special",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    dat_translator::make_weapon_special(data);\n`,
      to: ``,
    },
    {
      note: "ed: 不再调 make_fighter_special",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    (void)dat_translator::make_fighter_special(data);\n`,
      to: ``,
    },
    {
      note: "ed: pre_hitkeys 无条件覆盖（不再判 nullish）",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    if (is_nullish(field_or(data, u"pre_hitkeys")))`,
      to: `    if (true)`,
    },
    {
      note: "ed: pre_hitkeys 的 nullish 判断读错键",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    if (is_nullish(field_or(data, u"pre_hitkeys")))`,
      to: `    if (is_nullish(field_or(data, u"pre_hitkey_")))`,
    },
    {
      note: "ed: 默认 pre_hitkeys 的 ja 写成 jb",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  return make_obj({{u"ja", make_obj({{u"reset_keys", n(1)},`,
      to: `  return make_obj({{u"jb", make_obj({{u"reset_keys", n(1)},`,
    },
    {
      note: "ed: 默认 pre_hitkeys 的 reset_keys 写成 0",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `{u"reset_keys", n(1)},`,
      to: `{u"reset_keys", n(0)},`,
    },
    {
      note: "ed: 默认 pre_hitkeys 的 transfrom_to_another 写成 0",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `{u"transfrom_to_another", n(1)},`,
      to: `{u"transfrom_to_another", n(0)},`,
    },
    {
      note: "ed: 默认 expression 的第一项写成 == 1",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  cond.add(s(entity_val::kTransformListSize), u"==", n(2))`,
      to: `  cond.add(s(entity_val::kTransformListSize), u"==", n(1))`,
    },
    {
      note: "ed: 默认 expression 第二项用 or_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      .and_(s(entity_val::kIsOnGround), u"==", n(1))`,
      to: `      .or_(s(entity_val::kIsOnGround), u"==", n(1))`,
    },
    {
      note: "ed: 默认 expression 第三项用 kIsOnGround",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      .and_(s(entity_val::kTransformIndex), u"==", n(1));`,
      to: `      .and_(s(entity_val::kIsOnGround), u"==", n(1));`,
    },
    {
      note: "ed: 默认 expression 丢掉第二项",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      .and_(s(entity_val::kIsOnGround), u"==", n(1))\n`,
      to: ``,
    },
    {
      note: "weapon: test 表达式的运算符换成 !=",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  cond.add(s(collision_val::kVFALLING), u"==", n(0));`,
      to: `  cond.add(s(collision_val::kVFALLING), u"!=", n(0));`,
    },
    {
      note: "weapon: test 表达式换成 entity_val 的字段",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  cond.add(s(collision_val::kVFALLING), u"==", n(0));`,
      to: `  cond.add(s(entity_val::kTransformIndex), u"==", n(0));`,
    },
    // ---------------------------------------------------------------- itr_prefabs
    {
      note: "itr: 表从 data.itr_prefab 取（少个 s）",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  Value itr_prefabs = field_or(data, u"itr_prefabs");`,
      to: `  Value itr_prefabs = field_or(data, u"itr_prefab");`,
    },
    {
      note: "itr: 不再跳过假值项",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (!truthy(itr)) return true;\n`,
      to: ``,
    },
    {
      note: "itr: test 的填充不再限定 weapon 数据",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (entity::is_weapon_data(data) && is_nullish(field_or(itr, u"test"))) {`,
      to: `      if (is_nullish(field_or(itr, u"test"))) {`,
    },
    {
      note: "itr: test 已存在时也覆盖（??= 变成 =）",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (entity::is_weapon_data(data) && is_nullish(field_or(itr, u"test"))) {`,
      to: `      if (entity::is_weapon_data(data) && truthy(field_or(itr, u"test"))) {`,
    },
    {
      note: "itr: test 读到 test_ 上",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (entity::is_weapon_data(data) && is_nullish(field_or(itr, u"test"))) {`,
      to: `      if (entity::is_weapon_data(data) && is_nullish(field_or(itr, u"test_"))) {`,
    },
    {
      note: "itr: test 写到 test_ 上",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `        io->set(u"test", weapon_on_hand_dont_hit_falling_guy());`,
      to: `        io->set(u"test_", weapon_on_hand_dont_hit_falling_guy());`,
    },
    {
      note: "itr: test 写成常量 1",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `        io->set(u"test", weapon_on_hand_dont_hit_falling_guy());`,
      to: `        io->set(u"test", Value(1.0));`,
    },
    {
      note: "itr: 标量 itr 上挂 test 不再失败",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `        if (io == nullptr) return false;   // 给标量挂 \`test\` ⇒ TS 抛\n`,
      to: ``,
    },
    {
      note: "itr: 子 ctx 的键写成 itr_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      Value sub = spread_assign(make_obj({{u"itr", itr}}), ctx);`,
      to: `      Value sub = spread_assign(make_obj({{u"itr_", itr}}), ctx);`,
    },
    {
      note: "itr: preprocess_itr 的失败被忽略",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (!preprocess_itr(sub, error)) return false;`,
      to: `      if (false && !preprocess_itr(sub, error)) return false;`,
    },
    {
      note: "itr: 不再写回处理后的 itr",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      itr = field_or(sub, u"itr");\n`,
      to: ``,
    },
    {
      note: "itr: 写回时读错键",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      itr = field_or(sub, u"itr");`,
      to: `      itr = field_or(sub, u"itr_");`,
    },
    // ---------------------------------------------------------------- bdy_prefabs
    {
      note: "bdy: 表从 data.bdy_prefab 取（少个 s）",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  Value bdy_prefabs = field_or(data, u"bdy_prefabs");`,
      to: `  Value bdy_prefabs = field_or(data, u"bdy_prefab");`,
    },
    {
      note: "bdy: 不再跳过假值项",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (!truthy(bdy)) return true;\n`,
      to: ``,
    },
    {
      note: "bdy: 子 ctx 的键写成 bdy_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      Value sub = spread_assign(make_obj({{u"bdy", bdy}}), ctx);`,
      to: `      Value sub = spread_assign(make_obj({{u"bdy_", bdy}}), ctx);`,
    },
    {
      note: "bdy: preprocess_bdy 的失败被忽略",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (!preprocess_bdy(sub, error)) return false;`,
      to: `      if (false && !preprocess_bdy(sub, error)) return false;`,
    },
    {
      note: "bdy: 不再写回处理后的 bdy",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      bdy = field_or(sub, u"bdy");\n`,
      to: ``,
    },
    {
      note: "bdy: 写回时读错键",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      bdy = field_or(sub, u"bdy");`,
      to: `      bdy = field_or(sub, u"bdy_");`,
    },
    // ---------------------------------------------------------------- lfw / base / 图 / 音效
    {
      note: "ed: 不再检查 lfw 缺失",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_nullish(lfw)) return false;   // \`const { images, sounds } = lfw\` 抛\n`,
      to: ``,
    },
    {
      note: "ed: lfw 读成 lfw_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  const Value lfw = field_or(ctx, u"lfw");`,
      to: `  const Value lfw = field_or(ctx, u"lfw_");`,
    },
    {
      note: "ed: base 读成 base_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  const Value base = field_or(data, u"base");`,
      to: `  const Value base = field_or(data, u"base_");`,
    },
    {
      note: "ed: base 缺失时不失败",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_nullish(base)) return false;  // \`const { small, head } = data.base\` 抛\n`,
      to: ``,
    },
    {
      note: "ed: small 读成 head",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_non_blank_str(field_or(base, u"small")) && !push_job(jobs)) return false;`,
      to: `  if (is_non_blank_str(field_or(base, u"head")) && !push_job(jobs)) return false;`,
    },
    {
      note: "ed: small 的空串判断换成 truthy",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_non_blank_str(field_or(base, u"small")) && !push_job(jobs)) return false;`,
      to: `  if (truthy(field_or(base, u"small")) && !push_job(jobs)) return false;`,
    },
    {
      note: "ed: small 那行不再检查 jobs.push",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_non_blank_str(field_or(base, u"small")) && !push_job(jobs)) return false;`,
      to: `  if (is_non_blank_str(field_or(base, u"small"))) return false;`,
    },
    {
      note: "ed: head 读成 small",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_non_blank_str(field_or(base, u"head")) && !push_job(jobs)) return false;`,
      to: `  if (is_non_blank_str(field_or(base, u"small")) && !push_job(jobs)) return false;`,
    },
    {
      note: "ed: head 那行不再检查 jobs.push",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_non_blank_str(field_or(base, u"head")) && !push_job(jobs)) return false;`,
      to: `  if (is_non_blank_str(field_or(base, u"head"))) return false;`,
    },
    {
      note: "push_job: 永远返回 true",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `bool push_job(const Value& jobs) { return as_array(jobs) != nullptr; }`,
      to: `bool push_job(const Value& jobs) { (void)jobs; return true; }`,
    },
    {
      note: "push_job: 永远返回 false",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `bool push_job(const Value& jobs) { return as_array(jobs) != nullptr; }`,
      to: `bool push_job(const Value& jobs) { (void)jobs; return false; }`,
    },
    {
      note: "ed: dead_sounds 读成 drop_sounds",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (!each_sound(base, u"dead_sounds")) return false;`,
      to: `  if (!each_sound(base, u"drop_sounds")) return false;`,
    },
    {
      note: "ed: drop_sounds 读成 dead_sounds",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (!each_sound(base, u"drop_sounds")) return false;`,
      to: `  if (!each_sound(base, u"dead_sounds")) return false;`,
    },
    {
      note: "ed: hit_sounds 读成 drop_sounds",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (!each_sound(base, u"hit_sounds")) return false;`,
      to: `  if (!each_sound(base, u"drop_sounds")) return false;`,
    },
    {
      note: "each_sound: 空串 / 0 / false 也当 nullish 跳过",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_nullish(list)) return true;`,
      to: `  if (!truthy(list)) return true;`,
    },
    {
      note: "each_sound: 永远成功",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  return as_array(list) != nullptr;`,
      to: `  return true;`,
    },
    {
      note: "each_sound: 永远失败",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  return as_array(list) != nullptr;`,
      to: `  return false;`,
    },
    {
      note: "is_non_blank_str: 空白也算非空",
      file: "native/lfw/utils/type_check.h",
      from: `    if (!is_str_white_space(c)) return true;`,
      to: `    return true;`,
    },
    {
      note: "is_non_blank_str: 非字符串也算非空",
      file: "native/lfw/utils/type_check.h",
      from: `  if (text == nullptr) return false;`,
      to: `  if (text == nullptr) return true;`,
    },
    // ---------------------------------------------------------------- pre / post hitkeys
    {
      note: "hk: __pre_hitkeys_map 从 post_hitkeys 建",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      !build_hitkeys_map(data, u"pre_hitkeys", u"__pre_hitkeys_map"))`,
      to: `      !build_hitkeys_map(data, u"post_hitkeys", u"__pre_hitkeys_map"))`,
    },
    {
      note: "hk: pre_hitkeys 的结果挂到 __post_hitkeys_map",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      !build_hitkeys_map(data, u"pre_hitkeys", u"__pre_hitkeys_map"))`,
      to: `      !build_hitkeys_map(data, u"pre_hitkeys", u"__post_hitkeys_map"))`,
    },
    {
      note: "hk: post_hitkeys 的结果挂到 __pre_hitkeys_map",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      !build_hitkeys_map(data, u"post_hitkeys", u"__post_hitkeys_map"))`,
      to: `      !build_hitkeys_map(data, u"post_hitkeys", u"__pre_hitkeys_map"))`,
    },
    {
      note: "hk: post_hitkeys 的门用 pre_hitkeys",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (truthy(field_or(data, u"post_hitkeys")) &&`,
      to: `  if (truthy(field_or(data, u"pre_hitkeys")) &&`,
    },
    {
      note: "hk: map 的输入键写死 pre_hitkeys",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  const Value holder = field_or(data, key);`,
      to: `  const Value holder = field_or(data, u"pre_hitkeys");`,
    },
    {
      note: "hk: 不再特判字符串（走 traversal_write 的非空字符串失败）",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (const std::u16string* const text = std::get_if<std::u16string>(&holder)) {`,
      to: `  if (const std::u16string* const text = nullptr) {`,
    },
    {
      note: "hk: 字符串路径不看键长",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (k.size() < 2 || k[0] == k[1]) continue;`,
      to: `      if (k[0] == k[1]) continue;`,
    },
    {
      note: "hk: 字符串路径的 k[0] == k[1] 反过来",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (k.size() < 2 || k[0] == k[1]) continue;`,
      to: `      if (k.size() < 2 || k[0] != k[1]) continue;`,
    },
    {
      note: "hk: 字符串路径的值写成空",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      Value v(std::u16string(1, (*text)[i]));`,
      to: `      Value v;`,
    },
    {
      note: "hk: 字符串路径不再失败",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      return false;   // \`o[k] = nf\` 给字符串赋值 ⇒ TS 抛`,
      to: `      return true;`,
    },
    {
      note: "hk: 对象路径不再跳过假值",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    if (!truthy(v)) return true;\n`,
      to: ``,
    },
    {
      note: "hk: 对象路径的键长门放宽到 0",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    if (k.size() < 2) return true;`,
      to: `    if (k.size() < 1) return true;`,
    },
    {
      note: "hk: 对象路径不再跳过 k[0] == k[1]",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    if (k[0] == k[1]) return true;\n`,
      to: ``,
    },
    {
      note: "hk: 对象路径的 k[0] == k[1] 反过来",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    if (k[0] == k[1]) return true;`,
      to: `    if (k[0] != k[1]) return true;`,
    },
    {
      note: "hk: 对象路径忽略 preprocess_next_frame 失败",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    if (!preprocess_next_frame(v)) return false;\n    as_mut(map)->set(k, v);`,
      to: `    (void)preprocess_next_frame(v);\n    as_mut(map)->set(k, v);`,
    },
    {
      note: "hk: 不再往 map 里塞",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    as_mut(map)->set(k, v);\n`,
      to: ``,
    },
    {
      note: "hk: 往 map 里塞固定键",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    as_mut(map)->set(k, v);`,
      to: `    as_mut(map)->set(u"k", v);`,
    },
    {
      note: "hk: any 永远为 false（空 map 也当有）",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    any = true;\n`,
      to: ``,
    },
    {
      note: "hk: 空 map 也挂上",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (any) as_mut(data)->set(std::u16string(out_key), map);`,
      to: `  if (true) as_mut(data)->set(std::u16string(out_key), map);`,
    },
    {
      note: "hk: 遍历失败被忽略",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (!ok) return false;\n  if (any)`,
      to: `  if (any)`,
    },
    {
      note: "hk: 挂的键写死成 __pre_hitkeys_map",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (any) as_mut(data)->set(std::u16string(out_key), map);`,
      to: `  if (any) as_mut(data)->set(u"__pre_hitkeys_map", map);`,
    },
    // ---------------------------------------------------------------- on_dead / on_exhaustion
    {
      note: "on: on_dead 的门读成 on_dead_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (truthy(field_or(data, u"on_dead"))) {`,
      to: `  if (truthy(field_or(data, u"on_dead_"))) {`,
    },
    {
      note: "on: on_dead 的值读成 on_dead_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    Value nf = field_or(data, u"on_dead");`,
      to: `    Value nf = field_or(data, u"on_dead_");`,
    },
    {
      note: "on: on_dead 的 next_frame 失败被忽略",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    Value nf = field_or(data, u"on_dead");\n    if (!preprocess_next_frame(nf)) return false;`,
      to: `    Value nf = field_or(data, u"on_dead");\n    (void)preprocess_next_frame(nf);`,
    },
    {
      note: "on: on_dead 的写回键写成 on_dead_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    as_mut(data)->set(u"on_dead", nf);`,
      to: `    as_mut(data)->set(u"on_dead_", nf);`,
    },
    {
      note: "on: on_exhaustion 的门读成 on_dead",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (truthy(field_or(data, u"on_exhaustion"))) {`,
      to: `  if (truthy(field_or(data, u"on_dead"))) {`,
    },
    {
      note: "on: on_exhaustion 的值读成 on_exhaustion_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    Value nf = field_or(data, u"on_exhaustion");`,
      to: `    Value nf = field_or(data, u"on_exhaustion_");`,
    },
    {
      note: "on: on_exhaustion 的 next_frame 失败被忽略",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    Value nf = field_or(data, u"on_exhaustion");\n    if (!preprocess_next_frame(nf)) return false;`,
      to: `    Value nf = field_or(data, u"on_exhaustion");\n    (void)preprocess_next_frame(nf);`,
    },
    {
      note: "on: on_exhaustion 的写回键写成 on_exhaustion_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    as_mut(data)->set(u"on_exhaustion", nf);`,
      to: `    as_mut(data)->set(u"on_exhaustion_", nf);`,
    },
    // ---------------------------------------------------------------- files / jobs
    {
      note: "files: 读成 files_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  const Value files = field_or(base, u"files");`,
      to: `  const Value files = field_or(base, u"files_");`,
    },
    {
      note: "files: push 失败不再记下来",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (!push_job(jobs)) push_failed = true;`,
      to: `      (void)push_job(jobs);`,
    },
    {
      note: "files: push_failed 不置位",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (!push_job(jobs)) push_failed = true;`,
      to: `      if (false) push_failed = true;`,
    },
    {
      note: "files: push 失败不再返回",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    if (push_failed) return false;\n`,
      to: ``,
    },
    {
      note: "jobs: 缺失时不再失败",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_nullish(jobs)) return false;\n`,
      to: ``,
    },
    {
      note: "traversal: 值版不再给字符串下标",
      file: "native/lfw/utils/container_help/traversal.h",
      from: `  if (const std::u16string* const text = std::get_if<std::u16string>(&obj)) {`,
      to: `  if (const std::u16string* const text = nullptr) {`,
    },
    {
      note: "jobs: 不管 length 真假都查可迭代",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (truthy(value_length(jobs)) && as_array(jobs) == nullptr && !is_str(jobs)) return false;`,
      to: `  if (as_array(jobs) == nullptr && !is_str(jobs)) return false;`,
    },
    {
      note: "jobs: 可迭代判断丢掉字符串",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (truthy(value_length(jobs)) && as_array(jobs) == nullptr && !is_str(jobs)) return false;`,
      to: `  if (truthy(value_length(jobs)) && as_array(jobs) == nullptr) return false;`,
    },
    {
      note: "jobs: 可迭代判断丢掉数组",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (truthy(value_length(jobs)) && as_array(jobs) == nullptr && !is_str(jobs)) return false;`,
      to: `  if (truthy(value_length(jobs)) && !is_str(jobs)) return false;`,
    },
    {
      note: "length: 对象的 length 读成 len",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (as_object(v) != nullptr) return field_or(v, u"length");`,
      to: `  if (as_object(v) != nullptr) return field_or(v, u"len");`,
    },
    {
      note: "length: 数组的 length 恒为 0",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (const Array* const a = as_array(v)) return Value(static_cast<double>(a->size()));`,
      to: `  if (const Array* const a = as_array(v)) return Value(0.0);`,
    },
    // ---------------------------------------------------------------- portraits / frames
    {
      note: "portraits: 读成 portraits_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  const Value portraits = field_or(base, u"portraits");`,
      to: `  const Value portraits = field_or(base, u"portraits_");`,
    },
    {
      note: "portraits: 不再调 preprocess_pic",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      v = preprocess_pic(v);`,
      to: `      (void)v;`,
    },
    {
      note: "portraits: 改用 preprocess_frame_pic",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      v = preprocess_pic(v);`,
      to: `      v = preprocess_frame_pic(v);`,
    },
    {
      note: "portraits: 遍历失败被忽略",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      v = preprocess_pic(v);\n      return true;\n    });\n    if (!ok) return false;`,
      to: `      (void)preprocess_pic(v);\n      return true;\n    });`,
    },
    {
      note: "frames: 读成 frames_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  const Value frames = field_or(data, u"frames");`,
      to: `  const Value frames = field_or(data, u"frames_");`,
    },
    {
      note: "frames: pics 从合并后的帧读（不是回调参数）",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      const Value pics = value_length(field_or(raw_frame, u"pics"));`,
      to: `      const Value pics = value_length(field_or(frame, u"pics"));`,
    },
    {
      note: "frames: pics 读成 pics_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      const Value pics = value_length(field_or(raw_frame, u"pics"));`,
      to: `      const Value pics = value_length(field_or(raw_frame, u"pics_"));`,
    },
    {
      note: "frames: 子 ctx 的键写成 frame_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      Value sub = spread_assign(ctx, make_obj({{u"frame", raw_frame}}));`,
      to: `      Value sub = spread_assign(ctx, make_obj({{u"frame_", raw_frame}}));`,
    },
    {
      note: "frames: preprocess_frame 的失败被忽略",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (!preprocess_frame(sub, error)) return false;`,
      to: `      if (false && !preprocess_frame(sub, error)) return false;`,
    },
    {
      note: "frames: 不再写回处理后的帧",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      frame = field_or(sub, u"frame");\n`,
      to: ``,
    },
    {
      note: "frames: 写回时读错键",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      frame = field_or(sub, u"frame");`,
      to: `      frame = field_or(sub, u"frame_");`,
    },
    {
      note: "frames: 不再累加 __pics",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      if (truthy(pics)) {`,
      to: `      if (false) {`,
    },
    {
      note: "frames: __pics 读成 __pic",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `        const Value prev = field_or(data, u"__pics");`,
      to: `        const Value prev = field_or(data, u"__pic");`,
    },
    {
      note: "frames: prev 不再 || 0",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `        const double b = to_number(truthy(prev) ? prev : Value(0.0));`,
      to: `        const double b = to_number(prev);`,
    },
    {
      note: "frames: __pics 只取 pics",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `        as_mut(data)->set(u"__pics", Value(max(a, b)));`,
      to: `        as_mut(data)->set(u"__pics", Value(a));`,
    },
    {
      note: "frames: __pics 只取旧值",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `        as_mut(data)->set(u"__pics", Value(max(a, b)));`,
      to: `        as_mut(data)->set(u"__pics", Value(b));`,
    },
    {
      note: "frames: __pics 写成 __pics_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `        as_mut(data)->set(u"__pics", Value(max(a, b)));`,
      to: `        as_mut(data)->set(u"__pics_", Value(max(a, b)));`,
    },
    {
      note: "frames: 遍历失败被忽略",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `      return true;\n    });\n    if (!ok) return false;\n  }\n\n  if (truthy(field_or(base, u"bot"))) {`,
      to: `      return true;\n    });\n  }\n\n  if (truthy(field_or(base, u"bot"))) {`,
    },
    // ---------------------------------------------------------------- bot / processed / errors
    {
      note: "bot: 门读成 bot_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (truthy(field_or(base, u"bot"))) {`,
      to: `  if (truthy(field_or(base, u"bot_"))) {`,
    },
    {
      note: "bot: 的值读成 bot_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    Value bot = field_or(base, u"bot");`,
      to: `    Value bot = field_or(base, u"bot_");`,
    },
    {
      note: "bot: preprocess_bot_data 的失败被忽略",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `    if (!preprocess_bot_data(bot)) return false;`,
      to: `    (void)preprocess_bot_data(bot);`,
    },
    {
      note: "ed: processed 写成 false",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  as_mut(data)->set(u"processed", Value(true));`,
      to: `  as_mut(data)->set(u"processed", Value(false));`,
    },
    {
      note: "ed: processed 写成加工过的键",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  as_mut(data)->set(u"processed", Value(true));`,
      to: `  as_mut(data)->set(u"processed_", Value(true));`,
    },
    {
      note: "ed: processed 写成 1",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  as_mut(data)->set(u"processed", Value(true));`,
      to: `  as_mut(data)->set(u"processed", Value(1.0));`,
    },
    {
      note: "ed: errors 缺失时不再失败",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_nullish(field_or(ctx, u"errors"))) return false;\n`,
      to: ``,
    },
    {
      note: "ed: errors 读成 errors_",
      file: "native/lfw/loader/preprocess_entity_data.cpp",
      from: `  if (is_nullish(field_or(ctx, u"errors"))) return false;`,
      to: `  if (is_nullish(field_or(ctx, u"errors_"))) return false;`,
    },
  ],
};
