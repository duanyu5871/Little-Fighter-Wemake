// `PlayerInfo`（`src/LFW/PlayerInfo.ts`）+ 顺带两处共享文件的小改：
// `core/js_string.h` 的 `to_lower_case`（`String.prototype.toLowerCase`）与
// `defines/defines.cpp` 的 `get_default_keys_value`（`Defines.get_default_keys` 的原始返回）。
//
// 用例：`cases/player_info/all.txt`（`cache_*` / `new` / `dump` / `reload` / `save` / `set*` / `getkey`）。
//
// 有意不覆盖（不可观察、按构造等价或无法在台面上造出来）：
//   * `prop_get` 的 `is_nullish(o)` 分支与 `prop_set` 的**数组分支 / 非对象失败分支**：
//     `_info.keys` 一定是构造函数写进去的共享键表（对象）、`load` 从不替换它，而 payload 里的
//     `keys`（字符串 / 数组）只在 `for...in` 里按下标读 ⇒ 这两条写路径在本 subject 里造不出来
//     （端口按 JS 属性语义保留，字段表来自 TS 数据，理论上仍可能给出非对象）；
//   * `prop_get` 的**字符串 / 数组 `length` 分支**：同上，`_info.keys` 是对象，payload 的
//     `keys` 只按下标读；
//   * `prop_set` 的 `length` 上限 `1000000` 与「数组上的字符串键」：TS 允许 1e9 这种超长稀疏
//     数组、也允许给数组挂字符串键，端口的 `Array`（`std::vector`）装不下 ⇒ 记在偏差表里，
//     用例只碰 ≤ 1e6 的下标/长度；
//   * `for_in_keys` 分不出数组的「洞」（端口的数组元素一律是显式 `undefined`）：用例不让
//     `for...in` 去遍历带洞的数组；
//   * `load_impl` 里 `if (truthy(keys_v))` 这个门：`for_in_keys` 对假值本来就不给键 ⇒ 等价；
//   * `load_impl` 里 `if (!strict_equals(ctrl_v, ctrl()))` 这个门：`set_ctrl` 自己会短路 ⇒ 等价；
//   * `load_impl` 里 `if (!parsed.ok)` 去掉后与「`parsed.value` 是 `undefined` ⇒ 解构失败」同一条
//     warn（`load failed, reason`）⇒ 按构造等价；
//   * `save` 里 `if (!text.has_value())` 分支：本刀的数据 `json_stringify` 不会失败；
//   * `save` 里 `put.type = kDataType` 换成 `kTag`：两者文本都是 `"PlayerInfo"`；
//   * `get_default_keys_value` 的 `truthy(*exact)` 写成 `exact != nullptr`：表里只有 `'_'` 一个键
//     且它是真值，`player_id` 又都不在表里 ⇒ 两条路一个结果；
//   * `to_lower_case` 表外码点（本刀只覆盖 ASCII / Latin-1 / Latin Extended-A / 希腊 / 西里尔，
//     见 README 偏差表）与 `String.prototype.toLowerCase` 的完整 Unicode 表；
//   * harness 层的 `cache_*` 脚本与 `dump`/`getkey` 回显行：那是台面自己的输出。
export default {
  subject: "player_info",
  cases: ["all"],
  mutations: [
    // ---------------------------------------------------------------- 构造函数与挂起的 load
    {
      note: "PlayerInfo: local 默认值写成 false",
      file: "native/lfw/player_info.cpp",
      from: `  if (is_undefined(_local)) _local = Value(true);`,
      to: `  if (is_undefined(_local)) _local = Value(false);`,
    },
    {
      note: "PlayerInfo: local 不套默认值",
      file: "native/lfw/player_info.cpp",
      from: `  if (is_undefined(_local)) _local = Value(true);`,
      to: `  if (false) _local = Value(true);`,
    },
    {
      note: "PlayerInfo: mine 默认值写成 false",
      file: "native/lfw/player_info.cpp",
      from: `  if (is_undefined(_mine)) _mine = Value(true);`,
      to: `  if (is_undefined(_mine)) _mine = Value(false);`,
    },
    {
      note: "PlayerInfo: name 不套默认值（id）",
      file: "native/lfw/player_info.cpp",
      from: `  if (is_undefined(name_v)) name_v = Value(id);`,
      to: `  if (false) name_v = Value(id);`,
    },
    {
      note: "PlayerInfo: name 默认值写死 'x'",
      file: "native/lfw/player_info.cpp",
      from: `  if (is_undefined(name_v)) name_v = Value(id);`,
      to: `  if (is_undefined(name_v)) name_v = Value(u"x");`,
    },
    {
      note: "PlayerInfo: _info.id 用 name 填",
      file: "native/lfw/player_info.cpp",
      from: `  info->set(u"id", Value(id));`,
      to: `  info->set(u"id", name_v);`,
    },
    {
      note: "PlayerInfo: keys 不取默认键表",
      file: "native/lfw/player_info.cpp",
      from: `  info->set(u"keys", defines::get_default_keys_value(id));`,
      to: `  info->set(u"keys", Value());`,
    },
    {
      note: "PlayerInfo: version 初值写成 1",
      file: "native/lfw/player_info.cpp",
      from: `  info->set(u"version", Value(0.0));`,
      to: `  info->set(u"version", Value(1.0));`,
    },
    {
      note: "PlayerInfo: ctrl 初值写成 Gamepad_1",
      file: "native/lfw/player_info.cpp",
      from: `  info->set(u"ctrl", Value(static_cast<double>(CtrlDevice::Keyboard)));`,
      to: `  info->set(u"ctrl", Value(static_cast<double>(CtrlDevice::Gamepad_1)));`,
    },
    {
      note: "PlayerInfo: 构造函数里直接落地 load（不挂起）",
      file: "native/lfw/player_info.cpp",
      from: `  // TS 的 \`this.loaded = this.load()\`：那次 load 真正生效在第一个 \`await\` 之后 ⇒ 挂起。
  _load_pending = true;`,
      to: `  // TS 的 \`this.loaded = this.load()\`：那次 load 真正生效在第一个 \`await\` 之后 ⇒ 挂起。
  _loaded = load_impl();
  _load_pending = false;`,
    },
    {
      note: "PlayerInfo: flush_pending_load 不动作",
      file: "native/lfw/player_info.cpp",
      from: `  if (!_load_pending) return;
  _load_pending = false;
  _loaded = load_impl();`,
      to: `  if (!_load_pending) return;
  _load_pending = false;`,
    },
    {
      note: "PlayerInfo: storage_key 少了分隔下划线",
      file: "native/lfw/player_info.cpp",
      from: `std::u16string PlayerInfo::storage_key() const { return u"player_info_" + id(); }`,
      to: `std::u16string PlayerInfo::storage_key() const { return u"player_info" + id(); }`,
    },
    {
      note: "PlayerInfo: name() 读成 keys",
      file: "native/lfw/player_info.cpp",
      from: `Value PlayerInfo::name() const { return field(u"name"); }`,
      to: `Value PlayerInfo::name() const { return field(u"keys"); }`,
    },
    {
      note: "PlayerInfo: keys() 读成 ctrl",
      file: "native/lfw/player_info.cpp",
      from: `Value PlayerInfo::keys() const { return field(u"keys"); }`,
      to: `Value PlayerInfo::keys() const { return field(u"ctrl"); }`,
    },
    {
      note: "PlayerInfo: ctrl() 读成 version",
      file: "native/lfw/player_info.cpp",
      from: `Value PlayerInfo::ctrl() const { return field(u"ctrl"); }`,
      to: `Value PlayerInfo::ctrl() const { return field(u"version"); }`,
    },
    {
      note: "PlayerInfo: id() 读成 name",
      file: "native/lfw/player_info.cpp",
      from: `  const Value v = field(u"id");`,
      to: `  const Value v = field(u"name");`,
    },
    // ---------------------------------------------------------------- set_name / set_ctrl / set_is_com
    {
      note: "PlayerInfo: set_name 不短路",
      file: "native/lfw/player_info.cpp",
      from: `  if (strict_equals(prev, name)) return;
  set_field(u"name", name);`,
      to: `  set_field(u"name", name);`,
    },
    {
      note: "PlayerInfo: set_name 写错字段名",
      file: "native/lfw/player_info.cpp",
      from: `  set_field(u"name", name);`,
      to: `  set_field(u"name2", name);`,
    },
    {
      note: "PlayerInfo: on_name_changed 两个参数反了",
      file: "native/lfw/player_info.cpp",
      from: `  if (emit) callbacks.call(u"on_name_changed", {name, prev});`,
      to: `  if (emit) callbacks.call(u"on_name_changed", {prev, name});`,
    },
    {
      note: "PlayerInfo: set_name 从不发回调",
      file: "native/lfw/player_info.cpp",
      from: `  if (emit) callbacks.call(u"on_name_changed", {name, prev});`,
      to: `  if (false) callbacks.call(u"on_name_changed", {name, prev});`,
    },
    {
      note: "PlayerInfo: set_ctrl 不短路",
      file: "native/lfw/player_info.cpp",
      from: `  if (strict_equals(prev, ctrl)) return;
  set_field(u"ctrl", ctrl);`,
      to: `  set_field(u"ctrl", ctrl);`,
    },
    {
      note: "PlayerInfo: set_ctrl 的 prev 取成 ctrl",
      file: "native/lfw/player_info.cpp",
      from: `  const Value prev = field(u"ctrl");`,
      to: `  const Value prev = ctrl;`,
    },
    {
      note: "PlayerInfo: on_ctrl_changed 两个参数反了",
      file: "native/lfw/player_info.cpp",
      from: `  if (emit) callbacks.call(u"on_ctrl_changed", {ctrl, prev});`,
      to: `  if (emit) callbacks.call(u"on_ctrl_changed", {prev, ctrl});`,
    },
    {
      note: "PlayerInfo: set_is_com 不短路",
      file: "native/lfw/player_info.cpp",
      from: `  if (_is_com == is_com) return;`,
      to: `  if (false) return;`,
    },
    {
      note: "PlayerInfo: set_is_com 存反",
      file: "native/lfw/player_info.cpp",
      from: `  _is_com = is_com;`,
      to: `  _is_com = !is_com;`,
    },
    {
      note: "PlayerInfo: on_is_com_changed 参数取反",
      file: "native/lfw/player_info.cpp",
      from: `  if (emit) callbacks.call(u"on_is_com_changed", {Value(is_com)});`,
      to: `  if (emit) callbacks.call(u"on_is_com_changed", {Value(!is_com)});`,
    },
    // ---------------------------------------------------------------- set_key / get_key
    {
      note: "PlayerInfo: set_key 不短路",
      file: "native/lfw/player_info.cpp",
      from: `  if (strict_equals(prev, key)) return true;`,
      to: `  if (false) return true;`,
    },
    {
      note: "PlayerInfo: set_key 不做 toLowerCase",
      file: "native/lfw/player_info.cpp",
      from: `  const Value lowered(std::u16string(to_lower_case(*k)));`,
      to: `  const Value lowered{std::u16string(*k)};`,
    },
    {
      note: "PlayerInfo: set_key 写回原值而不是小写值",
      file: "native/lfw/player_info.cpp",
      from: `  if (!prop_set(keys_v, name, lowered, error)) return false;`,
      to: `  if (!prop_set(keys_v, name, key, error)) return false;`,
    },
    {
      note: "PlayerInfo: set_key 不检查 key 是不是字符串",
      file: "native/lfw/player_info.cpp",
      from: `  const std::u16string* const k = std::get_if<std::u16string>(&key);
  if (k == nullptr) {
    error = u"key.toLowerCase is not a function";
    return false;
  }`,
      to: `  const std::u16string* const k = std::get_if<std::u16string>(&key);
  if (k == nullptr) {
    if (!prop_set(keys_v, name, key, error)) return false;
    if (emit) callbacks.call(u"on_key_changed", {name, key, prev});
    return true;
  }`,
    },
    {
      note: "PlayerInfo: on_key_changed 的参数顺序换错",
      file: "native/lfw/player_info.cpp",
      from: `  if (emit) callbacks.call(u"on_key_changed", {name, lowered, prev});`,
      to: `  if (emit) callbacks.call(u"on_key_changed", {name, prev, lowered});`,
    },
    {
      note: "PlayerInfo: get_key 的键取成 undefined",
      file: "native/lfw/player_info.cpp",
      from: `  return prop_get(keys(), name, out, error);`,
      to: `  return prop_get(keys(), Value(), out, error);`,
    },
    // ---------------------------------------------------------------- prop_get / prop_set / for_in_keys
    {
      note: "prop_get: 字符串下标一律当没有",
      file: "native/lfw/player_info.cpp",
      from: `      out = idx < s->size() ? Value(std::u16string(1, (*s)[idx])) : Value();`,
      to: `      out = Value();`,
    },
    {
      note: "prop_get: 数组下标一律读第 0 个",
      file: "native/lfw/player_info.cpp",
      from: `      out = idx < arr->size() ? arr->at(idx) : Value();`,
      to: `      out = idx < arr->size() ? arr->at(0) : Value();`,
    },
    {
      note: "for_in_keys: 对象分支返回空",
      file: "native/lfw/player_info.cpp",
      from: `  if (const Object* obj = as_object(v)) return obj->keys();`,
      to: `  if (const Object* obj = as_object(v)) return {};`,
    },
    {
      note: "for_in_keys: 数组分支不给键",
      file: "native/lfw/player_info.cpp",
      from: `  if (const Array* arr = as_array(v)) {
    for (size_t i = 0; i < arr->size(); ++i) out.push_back(number_to_string(static_cast<double>(i)));
    return out;
  }`,
      to: `  if (const Array* arr = as_array(v)) return out;`,
    },
    {
      note: "for_in_keys: 字符串分支不给键",
      file: "native/lfw/player_info.cpp",
      from: `  if (const std::u16string* s = std::get_if<std::u16string>(&v)) {
    for (size_t i = 0; i < s->size(); ++i) out.push_back(number_to_string(static_cast<double>(i)));
  }`,
      to: `  if (const std::u16string* s = std::get_if<std::u16string>(&v)) return out;`,
    },
    {
      note: "destructure: 默认值也套给 null",
      file: "native/lfw/player_info.cpp",
      from: `  if (is_undefined(out) && fallback != nullptr) out = *fallback;`,
      to: `  if (is_nullish(out) && fallback != nullptr) out = *fallback;`,
    },
    // ---------------------------------------------------------------- load
    {
      note: "load: 不看 local 直接查缓存",
      file: "native/lfw/player_info.cpp",
      from: `  if (!truthy(_local)) return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "load: get 抛时报错的文本换成通用那条",
      file: "native/lfw/player_info.cpp",
      from: `    warn(u"[PlayerInfo::load] failed to load, reason");`,
      to: `    warn(u"[PlayerInfo::load] load failed, reason");`,
    },
    {
      note: "load: 没有这条缓存也往下走",
      file: "native/lfw/player_info.cpp",
      from: `  if (cache.missing) return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "load: no data 的文本变了",
      file: "native/lfw/player_info.cpp",
      from: `      warn(u"[PlayerInfo::load] no data");`,
      to: `      warn(u"[PlayerInfo::load] no_data");`,
    },
    {
      note: "load: blob 抛的文本变了",
      file: "native/lfw/player_info.cpp",
      from: `      warn(u"[PlayerInfo::load] read blob failed, reason: ");`,
      to: `      warn(u"[PlayerInfo::load] read blob failed");`,
    },
    {
      note: "load: data 是假值时不看 blob 直接报 no data",
      file: "native/lfw/player_info.cpp",
      from: `    if (cache.blob_kind == PlayerInfoCacheEntry::BlobKind::kFalsy) {`,
      to: `    if (cache.blob_kind != PlayerInfoCacheEntry::BlobKind::kBytes) {`,
    },
    {
      note: "load: data 非字节时不报错，当成没有 data",
      file: "native/lfw/player_info.cpp",
      from: `  } else if (cache.data_kind == PlayerInfoCacheEntry::DataKind::kOther) {
    // 真值但不是 \`Uint8Array\` ⇒ \`decodeUTF8(data)\` 在下面那个 try 里抛。
    warn(u"[PlayerInfo::load] load failed, reason");
    return false;
  } else {`,
      to: `  } else if (false) {
    warn(u"[PlayerInfo::load] load failed, reason");
    return false;
  } else {`,
    },
    {
      note: "load: 解构 name 用错键名",
      file: "native/lfw/player_info.cpp",
      from: `  if (!destructure(raw_info, Value(u"name"), nullptr, name_v, error) ||`,
      to: `  if (!destructure(raw_info, Value(u"names"), nullptr, name_v, error) ||`,
    },
    {
      note: "load: 解构 keys 用错键名",
      file: "native/lfw/player_info.cpp",
      from: `      !destructure(raw_info, Value(u"keys"), nullptr, keys_v, error) ||`,
      to: `      !destructure(raw_info, Value(u"key"), nullptr, keys_v, error) ||`,
    },
    {
      note: "load: 解构 ctrl 用错键名",
      file: "native/lfw/player_info.cpp",
      from: `      !destructure(raw_info, Value(u"ctrl"), &ctrl_default, ctrl_v, error) ||`,
      to: `      !destructure(raw_info, Value(u"ctrls"), &ctrl_default, ctrl_v, error) ||`,
    },
    {
      note: "load: ctrl 不给默认值",
      file: "native/lfw/player_info.cpp",
      from: `      !destructure(raw_info, Value(u"ctrl"), &ctrl_default, ctrl_v, error) ||`,
      to: `      !destructure(raw_info, Value(u"ctrl"), nullptr, ctrl_v, error) ||`,
    },
    {
      note: "load: ctrl 的默认值取成固定 0",
      file: "native/lfw/player_info.cpp",
      from: `  const Value ctrl_default = ctrl();`,
      to: `  const Value ctrl_default = Value(0.0);`,
    },
    {
      note: "load: 解构 version 用错键名",
      file: "native/lfw/player_info.cpp",
      from: `      !destructure(raw_info, Value(u"version"), nullptr, version_v, error)) {`,
      to: `      !destructure(raw_info, Value(u"versions"), nullptr, version_v, error)) {`,
    },
    {
      note: "load: 版本门去掉",
      file: "native/lfw/player_info.cpp",
      from: `  if (!strict_equals(version_v, field(u"version"))) {
    warn(u"[PlayerInfo::load] version changed");
    return false;
  }`,
      to: `  if (false) {
    warn(u"[PlayerInfo::load] version changed");
    return false;
  }`,
    },
    {
      note: "load: 版本门改成宽松相等",
      file: "native/lfw/player_info.cpp",
      from: `  if (!strict_equals(version_v, field(u"version"))) {`,
      to: `  if (!equals(version_v, field(u"version"))) {`,
    },
    {
      note: "load: name 不是字符串也照样 set",
      file: "native/lfw/player_info.cpp",
      from: `  if (is_str(name_v)) set_name(name_v, true);`,
      to: `  set_name(name_v, true);`,
    },
    {
      note: "load: name 落地时不发回调",
      file: "native/lfw/player_info.cpp",
      from: `  if (is_str(name_v)) set_name(name_v, true);`,
      to: `  if (is_str(name_v)) set_name(name_v, false);`,
    },
    {
      note: "load: 遍历 keys 时读的是原始 payload 的对象",
      file: "native/lfw/player_info.cpp",
      from: `      if (!prop_get(keys_v, Value(k), kv, error)) {`,
      to: `      if (!prop_get(raw_info, Value(k), kv, error)) {`,
    },
    {
      note: "load: 遍历 keys 时不发回调",
      file: "native/lfw/player_info.cpp",
      from: `      if (!set_key(Value(k), kv, true, error)) {`,
      to: `      if (!set_key(Value(k), kv, false, error)) {`,
    },
    {
      note: "load: ctrl 落地时不发回调",
      file: "native/lfw/player_info.cpp",
      from: `  if (!strict_equals(ctrl_v, ctrl())) set_ctrl(ctrl_v, true);`,
      to: `  if (!strict_equals(ctrl_v, ctrl())) set_ctrl(ctrl_v, false);`,
    },
    {
      note: "load: 解构失败时不报警直接返回 true",
      file: "native/lfw/player_info.cpp",
      from: `      !destructure(raw_info, Value(u"version"), nullptr, version_v, error)) {
    warn(u"[PlayerInfo::load] load failed, reason");
    return false;
  }`,
      to: `      !destructure(raw_info, Value(u"version"), nullptr, version_v, error)) {
    return true;
  }`,
    },
    {
      note: "load: 版本变了但报的是通用文本",
      file: "native/lfw/player_info.cpp",
      from: `  if (!strict_equals(version_v, field(u"version"))) {
    warn(u"[PlayerInfo::load] version changed");`,
      to: `  if (!strict_equals(version_v, field(u"version"))) {
    warn(u"[PlayerInfo::load] load failed, reason");`,
    },
    // ---------------------------------------------------------------- save
    {
      note: "save: 不看 local 直接写",
      file: "native/lfw/player_info.cpp",
      from: `  if (!truthy(_local)) return;

  std::u16string error;`,
      to: `  std::u16string error;`,
    },
    {
      note: "save: del 失败也继续 put",
      file: "native/lfw/player_info.cpp",
      from: `  if (!_host->cache_del(storage_key(), error)) {
    warn(u"[PlayerInfo::load]");
    return;
  }`,
      to: `  if (!_host->cache_del(storage_key(), error)) {
    warn(u"[PlayerInfo::load]");
  }`,
    },
    {
      note: "save: del 失败的 warn tag 写成 save",
      file: "native/lfw/player_info.cpp",
      from: `  if (!_host->cache_del(storage_key(), error)) {
    warn(u"[PlayerInfo::load]");
    return;
  }`,
      to: `  if (!_host->cache_del(storage_key(), error)) {
    warn(u"[PlayerInfo::save]");
    return;
  }`,
    },
    {
      note: "save: put 的名字用 id 而不是 storage_key",
      file: "native/lfw/player_info.cpp",
      from: `  put.name = storage_key();`,
      to: `  put.name = id();`,
    },
    {
      note: "save: put 的 version 写成 2",
      file: "native/lfw/player_info.cpp",
      from: `  put.version = kDataVersion;`,
      to: `  put.version = 2;`,
    },
    {
      note: "save: 序列化的是 name 而不是整个 _info",
      file: "native/lfw/player_info.cpp",
      from: `  const std::optional<std::u16string> text = json_stringify(_info);`,
      to: `  const std::optional<std::u16string> text = json_stringify(field(u"name"));`,
    },
    {
      note: "save: 落盘的字节被截断",
      file: "native/lfw/player_info.cpp",
      from: `  put.data = encode_utf8(*text);`,
      to: `  put.data = encode_utf8(text->substr(0, 1));`,
    },
    {
      note: "save: 不写 put（只 del）",
      file: "native/lfw/player_info.cpp",
      from: `  _host->cache_put(put);`,
      to: `  (void)put;`,
    },
    // ---------------------------------------------------------------- to_lower_case
    {
      note: "to_lower_case: ASCII 映射少 1",
      file: "native/lfw/core/js_string.cpp",
      from: `    if (in_range(c, 0x41, 0x5a) ||                                   // A-Z
        (in_range(c, 0xc0, 0xde) && c != 0xd7) ||                    // À-Þ（除 ×）`,
      to: `    if (in_range(c, 0x41, 0x5a) ||                                   // A-Z
        (in_range(c, 0xbf, 0xdd) && c != 0xd7) ||                    // À-Þ（除 ×）`,
    },
    {
      note: "to_lower_case: 希腊/西里尔那一段一起映射",
      file: "native/lfw/core/js_string.cpp",
      from: `        in_range(c, 0x391, 0x3a1) || in_range(c, 0x3a3, 0x3ab) ||    // Α-Ρ / Σ-Ϋ
        in_range(c, 0x410, 0x42f)) {                                 // А-Я`,
      to: `        in_range(c, 0x391, 0x3a1) || in_range(c, 0x3a3, 0x3ab)) {    // Α-Ρ / Σ-Ϋ`,
    },
    {
      note: "to_lower_case: Ѐ-Џ 的偏移写成 +0x20",
      file: "native/lfw/core/js_string.cpp",
      from: `    if (in_range(c, 0x400, 0x40f)) {                                 // Ѐ-Џ
      out.push_back(static_cast<char16_t>(c + 0x50));`,
      to: `    if (in_range(c, 0x400, 0x40f)) {                                 // Ѐ-Џ
      out.push_back(static_cast<char16_t>(c + 0x20));`,
    },
    {
      note: "to_lower_case: 不做 İ 的特例",
      file: "native/lfw/core/js_string.cpp",
      from: `    if (c == 0x130) {                                                // İ ⇒ i + U+0307
      out.push_back(u'i');
      out.push_back(static_cast<char16_t>(0x307));
      continue;
    }`,
      to: `    if (false) {                                                // İ ⇒ i + U+0307
      out.push_back(u'i');
      out.push_back(static_cast<char16_t>(0x307));
      continue;
    }`,
    },
    {
      note: "to_lower_case: 不做 Ÿ 的特例",
      file: "native/lfw/core/js_string.cpp",
      from: `    if (c == 0x178) {                                                // Ÿ
      out.push_back(static_cast<char16_t>(0xff));
      continue;
    }`,
      to: `    if (false) {                                                // Ÿ
      out.push_back(static_cast<char16_t>(0xff));
      continue;
    }`,
    },
    {
      note: "to_lower_case: Latin Extended-A 的奇偶判反",
      file: "native/lfw/core/js_string.cpp",
      from: `    if ((in_range(c, 0x100, 0x137) || in_range(c, 0x14a, 0x177)) && even_case_pair(c)) {`,
      to: `    if ((in_range(c, 0x100, 0x137) || in_range(c, 0x14a, 0x177)) && !even_case_pair(c)) {`,
    },
    {
      note: "to_lower_case: Latin Extended-A 第二段的奇偶判反",
      file: "native/lfw/core/js_string.cpp",
      from: `    if ((in_range(c, 0x139, 0x148) || in_range(c, 0x179, 0x17e)) && !even_case_pair(c)) {`,
      to: `    if ((in_range(c, 0x139, 0x148) || in_range(c, 0x179, 0x17e)) && even_case_pair(c)) {`,
    },
    // ---------------------------------------------------------------- defines
    {
      note: "get_default_keys_value: 兜底键 '_' 换成 'x'",
      file: "native/lfw/defines/defines.cpp",
      from: `  if (exact != nullptr && truthy(*exact)) return *exact;
  const Value* fallback = o->get(u"_");`,
      to: `  if (exact != nullptr && truthy(*exact)) return *exact;
  const Value* fallback = o->get(u"x");`,
    },
  ],
};
