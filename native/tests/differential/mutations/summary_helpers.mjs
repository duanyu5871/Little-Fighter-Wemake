export default {
  subject: "summary_helpers",
  mutations: [
    {
      note: "js_add 不做 ToPrimitive（a 侧）",
      file: "native/lfw/utils/js_add.cpp",
      from: `  const Value pa = to_primitive(a);`,
      to: `  const Value pa = a;`,
    },
    {
      note: "js_add 不做 ToPrimitive（b 侧）",
      file: "native/lfw/utils/js_add.cpp",
      from: `  const Value pb = to_primitive(b);`,
      to: `  const Value pb = b;`,
    },
    {
      note: "js_add 的字符串判定只看 a",
      file: "native/lfw/utils/js_add.cpp",
      from: `  if (std::holds_alternative<std::u16string>(pa) || std::holds_alternative<std::u16string>(pb)) {`,
      to: `  if (std::holds_alternative<std::u16string>(pa)) {`,
    },
    {
      note: "js_add 的字符串判定只看 b",
      file: "native/lfw/utils/js_add.cpp",
      from: `  if (std::holds_alternative<std::u16string>(pa) || std::holds_alternative<std::u16string>(pb)) {`,
      to: `  if (std::holds_alternative<std::u16string>(pb)) {`,
    },
    {
      note: "js_add 的字符串判定用 &&",
      file: "native/lfw/utils/js_add.cpp",
      from: `  if (std::holds_alternative<std::u16string>(pa) || std::holds_alternative<std::u16string>(pb)) {`,
      to: `  if (std::holds_alternative<std::u16string>(pa) && std::holds_alternative<std::u16string>(pb)) {`,
    },
    {
      note: "js_add 丢掉字符串分支",
      file: "native/lfw/utils/js_add.cpp",
      from: `  if (std::holds_alternative<std::u16string>(pa) || std::holds_alternative<std::u16string>(pb)) {
    return Value(to_string(pa) + to_string(pb));
  }`,
      to: `  if (false) {
    return Value(to_string(pa) + to_string(pb));
  }`,
    },
    {
      note: "js_add 的字符串拼接顺序反了",
      file: "native/lfw/utils/js_add.cpp",
      from: `    return Value(to_string(pa) + to_string(pb));`,
      to: `    return Value(to_string(pb) + to_string(pa));`,
    },
    {
      note: "Summary.set_damage_sum 没有相同值守卫",
      file: "native/lfw/entity/summary.cpp",
      from: `  const Value o = _damage_sum;
  if (equals(o, v)) return;`,
      to: `  const Value o = _damage_sum;`,
    },
    {
      note: "Summary.set_damage_sum 的守卫取反",
      file: "native/lfw/entity/summary.cpp",
      from: `  const Value o = _damage_sum;
  if (equals(o, v)) return;`,
      to: `  const Value o = _damage_sum;
  if (!equals(o, v)) return;`,
    },
    {
      note: "Summary.set_damage_sum 的守卫用严相等",
      file: "native/lfw/entity/summary.cpp",
      from: `  const Value o = _damage_sum;
  if (equals(o, v)) return;`,
      to: `  const Value o = _damage_sum;
  if (strict_equals(o, v)) return;`,
    },
    {
      note: "Summary.set_damage_sum 存了旧值",
      file: "native/lfw/entity/summary.cpp",
      from: `  _damage_sum = v;
  callbacks.call(u"on_damage_sum_changed", {v, o, Value()});`,
      to: `  _damage_sum = o;
  callbacks.call(u"on_damage_sum_changed", {v, o, Value()});`,
    },
    {
      note: "Summary.set_damage_sum 触发错事件名",
      file: "native/lfw/entity/summary.cpp",
      from: `  callbacks.call(u"on_damage_sum_changed", {v, o, Value()});`,
      to: `  callbacks.call(u"on_kill_sum_changed", {v, o, Value()});`,
    },
    {
      note: "Summary.set_damage_sum 回调参数顺序反了",
      file: "native/lfw/entity/summary.cpp",
      from: `  callbacks.call(u"on_damage_sum_changed", {v, o, Value()});`,
      to: `  callbacks.call(u"on_damage_sum_changed", {o, v, Value()});`,
    },
    {
      note: "Summary.set_kill_sum 没有相同值守卫",
      file: "native/lfw/entity/summary.cpp",
      from: `  const Value o = _kill_sum;
  if (equals(o, v)) return;`,
      to: `  const Value o = _kill_sum;`,
    },
    {
      note: "Summary.set_picking_sum 没有相同值守卫",
      file: "native/lfw/entity/summary.cpp",
      from: `  const Value o = _picking_sum;
  if (equals(o, v)) return;`,
      to: `  const Value o = _picking_sum;`,
    },
    {
      note: "Summary.set_hp_lost 没有相同值守卫",
      file: "native/lfw/entity/summary.cpp",
      from: `  const Value o = _hp_lost;
  if (equals(o, v)) return;`,
      to: `  const Value o = _hp_lost;`,
    },
    {
      note: "Summary.set_mp_usage 没有相同值守卫",
      file: "native/lfw/entity/summary.cpp",
      from: `  const Value o = _mp_usage;
  if (equals(o, v)) return;`,
      to: `  const Value o = _mp_usage;`,
    },
    {
      note: "Summary.set_kill_sum 的字段写错",
      file: "native/lfw/entity/summary.cpp",
      from: `  _kill_sum = v;
  callbacks.call(u"on_kill_sum_changed", {v, o, Value()});`,
      to: `  _damage_sum = v;
  callbacks.call(u"on_kill_sum_changed", {v, o, Value()});`,
    },
    {
      note: "Summary.set_mp_usage 的字段写错",
      file: "native/lfw/entity/summary.cpp",
      from: `  _mp_usage = v;
  callbacks.call(u"on_mp_usage_changed", {v, o, Value()});`,
      to: `  _hp_lost = v;
  callbacks.call(u"on_mp_usage_changed", {v, o, Value()});`,
    },
    {
      note: "Summary.set_kill_sum 触发错事件名",
      file: "native/lfw/entity/summary.cpp",
      from: `  callbacks.call(u"on_kill_sum_changed", {v, o, Value()});`,
      to: `  callbacks.call(u"on_damage_sum_changed", {v, o, Value()});`,
    },
    {
      note: "Summary.reset 漏清 kill_sum",
      file: "native/lfw/entity/summary.cpp",
      from: `  _damage_sum = Value(0.0);
  _kill_sum = Value(0.0);`,
      to: `  _damage_sum = Value(0.0);`,
    },
    {
      note: "Summary.reset 漏清 mp_usage",
      file: "native/lfw/entity/summary.cpp",
      from: `  _hp_lost = Value(0.0);
  _mp_usage = Value(0.0);`,
      to: `  _hp_lost = Value(0.0);`,
    },
    {
      note: "Summary.reset 漏清 hp_lost",
      file: "native/lfw/entity/summary.cpp",
      from: `  _hp_lost = Value(0.0);
  _mp_usage = Value(0.0);`,
      to: `  _mp_usage = Value(0.0);`,
    },
    {
      note: "Summary.reset 不写 id",
      file: "native/lfw/entity/summary.cpp",
      from: `void Summary::reset(const std::u16string& id) {
  _id = id;`,
      to: `void Summary::reset(const std::u16string& id) {
  (void)id;`,
    },
    {
      note: "Summary.release 不清回调",
      file: "native/lfw/entity/summary.cpp",
      from: `void Summary::release() {
  callbacks.clear();
  reset(u"");`,
      to: `void Summary::release() {
  reset(u"");`,
    },
    {
      note: "Summary.release 不重置",
      file: "native/lfw/entity/summary.cpp",
      from: `void Summary::release() {
  callbacks.clear();
  reset(u"");`,
      to: `void Summary::release() {
  callbacks.clear();`,
    },
    {
      note: "SummaryMgr.get 不查已有条目",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  std::shared_ptr<Summary>* found = find_item(id);
  if (found != nullptr) return *found;`,
      to: `  std::shared_ptr<Summary>* found = find_item(id);
  (void)found;`,
    },
    {
      note: "SummaryMgr.get 不登记新条目",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  std::shared_ptr<Summary> made = acquire(id);
  _items.push_back(std::make_pair(id, made));
  return made;`,
      to: `  return acquire(id);`,
    },
    {
      note: "SummaryMgr.get 不走墓碑复用",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  std::shared_ptr<Summary> made = acquire(id);`,
      to: `  std::shared_ptr<Summary> made = std::make_shared<Summary>(id);`,
    },
    {
      note: "SummaryMgr.clear 什么都不做",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  std::vector<std::u16string> keys;
  for (const auto& kv : _items) keys.push_back(kv.first);
  for (const std::u16string& k : keys) release(k);`,
      to: `  std::vector<std::u16string> keys;
  for (const auto& kv : _items) keys.push_back(kv.first);`,
    },
    {
      note: "SummaryMgr.clear 只删条目不进墓碑",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  for (const std::u16string& k : keys) release(k);`,
      to: `  for (const std::u16string& k : keys) del_item(k);`,
    },
    {
      note: "SummaryMgr.release 不调用 Summary.release",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  std::shared_ptr<Summary> item = *found;
  item->release();
  del_item(id);`,
      to: `  std::shared_ptr<Summary> item = *found;
  del_item(id);`,
    },
    {
      note: "SummaryMgr.release 不从条目表删除",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  std::shared_ptr<Summary> item = *found;
  item->release();
  del_item(id);
  _graves.push_back(item);`,
      to: `  std::shared_ptr<Summary> item = *found;
  item->release();
  _graves.push_back(item);`,
    },
    {
      note: "SummaryMgr.release 不进墓碑",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  std::shared_ptr<Summary> item = *found;
  item->release();
  del_item(id);
  _graves.push_back(item);`,
      to: `  std::shared_ptr<Summary> item = *found;
  item->release();
  del_item(id);`,
    },
    {
      note: "SummaryMgr.acquire 不复用墓碑",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  if (!_graves.empty()) {`,
      to: `  if (false) {`,
    },
    {
      note: "SummaryMgr.acquire 复用时不 reset",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `    std::shared_ptr<Summary> ret = _graves.back();
    _graves.pop_back();
    ret->reset(id);
    return ret;`,
      to: `    std::shared_ptr<Summary> ret = _graves.back();
    _graves.pop_back();
    return ret;`,
    },
    {
      note: "SummaryMgr.del_item 删错条目",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `    if (_items[i].first == id) {`,
      to: `    if (_items[i].first != id) {`,
    },
    {
      note: "add_damage_sum 直接赋值而不是累加",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  self->set_damage_sum(js_add(self->damage_sum(), value));`,
      to: `  self->set_damage_sum(value);`,
    },
    {
      note: "add_damage_sum 的队伍读成 id",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  const std::u16string team = to_string(field_or(a, u"team"));
  if (is_independent(team)) {
    std::shared_ptr<Summary> team_sum = get(team);
    team_sum->set_damage_sum(js_add(team_sum->damage_sum(), value));`,
      to: `  const std::u16string team = to_string(field_or(a, u"id"));
  if (is_independent(team)) {
    std::shared_ptr<Summary> team_sum = get(team);
    team_sum->set_damage_sum(js_add(team_sum->damage_sum(), value));`,
    },
    {
      note: "add_damage_sum 不做队伍扇出",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  const std::u16string team = to_string(field_or(a, u"team"));
  if (is_independent(team)) {
    std::shared_ptr<Summary> team_sum = get(team);
    team_sum->set_damage_sum(js_add(team_sum->damage_sum(), value));
  }`,
      to: `  const std::u16string team = to_string(field_or(a, u"team"));
  if (false) {
    std::shared_ptr<Summary> team_sum = get(team);
    team_sum->set_damage_sum(js_add(team_sum->damage_sum(), value));
  }`,
    },
    {
      note: "add_kill_sum 的默认值写成 0",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  const Value amount = std::holds_alternative<std::monostate>(value) ? Value(1.0) : value;`,
      to: `  const Value amount = std::holds_alternative<std::monostate>(value) ? Value(0.0) : value;`,
    },
    {
      note: "add_kill_sum 的默认值对 null 也生效",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  const Value amount = std::holds_alternative<std::monostate>(value) ? Value(1.0) : value;`,
      to: `  const Value amount =
      (std::holds_alternative<std::monostate>(value) || std::holds_alternative<NullTag>(value))
          ? Value(1.0)
          : value;`,
    },
    {
      note: "add_kill_sum 直接赋值而不是累加",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  self->set_kill_sum(js_add(self->kill_sum(), amount));`,
      to: `  self->set_kill_sum(amount);`,
    },
    {
      note: "add_kill_sum 不做队伍扇出",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  const std::u16string team = to_string(field_or(a, u"team"));
  if (is_independent(team)) {
    std::shared_ptr<Summary> team_sum = get(team);
    team_sum->set_kill_sum(js_add(team_sum->kill_sum(), amount));
  }`,
      to: `  const std::u16string team = to_string(field_or(a, u"team"));
  if (false) {
    std::shared_ptr<Summary> team_sum = get(team);
    team_sum->set_kill_sum(js_add(team_sum->kill_sum(), amount));
  }`,
    },
    {
      note: "apply_damage 忽略 injury",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  add_damage_sum(a, injury);`,
      to: `  add_damage_sum(a, Value(0.0));`,
    },
    {
      note: "apply_damage 的击杀算到自己而不是单例",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `    summary_mgr().add_kill_sum(a);`,
      to: `    add_kill_sum(a);`,
    },
    {
      note: "apply_damage 不判 fighter",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  if (entity::is_fighter(v) && emitters_length(v) == 0 &&`,
      to: `  if (emitters_length(v) == 0 &&`,
    },
    {
      note: "apply_damage 的 emitters 判定取反",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  if (entity::is_fighter(v) && emitters_length(v) == 0 &&`,
      to: `  if (entity::is_fighter(v) && emitters_length(v) != 0 &&`,
    },
    {
      note: "apply_damage 的 fighter 判定用 ||",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `  if (entity::is_fighter(v) && emitters_length(v) == 0 &&`,
      to: `  if (entity::is_fighter(v) || emitters_length(v) == 0 &&`,
    },
    {
      note: "apply_damage 的 hp <= 0 用 <",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `      le(field_or(v, u"hp"), Value(0.0)) && gt(prev_hp, Value(0.0))) {`,
      to: `      lt(field_or(v, u"hp"), Value(0.0)) && gt(prev_hp, Value(0.0))) {`,
    },
    {
      note: "apply_damage 的 prev_hp > 0 用 >=",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `      le(field_or(v, u"hp"), Value(0.0)) && gt(prev_hp, Value(0.0))) {`,
      to: `      le(field_or(v, u"hp"), Value(0.0)) && ge(prev_hp, Value(0.0))) {`,
    },
    {
      note: "apply_damage 的 prev_hp > 0 用 <",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `      le(field_or(v, u"hp"), Value(0.0)) && gt(prev_hp, Value(0.0))) {`,
      to: `      le(field_or(v, u"hp"), Value(0.0)) && lt(prev_hp, Value(0.0))) {`,
    },
    {
      note: "apply_damage 的 hp 读成 prev_hp",
      file: "native/lfw/entity/summary_mgr.cpp",
      from: `      le(field_or(v, u"hp"), Value(0.0)) && gt(prev_hp, Value(0.0))) {`,
      to: `      le(prev_hp, Value(0.0)) && gt(prev_hp, Value(0.0))) {`,
    },
    {
      note: "is_independent 用 size() > 1",
      file: "native/lfw/defines/team_enum.h",
      from: `inline bool is_independent(const std::u16string& team) { return team.size() != 1; }`,
      to: `inline bool is_independent(const std::u16string& team) { return team.size() > 1; }`,
    },
    {
      note: "is_independent 用 size() != 0",
      file: "native/lfw/defines/team_enum.h",
      from: `inline bool is_independent(const std::u16string& team) { return team.size() != 1; }`,
      to: `inline bool is_independent(const std::u16string& team) { return team.size() != 0; }`,
    },
    {
      note: "is_independent 用 empty()",
      file: "native/lfw/defines/team_enum.h",
      from: `inline bool is_independent(const std::u16string& team) { return team.size() != 1; }`,
      to: `inline bool is_independent(const std::u16string& team) { return team.empty(); }`,
    },
    {
      note: "is_independent 用 size() != 3",
      file: "native/lfw/defines/team_enum.h",
      from: `inline bool is_independent(const std::u16string& team) { return team.size() != 1; }`,
      to: `inline bool is_independent(const std::u16string& team) { return team.size() != 3; }`,
    },
  ],
};
