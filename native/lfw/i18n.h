#pragma once

#include <map>
#include <string>

#include "lfw/core/value.h"

namespace lfw {

// TS `I18N`：语言别名 + 词表。内部三张表在 TS 里是 `Map`，这里用 `std::map` 顶替 ——
// 三张表都只做「按键取」，从不迭代，所以序不可观察（同 `NestedMap` 的既有约定）。
class I18N {
 public:
  static constexpr const char16_t* kTag = u"I18N";

  I18N();

  const std::u16string& lang() const { return _lang; }

  // TS 在 `lang` 不是字符串时抛 `[I18N::set_lang] lang should be string, but got …`
  // ⇒ 端口返回 false 并把同一段文本写进 `error`。
  bool set_lang(const Value& lang, std::u16string& error);

  // `add(langs)`：三道门（假值 / 不是对象 / 是数组都直接返回），值是字符串 ⇒ 记进别名表，
  // 值是对象 ⇒ 逐个词展开（字符串 ⇒ `strings[k]=v` + `lists[k]=[v]`；数组 ⇒ `strings[k]=
  // 用 '\n' 连接` + `lists[k]=每个元素字符串化`；其余类型忽略）。
  void add(const Value& langs);

  // `alias(lang)`：沿别名表走到没有别名为止（带环检测），无别名（含循环）时是 `undefined`。
  Value alias(const Value& lang) const;
  // `canonical(lang)` = `alias(lang) ?? lang`。
  Value canonical(const Value& lang) const;
  // `string(name, lang)`：`lang == ''`（**松散**比较）时只看基表，键没有就回 `name`；
  // 否则先查本语言表，没有就顺着别名递归（别名是 `undefined` 时用 `''` 兜底）。
  Value string(const Value& name, const Value& lang) const;
  // 同 `string`，但要的是列表（键没有时回 `[name]`）。
  Value strings(const Value& name, const Value& lang) const;

 private:
  const Object* words_of(const Value& lang) const;
  const Object* lists_of(const Value& lang) const;
  const std::u16string* alias_of(const std::u16string& lang) const;

  std::map<std::u16string, Value> _words;  // 语言 → `{ 词: string }`
  std::map<std::u16string, Value> _lists;  // 语言 → `{ 词: string[] }`
  std::map<std::u16string, std::u16string> _alias_map;
  std::u16string _lang;
};

}
