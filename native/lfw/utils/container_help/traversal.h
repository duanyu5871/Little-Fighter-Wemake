#pragma once

#include <functional>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"

namespace lfw {

inline void traversal(Value& obj, const std::function<void(const std::u16string&, Value&)>& fn) {
  if (Array* a = as_array(obj)) {
    const size_t n = a->size();
    for (size_t i = 0; i < n; ++i) {
      Value item = i < a->size() ? a->at(i) : Value();
      fn(number_to_string(static_cast<double>(i)), item);
    }
    return;
  }
  if (const std::u16string* const text = std::get_if<std::u16string>(&obj)) {
    // `Object.keys("ab")` 是 `["0","1"]`，回调拿到的值是那一个字符。
    for (size_t i = 0; i < text->size(); ++i) {
      Value item(std::u16string(1, (*text)[i]));
      fn(number_to_string(static_cast<double>(i)), item);
    }
    return;
  }
  Object* o = as_object(obj);
  if (o == nullptr) return;
  const std::vector<std::u16string> keys = o->keys();
  for (const std::u16string& k : keys) {
    const Value* found = o->get(k);
    Value item = found != nullptr ? *found : Value();
    fn(k, item);
  }
}

// `traversal(r, (k, v, o) => { …; o[k] = v })` 的等价物：回调返回 true ⇒ 把（可能被改过的）`v`
// 写回，返回 false ⇒ 整体失败（TS 那边是抛）。
//
// 与上面那个值版的两点差别：① 写回；② **非空字符串**直接失败 —— `Object.keys("ab")` 是
// `["0","1"]`，而回调里的 `o[k] = …` 在严格模式下给字符串赋值会抛；数字 / 布尔的 `Object.keys`
// 是空表（不遍历、也不失败）。键表照抄 `Object.keys` 的快照语义（遍历中新增的键不会被访问）。
inline bool traversal_write(Value& obj,
                            const std::function<bool(const std::u16string&, Value&)>& fn) {
  if (const std::u16string* const text = std::get_if<std::u16string>(&obj)) return text->empty();
  if (Array* const a = as_array(obj)) {
    const size_t n = a->size();
    for (size_t i = 0; i < n; ++i) {
      Value item = i < a->size() ? a->at(i) : Value();
      if (!fn(number_to_string(static_cast<double>(i)), item)) return false;
      if (i < a->size()) a->at(i) = item;
    }
    return true;
  }
  Object* const o = as_object(obj);
  if (o == nullptr) return true;
  const std::vector<std::u16string> keys = o->keys();
  for (const std::u16string& k : keys) {
    const Value* const found = o->get(k);
    if (found == nullptr) continue;
    Value item = *found;
    if (!fn(k, item)) return false;
    o->set(k, item);
  }
  return true;
}

}

