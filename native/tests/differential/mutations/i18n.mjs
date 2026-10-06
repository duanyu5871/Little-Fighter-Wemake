// `I18N`（`src/LFW/I18N.ts`）与 `loader/get_import_fallbacks`（`src/LFW/loader/get_import_fallbacks.ts`）。
//
// 用例：`cases/i18n/all.txt`（`gif` / `new` / `add` / `lang` / `alias` / `canonical` / `str` / `strs`）。
//
// 有意不覆盖（不可观察或按构造等价）：
//   * `add` 里 `if (!truthy(langs)) return;` 与 `if (!truthy(new_words)) continue;`：去掉后
//     `as_object` 对同一批假值返回 `nullptr`，走的是同一个「跳过」出口；
//   * `add` 里「先判字符串别名、再判真值」的顺序：唯一的区别是值为空串时算不算别名，而
//     `alias` 对空串别名本来就当「没有别名」（`if (!next) break`）⇒ 两种顺序结果一样；
//   * `alias` 里 `visited.push_back(ret)` 记的是 `ret` 还是 `lang`：只在原地打转时才会
//     命中环检测，而那种情况两种记法都在第二轮命中；
//   * `string` / `strings` 里 `alias(lang) ?? ''` 的 `''` 换成 `undefined`：`words_of` 对
//     `undefined` 与 `''` 的差别被基表兜住（`_words.get(undefined)` 与 `get('')` 都为
//     `undefined`），递归一步后结果相同；
//   * `[null] == ''` 这类数组转型：`loose_empty` 直接复用 `core/value.h` 的 `equals`，
//     没有另写的分支；
//   * `I18N` 构造函数里给 `_words['']` / `_lists['']` 种的空表：`add` 处理任何语言名（含
//     `''`）时都会补建这两张表，而 `_words.get(...)` 是 `undefined` 还是空表在
//     `?.[name] ?? fallback` 下不可区分；
//   * `split_path` 里 `if (begin > end) swap(...)` 的交换分支：`endsWith(suffix)` 保证最后一段
//     一定整个含住后缀（后缀里没有 `/`）⇒ `begin > end` 永不成立，去掉也不变；
//   * TS 的 `base_words` / `base_lists` 两个 getter：`src/LFW` 里没有使用者，端口没实现
//     （见 README 偏差表）；
//   * `get_import_fallbacks` 里图/音两个后缀组的先后：`endsWith` 不可能同时命中；
//   * harness 层的 `new` / `add` / `lang` 回显行：那是台面自己的输出，不是端口代码。
export default {
  subject: "i18n",
  cases: ["all"],
  mutations: [
    // ---------------------------------------------------------------- I18N：构造函数与三张表
    {
      note: "I18N: 基表 `_lists['']` 种成 undefined",
      file: "native/lfw/i18n.cpp",
      from: `  _lists[u""] = empty_object();`,
      to: `  _lists[u""] = Value();`,
    },
    {
      note: "I18N: words_of 恒返回 nullptr",
      file: "native/lfw/i18n.cpp",
      from: `  const auto it = _words.find(*k);
  return it == _words.end() ? nullptr : as_object(it->second);`,
      to: `  const auto it = _words.find(*k);
  return it == _words.end() ? nullptr : nullptr;`,
    },
    {
      note: "I18N: words_of 查成 _lists",
      file: "native/lfw/i18n.cpp",
      from: `  const auto it = _words.find(*k);
  return it == _words.end() ? nullptr : as_object(it->second);`,
      to: `  const auto it = _lists.find(*k);
  return it == _words.end() ? nullptr : as_object(it->second);`,
    },
    {
      note: "I18N: lists_of 恒返回 nullptr",
      file: "native/lfw/i18n.cpp",
      from: `  const auto it = _lists.find(*k);
  return it == _lists.end() ? nullptr : as_object(it->second);`,
      to: `  const auto it = _lists.find(*k);
  return it == _lists.end() ? nullptr : nullptr;`,
    },
    {
      note: "I18N: lists_of 查成 _words",
      file: "native/lfw/i18n.cpp",
      from: `  const auto it = _lists.find(*k);
  return it == _lists.end() ? nullptr : as_object(it->second);`,
      to: `  const auto it = _words.find(*k);
  return it == _lists.end() ? nullptr : as_object(it->second);`,
    },
    {
      note: "I18N: alias_of 恒返回 nullptr",
      file: "native/lfw/i18n.cpp",
      from: `  return it == _alias_map.end() ? nullptr : &it->second;`,
      to: `  return it == _alias_map.end() ? nullptr : nullptr;`,
    },
    {
      note: "I18N: set_lang 不写 _lang",
      file: "native/lfw/i18n.cpp",
      from: `  _lang = *text;
  return true;`,
      to: `  return true;`,
    },
    {
      note: "I18N: set_lang 写死 'zh'",
      file: "native/lfw/i18n.cpp",
      from: `  _lang = *text;`,
      to: `  _lang = u"zh";`,
    },
    {
      note: "I18N: set_lang 错误文本少了 'but' 前的空格",
      file: "native/lfw/i18n.cpp",
      from: `    error = u"[" + std::u16string(kTag) + u"::set_lang] lang should be string, but got " +
            to_string(lang);`,
      to: `    error = u"[" + std::u16string(kTag) + u"::set_lang] lang should be string, but got:" +
            to_string(lang);`,
    },
    {
      note: "I18N: set_lang 错误文本里的 tag 换成小写",
      file: "native/lfw/i18n.cpp",
      from: `    error = u"[" + std::u16string(kTag) + u"::set_lang] lang should be string, but got " +`,
      to: `    error = u"[i18n::set_lang] lang should be string, but got " +`,
    },
    {
      note: "I18N: set_lang 错误文本里用 'undefined' 代替值",
      file: "native/lfw/i18n.cpp",
      from: `            to_string(lang);`,
      to: `            to_string(Value());`,
    },
    // ---------------------------------------------------------------- I18N::add
    {
      note: "I18N: add 的字符串别名改成记成语言自己",
      file: "native/lfw/i18n.cpp",
      from: `      _alias_map[lang_name] = *alias;`,
      to: `      _alias_map[lang_name] = lang_name;`,
    },
    {
      note: "I18N: add 遇到非对象词图直接结束整个 add",
      file: "native/lfw/i18n.cpp",
      from: `    const Object* const word_map = as_object(new_words);
    if (word_map == nullptr) continue;   // 非对象 / 数组都跳过`,
      to: `    const Object* const word_map = as_object(new_words);
    if (word_map == nullptr) return;   // 非对象 / 数组都跳过`,
    },
    {
      note: "I18N: add 每次都重建词表",
      file: "native/lfw/i18n.cpp",
      from: `    auto words_it = _words.find(lang_name);
    if (words_it == _words.end()) words_it = _words.emplace(lang_name, empty_object()).first;`,
      to: `    _words[lang_name] = empty_object();
    auto words_it = _words.find(lang_name);`,
    },
    {
      note: "I18N: add 每次都重建列表",
      file: "native/lfw/i18n.cpp",
      from: `    auto lists_it = _lists.find(lang_name);
    if (lists_it == _lists.end()) lists_it = _lists.emplace(lang_name, empty_object()).first;`,
      to: `    _lists[lang_name] = empty_object();
    auto lists_it = _lists.find(lang_name);`,
    },
    {
      note: "I18N: add 把词表指针指到列表表",
      file: "native/lfw/i18n.cpp",
      from: `    Object* const strings = as_object(words_it->second);
    Object* const lists = as_object(lists_it->second);`,
      to: `    Object* const strings = as_object(lists_it->second);
    Object* const lists = as_object(lists_it->second);`,
    },
    {
      note: "I18N: add 的字符串词值多一个 '!'",
      file: "native/lfw/i18n.cpp",
      from: `      if (const std::u16string* const text = std::get_if<std::u16string>(&word)) {
        strings->set(k, Value(*text));`,
      to: `      if (const std::u16string* const text = std::get_if<std::u16string>(&word)) {
        strings->set(k, Value(*text + u"!"));`,
    },
    {
      note: "I18N: add 的字符串词列表写成空数组",
      file: "native/lfw/i18n.cpp",
      from: `        lists->set(k, Value(std::make_shared<Array>(one)));`,
      to: `        lists->set(k, Value(std::make_shared<Array>()));`,
    },
    {
      note: "I18N: add 的数组词用 ',' 连接",
      file: "native/lfw/i18n.cpp",
      from: `      strings->set(k, Value(join_lines(*arr)));`,
      to: `      strings->set(k, Value(array_join(*arr)));`,
    },
    {
      note: "I18N: add 的数组词列表写成空数组",
      file: "native/lfw/i18n.cpp",
      from: `      lists->set(k, Value(std::make_shared<Array>(items)));`,
      to: `      lists->set(k, Value(std::make_shared<Array>()));`,
    },
    {
      note: "I18N: add 的数组词列表不字符串化元素",
      file: "native/lfw/i18n.cpp",
      from: `      for (size_t i = 0; i < arr->size(); ++i) items.push_back(Value(to_string(arr->at(i))));`,
      to: `      for (size_t i = 0; i < arr->size(); ++i) items.push_back(arr->at(i));`,
    },
    {
      note: "join_lines: 分隔符用 ','",
      file: "native/lfw/i18n.cpp",
      from: `    if (i != 0) out.push_back(u'\\n');`,
      to: `    if (i != 0) out.push_back(u',');`,
    },
    {
      note: "join_lines: 不跳过 null/undefined",
      file: "native/lfw/i18n.cpp",
      from: `    if (is_nullish(item)) continue;
    out += to_string(item);`,
      to: `    out += to_string(item);`,
    },
    {
      note: "join_lines: 元素不字符串化",
      file: "native/lfw/i18n.cpp",
      from: `    out += to_string(item);`,
      to: `    out += u"?";`,
    },
    // ---------------------------------------------------------------- I18N::alias / canonical
    {
      note: "I18N: alias 成环时返回环上那个值",
      file: "native/lfw/i18n.cpp",
      from: `      if (strict_equals(seen, ret)) return Value();   // 成环 ⇒ undefined`,
      to: `      if (strict_equals(seen, ret)) return ret;   // 成环 ⇒ undefined`,
    },
    {
      note: "I18N: alias 把空串别名当成有效别名",
      file: "native/lfw/i18n.cpp",
      from: `    if (next == nullptr || next->empty()) break;`,
      to: `    if (next == nullptr) break;`,
    },
    {
      note: "I18N: alias 没走动时返回原值",
      file: "native/lfw/i18n.cpp",
      from: `  if (strict_equals(lang, ret)) return Value();   // 没走动过（\`lang == ret\`）⇒ undefined`,
      to: `  if (strict_equals(lang, ret)) return ret;   // 没走动过（\`lang == ret\`）⇒ undefined`,
    },
    {
      note: "I18N: canonical 的 ?? 两端反了",
      file: "native/lfw/i18n.cpp",
      from: `  return is_nullish(got) ? lang : got;`,
      to: `  return is_nullish(got) ? got : lang;`,
    },
    {
      note: "I18N: canonical 没别名时回 undefined",
      file: "native/lfw/i18n.cpp",
      from: `Value I18N::canonical(const Value& lang) const {
  const Value got = alias(lang);
  return is_nullish(got) ? lang : got;`,
      to: `Value I18N::canonical(const Value& lang) const {
  const Value got = alias(lang);
  return is_nullish(got) ? Value() : got;`,
    },
    {
      note: "I18N: canonical 不看别名表",
      file: "native/lfw/i18n.cpp",
      from: `Value I18N::canonical(const Value& lang) const {
  const Value got = alias(lang);`,
      to: `Value I18N::canonical(const Value& lang) const {
  const Value got = lang;`,
    },
    // ---------------------------------------------------------------- I18N::string / strings
    {
      note: "I18N: string 的 `lang == ''` 改成严格相等",
      file: "native/lfw/i18n.cpp",
      from: `  const Object* const words = words_of(lang);
  if (loose_empty(lang)) {`,
      to: `  const Object* const words = words_of(lang);
  if (strict_equals(lang, Value(std::u16string()))) {`,
    },
    {
      note: "I18N: string 基表分支的 ?? 两端反了",
      file: "native/lfw/i18n.cpp",
      from: `    const Value got = key_at(words, name);
    return is_nullish(got) ? name : got;`,
      to: `    const Value got = key_at(words, name);
    return is_nullish(got) ? got : name;`,
    },
    {
      note: "I18N: string 不再顺别名递归",
      file: "native/lfw/i18n.cpp",
      from: `  const Value got = key_at(words, name);
  if (!is_nullish(got)) return got;
  const Value a = alias(lang);
  return string(name, is_nullish(a) ? Value(std::u16string()) : a);`,
      to: `  const Value got = key_at(words, name);
  if (!is_nullish(got)) return got;
  const Value a = alias(lang);
  return string(name, Value(std::u16string()));`,
    },
    {
      note: "I18N: string 顺别名时递归进 strings",
      file: "native/lfw/i18n.cpp",
      from: `  const Value got = key_at(words, name);
  if (!is_nullish(got)) return got;
  const Value a = alias(lang);
  return string(name, is_nullish(a) ? Value(std::u16string()) : a);`,
      to: `  const Value got = key_at(words, name);
  if (!is_nullish(got)) return got;
  const Value a = alias(lang);
  return strings(name, is_nullish(a) ? Value(std::u16string()) : a);`,
    },
    {
      note: "I18N: strings 的列表表查成词表",
      file: "native/lfw/i18n.cpp",
      from: `  const Object* const lists = lists_of(lang);
  if (loose_empty(lang)) {`,
      to: `  const Object* const lists = words_of(lang);
  if (loose_empty(lang)) {`,
    },
    {
      note: "I18N: strings 基表分支的 ?? 两端反了",
      file: "native/lfw/i18n.cpp",
      from: `    return is_nullish(got) ? fallback_list(name) : got;`,
      to: `    return is_nullish(got) ? got : fallback_list(name);`,
    },
    {
      note: "I18N: strings 的 `?? [name]` 换成 `[undefined]`",
      file: "native/lfw/i18n.cpp",
      from: `Value fallback_list(const Value& name) {
  Array one;
  one.push_back(name);`,
      to: `Value fallback_list(const Value& name) {
  Array one;
  one.push_back(Value());`,
    },
    {
      note: "I18N: strings 不再顺别名递归",
      file: "native/lfw/i18n.cpp",
      from: `  const Value got = key_at(lists, name);
  if (!is_nullish(got)) return got;
  const Value a = alias(lang);
  return strings(name, is_nullish(a) ? Value(std::u16string()) : a);`,
      to: `  const Value got = key_at(lists, name);
  if (!is_nullish(got)) return got;
  const Value a = alias(lang);
  return strings(name, Value(std::u16string()));`,
    },
    {
      note: "I18N: strings 顺别名时递归进 string",
      file: "native/lfw/i18n.cpp",
      from: `  const Value got = key_at(lists, name);
  if (!is_nullish(got)) return got;
  const Value a = alias(lang);
  return strings(name, is_nullish(a) ? Value(std::u16string()) : a);`,
      to: `  const Value got = key_at(lists, name);
  if (!is_nullish(got)) return got;
  const Value a = alias(lang);
  return string(name, is_nullish(a) ? Value(std::u16string()) : a);`,
    },
    {
      note: "I18N: key_at 的键不做 ToString",
      file: "native/lfw/i18n.cpp",
      from: `  const Value* const found = obj->get(to_string(name));`,
      to: `  const Value* const found = obj->get(u"undefined");`,
    },
    {
      note: "I18N: key_at 命中也返回 undefined",
      file: "native/lfw/i18n.cpp",
      from: `  return found != nullptr ? *found : Value();`,
      to: `  return found != nullptr ? Value() : *found;`,
    },
    {
      note: "I18N: loose_empty 改用严格相等",
      file: "native/lfw/i18n.cpp",
      from: `bool loose_empty(const Value& v) { return equals(v, Value(std::u16string())); }`,
      to: `bool loose_empty(const Value& v) { return strict_equals(v, Value(std::u16string())); }`,
    },
    // ---------------------------------------------------------------- get_import_fallbacks
    {
      note: "gif: endsWith 的长度门槛放宽成 <=",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  if (path.size() < suffix.size() ||
      path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
    return false;`,
      to: `  if (path.size() <= suffix.size() ||
      path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
    return false;`,
    },
    {
      note: "gif: endsWith 的比较反了",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `      path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)`,
      to: `      path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0)`,
    },
    {
      note: "gif: 取第一个 '/' 而不是最后一个",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  const size_t slash = path.rfind(u'/');`,
      to: `  const size_t slash = path.find(u'/');`,
    },
    {
      note: "gif: 没有 '/' 时当成在开头切",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  const long long name_index = slash == std::u16string::npos ? -1 : static_cast<long long>(slash);`,
      to: `  const long long name_index = slash == std::u16string::npos ? 0 : static_cast<long long>(slash);`,
    },
    {
      note: "gif: dir_part 少算一个字符",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  dir_part = path.substr(0, static_cast<size_t>(name_index + 1));`,
      to: `  dir_part = path.substr(0, static_cast<size_t>(name_index));`,
    },
    {
      note: "gif: begin 少算一个字符",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  long long begin = name_index + 1;`,
      to: `  long long begin = name_index;`,
    },
    {
      note: "gif: name_part 的终点忘了扣后缀",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  long long end = static_cast<long long>(path.size() - suffix.size());`,
      to: `  long long end = static_cast<long long>(path.size());`,
    },
    {
      note: "gif: 图后缀组里去掉 .bmp",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  const char16_t* const image_suffixes[] = {u".png", u".bmp", u".webp"};`,
      to: `  const char16_t* const image_suffixes[] = {u".png", u".webp"};`,
    },
    {
      note: "gif: 图后缀组里去掉 .webp",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  const char16_t* const image_suffixes[] = {u".png", u".bmp", u".webp"};`,
      to: `  const char16_t* const image_suffixes[] = {u".png", u".bmp"};`,
    },
    {
      note: "gif: 命中图分支后上报的后缀写死 .png",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `    if (!split_path(*text, std::u16string(s), dir_part, name_part)) continue;
    suffix = std::u16string(s);
    // TS：\`fallbacks.unshift(...[...].filter(v => v !== name))\` ⇒ 新名在前、原名殿后。`,
      to: `    if (!split_path(*text, std::u16string(s), dir_part, name_part)) continue;
    suffix = u".png";
    // TS：\`fallbacks.unshift(...[...].filter(v => v !== name))\` ⇒ 新名在前、原名殿后。`,
    },
    {
      note: "gif: @4x 组里 webp 与 png 顺序反了",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `    static const char16_t* const kinds[] = {u"@4x.webp", u"@4x.png", u"@3x.webp", u"@3x.png",
                                            u"@2x.webp", u"@2x.png"};`,
      to: `    static const char16_t* const kinds[] = {u"@4x.png", u"@4x.webp", u"@3x.webp", u"@3x.png",
                                            u"@2x.webp", u"@2x.png"};`,
    },
    {
      note: "gif: 少了一档 @2x.webp",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `                                            u"@2x.webp", u"@2x.png"};`,
      to: `                                            u"@2x.png"};`,
    },
    {
      note: "gif: @3x.png 写成 @4x.png",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `    static const char16_t* const kinds[] = {u"@4x.webp", u"@4x.png", u"@3x.webp", u"@3x.png",
                                            u"@2x.webp", u"@2x.png"};`,
      to: `    static const char16_t* const kinds[] = {u"@4x.webp", u"@4x.png", u"@3x.webp", u"@4x.png",
                                            u"@2x.webp", u"@2x.png"};`,
    },
    {
      note: "gif: 目录版备选名的档位顺序反了",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `    for (const char16_t* const fold : {u"@4x", u"@3x", u"@2x"}) {`,
      to: `    for (const char16_t* const fold : {u"@3x", u"@4x", u"@2x"}) {`,
    },
    {
      note: "gif: 目录版备选名里 webp 写成 bmp",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `      push_if_new(fallbacks, *text, dir_part + fold + u"/" + name_part + u".webp");`,
      to: `      push_if_new(fallbacks, *text, dir_part + fold + u"/" + name_part + u".bmp");`,
    },
    {
      note: "gif: 目录版备选名的 png 写成 webp",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `      push_if_new(fallbacks, *text, dir_part + fold + u"/" + name_part + u".png");`,
      to: `      push_if_new(fallbacks, *text, dir_part + fold + u"/" + name_part + u".webp");`,
    },
    {
      note: "gif: 图分支丢掉最后的 .png 备选名",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `    push_if_new(fallbacks, *text, dir_part + name_part + u".webp");
    push_if_new(fallbacks, *text, dir_part + name_part + u".png");`,
      to: `    push_if_new(fallbacks, *text, dir_part + name_part + u".webp");`,
    },
    {
      note: "gif: 不过滤与原名相同的备选名",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  if (v != name) fallbacks.push_back(v);`,
      to: `  fallbacks.push_back(v);`,
    },
    {
      note: "gif: 音后缀组里去掉 .wma",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  const char16_t* const sound_suffixes[] = {u".wav", u".wma"};`,
      to: `  const char16_t* const sound_suffixes[] = {u".wav"};`,
    },
    {
      note: "gif: 音分支把 dir_part 拼了两遍",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `    fallbacks.push_back(dir_part + name_part + u".mp3");`,
      to: `    fallbacks.push_back(dir_part + dir_part + name_part + u".mp3");`,
    },
    {
      note: "gif: 音分支第二档备选名忘了带后缀",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `    fallbacks.push_back(dir_part + name_part + suffix + u".mp3");`,
      to: `    fallbacks.push_back(dir_part + name_part + u".mp3");`,
    },
    {
      note: "gif: 音分支上报的后缀写死 .wav",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `    if (!split_path(*text, std::u16string(s), dir_part, name_part)) continue;
    suffix = std::u16string(s);
    fallbacks.push_back(dir_part + name_part + u".mp3");`,
      to: `    if (!split_path(*text, std::u16string(s), dir_part, name_part)) continue;
    suffix = u".wav";
    fallbacks.push_back(dir_part + name_part + u".mp3");`,
    },
    {
      note: "gif: name 不是字符串时当成成功",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  if (text == nullptr) return false;   // \`path.endsWith\` 抛`,
      to: `  if (text == nullptr) return true;   // \`path.endsWith\` 抛`,
    },
    {
      note: "gif: 起手把 suffix 写成非空",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  suffix.clear();`,
      to: `  suffix = u"zz";`,
    },
    {
      note: "gif: 两个后缀组都没命中时不报告成功",
      file: "native/lfw/loader/get_import_fallbacks.cpp",
      from: `  fallbacks.push_back(*text);
  return true;
}

}
}`,
      to: `  fallbacks.push_back(*text);
  return false;
}

}
}`,
    },
  ],
};
