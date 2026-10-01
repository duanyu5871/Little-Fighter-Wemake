#include "lfw/dat_translator/cook_file_variants.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {

namespace {

std::u16string strip_suffix(const std::u16string& s) {
  size_t last_dot = std::u16string::npos;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == u'.') last_dot = i;
  }
  if (last_dot == std::u16string::npos) return s;
  return s.substr(0, last_dot);
}

char16_t letter_of(int offset) { return static_cast<char16_t>(98 + offset); }

Value field_or_any(const Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  return v != nullptr ? *v : Value();
}

std::u16string path_of(const Value& v) {
  const Object* o = as_object(v);
  if (o == nullptr) return std::u16string();
  const Value* p = o->get(u"path");
  return p != nullptr ? strip_suffix(to_string(*p)) : std::u16string();
}

bool same_field(const Object& a, const Object& b, const char16_t* key) {
  return strict_equals(field_or_any(a, key), field_or_any(b, key));
}

}

void cook_file_variants(Value& ret) {
  Object* r = as_object(ret);
  if (r == nullptr) return;
  const Value* base_v = r->get(u"base");
  const Object* base = base_v != nullptr ? as_object(*base_v) : nullptr;
  const Value files_default = Value(std::make_shared<Object>());
  const Value* files_v = base != nullptr ? base->get(u"files") : nullptr;
  const Value files_value = files_v != nullptr ? *files_v : files_default;
  const Object* files = as_object(files_value);
  if (files == nullptr) return;

  std::vector<std::u16string> file_keys = files->keys();
  if (file_keys.empty() || file_keys.size() % 2 != 0) return;
  std::sort(file_keys.begin(), file_keys.end());

  std::vector<Value> infos;
  for (const std::u16string& k : file_keys) {
    const Value* v = files->get(k);
    infos.push_back(v != nullptr ? *v : Value());
  }

  const std::u16string first_str = path_of(infos[0]);
  std::vector<double> indexes;
  indexes.push_back(0);
  for (int i = 0; i < 16; ++i) {
    const std::u16string want = first_str + std::u16string(1, letter_of(i));
    double found = -1;
    for (size_t j = 0; j < infos.size(); ++j) {
      if (path_of(infos[j]) == want) {
        found = static_cast<double>(j);
        break;
      }
    }
    if (found < 1) break;
    indexes.push_back(found);
  }

  Value gap_v = n(0);
  for (size_t idx = 0; idx < indexes.size(); ++idx) {
    const double item = indexes[idx];
    if (idx == 0) {
      gap_v = n(item);
      continue;
    }
    const double diff = item - indexes[idx - 1];
    if (idx == 1) {
      gap_v = n(diff);
      continue;
    }
    gap_v = (to_number(gap_v) == diff) ? gap_v : Value(NullTag{});
  }
  if (!truthy(gap_v)) return;
  const size_t gap = static_cast<size_t>(to_number(gap_v));

  for (size_t i = 0; i < gap; ++i) {
    const Object* tmpl = as_object(infos[i]);
    if (tmpl == nullptr) return;
    for (size_t idx = 1; idx < indexes.size(); ++idx) {
      const size_t j = static_cast<size_t>(indexes[idx]) + i;
      const Object* variant = j < infos.size() ? as_object(infos[j]) : nullptr;
      if (variant == nullptr) return;
      const bool path_ok =
          path_of(infos[j]) == path_of(infos[i]) + std::u16string(1, letter_of(static_cast<int>(idx) - 1));
      if (!path_ok || !same_field(*variant, *tmpl, u"col") ||
          !same_field(*variant, *tmpl, u"row") || !same_field(*variant, *tmpl, u"cell_w") ||
          !same_field(*variant, *tmpl, u"cell_h")) {
        return;
      }
    }
  }

  for (size_t i = 0; i < gap; ++i) {
    Object* tmpl = as_object(infos[i]);
    if (tmpl == nullptr) continue;
    Array variants;
    for (size_t idx = 1; idx < indexes.size(); ++idx) {
      const size_t j = static_cast<size_t>(indexes[idx]) + i;
      const Object* src = j < infos.size() ? as_object(infos[j]) : nullptr;
      variants.push_back(src != nullptr ? field_or_any(*src, u"id") : Value());
    }
    tmpl->set(u"variants", Value(std::make_shared<Array>(variants)));
  }
}

}
}
