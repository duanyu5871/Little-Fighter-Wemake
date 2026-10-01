#include "lfw/loader/preprocess_bot_data.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/traversal.h"

namespace lfw {
namespace loader {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

std::vector<std::u16string> split_commas(const std::u16string& s) {
  std::vector<std::u16string> out;
  std::u16string cur;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == u',') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(s[i]);
    }
  }
  out.push_back(cur);
  return out;
}

bool is_high_surrogate(char16_t c) { return c >= 0xD800 && c <= 0xDBFF; }

bool is_low_surrogate(char16_t c) { return c >= 0xDC00 && c <= 0xDFFF; }

bool spread_copy(const Value& v, Value& out) {
  const Array* a = as_array(v);
  if (a != nullptr) {
    out = Value(std::make_shared<Array>(*a));
    return true;
  }
  const std::u16string* s = std::get_if<std::u16string>(&v);
  if (s != nullptr) {
    std::vector<Value> items;
    for (size_t i = 0; i < s->size(); ++i) {
      std::u16string cp;
      cp.push_back((*s)[i]);
      if (is_high_surrogate((*s)[i]) && i + 1 < s->size() && is_low_surrogate((*s)[i + 1])) {
        cp.push_back((*s)[i + 1]);
        ++i;
      }
      items.push_back(Value(cp));
    }
    out = Value(std::make_shared<Array>(std::move(items)));
    return true;
  }
  return false;
}

bool expand_comma_keys(Value& holder) {
  Object* o = as_object(holder);
  if (o == nullptr) return true;
  bool ok = true;
  traversal(holder, [o, &ok](const std::u16string& k, Value& v) {
    if (!ok) return;
    if (!truthy(v)) return;
    const std::vector<std::u16string> ks = split_commas(k);
    if (ks.size() <= 1) return;
    const Value source = v;
    o->remove(k);
    for (const std::u16string& key : ks) {
      Value item;
      if (!spread_copy(source, item)) {
        ok = false;
        return;
      }
      o->set(key, std::move(item));
    }
  });
  return ok;
}

}

bool preprocess_bot_data(Value& data) {
  if (is_nullish(data)) return false;
  Object* d = as_object(data);
  if (d == nullptr) return true;
  const Value* frames = d->get(u"frames");
  if (frames != nullptr) {
    Value holder = *frames;
    if (!expand_comma_keys(holder)) return false;
  }
  const Value* states = d->get(u"states");
  if (states != nullptr) {
    Value holder = *states;
    if (!expand_comma_keys(holder)) return false;
  }
  return true;
}

}
}
