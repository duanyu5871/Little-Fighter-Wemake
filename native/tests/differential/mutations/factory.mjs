// `Factory`（`src/LFW/Factory.ts`）。
//
// 用例：`cases/factory/all.txt`（`regent` / `regctrl` / `regbuff` / `ce` / `cebot` / `acq-e` /
// `rec-e` / `newctrl` / `acq-ctrl` / `rel-ctrl` / `cbuff` / `rec-buff` / `dump`）。
//
// 有意不覆盖（不可观察、按构造等价或无法在台面上造出来）：
//   * `create_entity_with_player` / `create_components` / `register_component`：前者要
//     `LocalController`（依赖 `LFW.player()`，随 `LFW` 那刀）、后者是 UI 层（README 里明确
//     `ui/` 最后搬）；
//   * `components` / `_usedALIAS` 两张表：`register_component` 与它们一起推迟，且 `_usedALIAS`
//     全仓库无人使用；
//   * `FactoryKey` 不建模 `symbol`（端口的 `Value` 只有字符串 / 数字 / 布尔 / nullish / 对象），
//     也不建模「`NaN` 当键」（JS `Map` 的 SameValueZero 会把两个 `NaN` 当同一个键，端口的
//     `strict_equals` 不是）；
//   * `map_set` / `index_of` 的返回索引：表里不会出现重复键（`map_set` 自己保证），
//     「取第一个」与「取最后一个」等价；
//   * `create_buff` 里 `b->create(lfw, id, b->kind())` 的第三个参数写成查表用的 `kind`：
//     `register_buff` 就是按 `KIND` 建索引的，两者恒等；
//   * `create_entity` / `create_entity_with_bot` 的 `states` 实参：台面两侧都不传；
//   * `acquire_ctrl` 的模板 / 返回类型与 `release_ctrl` 的链式返回：语言层差异，不可观察；
//   * harness 层的脚本 op 与回显行：那是台面自己的输出。
export default {
  subject: "factory",
  cases: ["all"],
  mutations: [
    // ---------------------------------------------------------------- 注册表：重复告警
    {
      note: "Factory: register_entity 不报重复",
      file: "native/lfw/factory.cpp",
      from: `  if (index_of(entity_creators(), type) != static_cast<size_t>(-1)) {`,
      to: `  if (false) {`,
    },
    {
      note: "Factory: register_ctrl 不报重复",
      file: "native/lfw/factory.cpp",
      from: `  if (index_of(ctrl_creators(), oid) != static_cast<size_t>(-1)) {`,
      to: `  if (false) {`,
    },
    {
      note: "Factory: register_buff 不报重复",
      file: "native/lfw/factory.cpp",
      from: `  if (index_of(buff_creators(), kind) != static_cast<size_t>(-1)) {`,
      to: `  if (false) {`,
    },
    {
      note: "Factory: warn 整个不生效",
      file: "native/lfw/factory.cpp",
      from: `  if (g_warn) g_warn(text);`,
      to: `  if (false) g_warn(text);`,
    },
    {
      note: "Factory: register_entity 的告警文案改了",
      file: "native/lfw/factory.cpp",
      from: `    warn(u"[Factory::register_entity] type already exists, " + to_string(type));`,
      to: `    warn(u"[Factory::register_entity] type exists, " + to_string(type));`,
    },
    {
      note: "Factory: register_ctrl 的告警文案改了",
      file: "native/lfw/factory.cpp",
      from: `    warn(u"[Factory::register_ctrl] oid already exists, " + to_string(oid));`,
      to: `    warn(u"[Factory::register_ctrl] oid already exists " + to_string(oid));`,
    },
    {
      note: "Factory: register_buff 的告警文案丢了 kind",
      file: "native/lfw/factory.cpp",
      from: `    warn(u"[Factory::register_buff] kind already exists, " + to_string(kind));`,
      to: `    warn(u"[Factory::register_buff] kind already exists, ");`,
    },

    // ---------------------------------------------------------------- 表的语义：覆盖 / 插入序 / 键相等 / Set 去重
    {
      note: "Factory: map_set 覆盖时追加新条目（键会重复）",
      file: "native/lfw/factory.cpp",
      from: `  list[i].second = value;`,
      to: `  list.push_back(std::make_pair(key, value));`,
    },
    {
      note: "Factory: map_set 插入新键时放到最前（插入序反了）",
      file: "native/lfw/factory.cpp",
      from: `    list.push_back(std::make_pair(key, value));`,
      to: `    list.insert(list.begin(), std::make_pair(key, value));`,
    },
    {
      note: "Factory: 键相等用宽松比较（`s \"1\"` 与 `n 1` 会撞在一起）",
      file: "native/lfw/factory.cpp",
      from: `    if (strict_equals(list[i].first, key)) return i;`,
      to: `    if (equals(list[i].first, key)) return i;`,
    },
    {
      note: "Factory: 控制器表的指针比较恒假（永远找不到）",
      file: "native/lfw/factory.cpp",
      from: `    if (list[i].first == key) return i;`,
      to: `    if (false) return i;`,
    },
    {
      note: "Factory: Set 追加不去重（分组里出现重复 kind）",
      file: "native/lfw/factory.cpp",
      from: `  for (const FactoryKey& v : list) {
    if (strict_equals(v, key)) return;
  }
  list.push_back(key);`,
      to: `  list.push_back(key);`,
    },
    {
      note: "Factory: Set 追加到最前（分组内顺序反了）",
      file: "native/lfw/factory.cpp",
      from: `  list.push_back(key);`,
      to: `  list.insert(list.begin(), key);`,
    },
    {
      note: "Factory: buff 分组表用 kind 当键（而不是分组名）",
      file: "native/lfw/factory.cpp",
      from: `    const size_t i = index_of(buff_groups(), g);`,
      to: `    const size_t i = index_of(buff_groups(), kind);`,
    },
    {
      note: "Factory: 新建分组时不登记 kind",
      file: "native/lfw/factory.cpp",
      from: `      set_add(buff_groups().back().second, kind);`,
      to: `      (void)0;`,
    },
    {
      note: "Factory: 已有分组时不登记 kind",
      file: "native/lfw/factory.cpp",
      from: `      set_add(buff_groups()[i].second, kind);`,
      to: `      (void)i;`,
    },

    // ---------------------------------------------------------------- create_buff
    {
      note: "Factory: create_buff 的「没登记」判断反了",
      file: "native/lfw/factory.cpp",
      from: `  if (ci == static_cast<size_t>(-1)) return nullptr;`,
      to: `  if (ci != static_cast<size_t>(-1)) return nullptr;`,
    },
    {
      note: "Factory: create_buff 从不取池里的实例（每次新建）",
      file: "native/lfw/factory.cpp",
      from: `  const size_t pi = index_of(buff_graves_maps, kind);`,
      to: `  const size_t pi = static_cast<size_t>(-1);`,
    },
    {
      note: "Factory: create_buff 复用时不 reset（id 还是旧的）",
      file: "native/lfw/factory.cpp",
      from: `  ret->reset(id);`,
      to: `  (void)id;`,
    },
    {
      note: "Factory: create_buff 不 init",
      file: "native/lfw/factory.cpp",
      from: `  ret->init();`,
      to: `  (void)0;`,
    },
    {
      note: "Factory: recycle_buff 的池键写死 undefined",
      file: "native/lfw/factory.cpp",
      from: `  pool_of(buff_graves_maps, buff->kind()).add(buff);`,
      to: `  pool_of(buff_graves_maps, Value()).add(buff);`,
    },

    // ---------------------------------------------------------------- 实体池 / 实体创建
    {
      note: "Factory: recycle_entity 的池键用 id（而不是 data.type）",
      file: "native/lfw/factory.cpp",
      from: `  pool_of(graves_maps, field_or(e->data(), u"type")).add(e);`,
      to: `  pool_of(graves_maps, field_or(e->data(), u"id")).add(e);`,
    },
    {
      note: "Factory: pool_of 找不到时返回第一张池（跨键污染）",
      file: "native/lfw/factory.cpp",
      from: `  list.push_back(std::make_pair(key, Graves<T>()));
  return list.back().second;`,
      to: `  list.push_back(std::make_pair(key, Graves<T>()));
  return list.front().second;`,
    },
    {
      note: "Factory: acquire_entity 的池键写死 undefined",
      file: "native/lfw/factory.cpp",
      from: `  const size_t i = index_of(graves_maps, type);`,
      to: `  const size_t i = index_of(graves_maps, Value());`,
    },
    {
      note: "Factory: acquire_entity 的「没有这条池」判断反了",
      file: "native/lfw/factory.cpp",
      from: `  if (i == static_cast<size_t>(-1)) return nullptr;
  const std::optional<Entity*> taken = graves_maps[i].second.take();`,
      to: `  if (i != static_cast<size_t>(-1)) return nullptr;
  const std::optional<Entity*> taken = graves_maps[i].second.take();`,
    },
    {
      note: "Factory: create_entity 用 data.id 查 creator",
      file: "native/lfw/factory.cpp",
      from: `  const size_t i = index_of(entity_creators(), field_or(data, u"type"));
  if (i == static_cast<size_t>(-1)) return nullptr;
  return entity_creators()[i].second(world, data, states);`,
      to: `  const size_t i = index_of(entity_creators(), field_or(data, u"id"));
  if (i == static_cast<size_t>(-1)) return nullptr;
  return entity_creators()[i].second(world, data, states);`,
    },
    {
      note: "Factory: create_entity_with_bot 的 creator 查找改用 data.id",
      file: "native/lfw/factory.cpp",
      from: `  const size_t i = index_of(entity_creators(), field_or(data, u"type"));
  if (i == static_cast<size_t>(-1)) return nullptr;
  Entity* const ret = entity_creators()[i].second(world, data, states);
  if (ret == nullptr) return ret;
  ret->set_ctrl(create_ctrl(field_or(data, u"id"), player_id, ret));`,
      to: `  const size_t i = index_of(entity_creators(), field_or(data, u"id"));
  if (i == static_cast<size_t>(-1)) return nullptr;
  Entity* const ret = entity_creators()[i].second(world, data, states);
  if (ret == nullptr) return ret;
  ret->set_ctrl(create_ctrl(field_or(data, u"id"), player_id, ret));`,
    },
    {
      note: "Factory: create_entity_with_bot 的「creator 没给出实体」判断反了",
      file: "native/lfw/factory.cpp",
      from: `  Entity* const ret = entity_creators()[i].second(world, data, states);
  if (ret == nullptr) return ret;
  ret->set_ctrl(create_ctrl(field_or(data, u"id"), player_id, ret));`,
      to: `  Entity* const ret = entity_creators()[i].second(world, data, states);
  if (ret != nullptr) return ret;
  ret->set_ctrl(create_ctrl(field_or(data, u"id"), player_id, ret));`,
    },
    {
      note: "Factory: create_entity_with_bot 用 data.type 查控制器（而不是 data.id）",
      file: "native/lfw/factory.cpp",
      from: `  ret->set_ctrl(create_ctrl(field_or(data, u"id"), player_id, ret));`,
      to: `  ret->set_ctrl(create_ctrl(field_or(data, u"type"), player_id, ret));`,
    },

    // ---------------------------------------------------------------- 控制器
    {
      note: "Factory: create_ctrl 的「没登记」判断反了",
      file: "native/lfw/factory.cpp",
      from: `  if (i == static_cast<size_t>(-1)) return nullptr;
  return acquire_ctrl(ctrl_creators()[i].second, player_id, entity);`,
      to: `  if (i != static_cast<size_t>(-1)) return nullptr;
  return acquire_ctrl(ctrl_creators()[i].second, player_id, entity);`,
    },
    {
      note: "Factory: acquire_ctrl 从不取池里的实例（每次新建）",
      file: "native/lfw/factory.cpp",
      from: `  const size_t i = index_of(ctrl_graves_maps, cls);`,
      to: `  const size_t i = index_of(ctrl_graves_maps, nullptr);`,
    },
    {
      note: "Factory: acquire_ctrl 复用时不写 player_id",
      file: "native/lfw/factory.cpp",
      from: `      ret->player_id = player_id;`,
      to: `      (void)player_id;`,
    },
    {
      note: "Factory: acquire_ctrl 复用时不 reset",
      file: "native/lfw/factory.cpp",
      from: `      ret->reset();`,
      to: `      (void)0;`,
    },
    {
      note: "Factory: 新建控制器时不记 creator（归池键会丢）",
      file: "native/lfw/factory.cpp",
      from: `  ret->set_creator(cls);`,
      to: `  (void)cls;`,
    },
    {
      note: "Factory: release_ctrl 的池键写死 nullptr",
      file: "native/lfw/factory.cpp",
      from: `  pool_of(ctrl_graves_maps, ctrl->creator()).add(ctrl);`,
      to: `  pool_of(ctrl_graves_maps, static_cast<const ICtrlCreator*>(nullptr)).add(ctrl);`,
    },
  ],
};
