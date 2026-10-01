#include "lfw/loader/resolve_prefab.h"

#include <memory>
#include <string>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"

namespace lfw {
namespace loader {

namespace {

Value field_at(const Value& v, const char16_t* key) {
  const Object* o = as_object(v);
  if (o == nullptr) return Value();
  const Value* p = o->get(std::u16string(key));
  return p != nullptr ? *p : Value();
}

bool is_undefined(const Value& v) { return std::holds_alternative<std::monostate>(v); }

Value ref_of(const Value& v) {
  const Value r = field_at(v, u"ref");
  if (!is_undefined(r)) return r;
  return field_at(v, u"prefab_id");
}

Value spread_assign(const Value& a, const Value& b) {
  Object out;
  const Object* ao = as_object(a);
  if (ao != nullptr) {
    const std::vector<std::u16string> ks = ao->keys();
    for (const std::u16string& k : ks) out.set(k, *ao->get(k));
  }
  const Object* bo = as_object(b);
  if (bo != nullptr) {
    const std::vector<std::u16string> ks = bo->keys();
    for (const std::u16string& k : ks) out.set(k, *bo->get(k));
  }
  return Value(std::make_shared<Object>(out));
}

bool chain_has(const std::vector<std::u16string>& chain, const std::u16string& ref) {
  for (const std::u16string& c : chain) {
    if (c == ref) return true;
  }
  return false;
}

std::vector<std::u16string> chain_with(const std::vector<std::u16string>& chain,
                                       const std::u16string& ref) {
  std::vector<std::u16string> out = chain;
  out.push_back(ref);
  return out;
}

}

ResolvePrefabResult resolve_prefab(const Value& obj, const Value& prefabs) {
  ResolvePrefabResult res;
  std::vector<std::u16string> chain;
  Value base;
  bool has_base = false;
  Value ref = ref_of(obj);
  while (!is_undefined(ref)) {
    const std::u16string key = to_string(ref);
    if (chain_has(chain, key)) {
      res.ok = false;
      res.cycle = true;
      res.chain = chain_with(chain, key);
      return res;
    }
    const Value prefab = field_at(prefabs, key.c_str());
    if (!truthy(prefab)) {
      res.ok = false;
      res.cycle = false;
      res.chain = chain_with(chain, key);
      return res;
    }
    chain.push_back(key);
    base = spread_assign(prefab, has_base ? base : Value());
    has_base = true;
    ref = ref_of(prefab);
  }
  res.ok = true;
  res.chain = chain;
  res.value = has_base ? spread_assign(base, obj) : obj;
  return res;
}

std::u16string prefab_error_message(const std::u16string& tag, const std::u16string& who,
                                    const std::u16string& what, const ResolvePrefabResult& r) {
  if (!r.cycle) {
    const std::u16string last = r.chain.empty() ? std::u16string() : r.chain[r.chain.size() - 1];
    return std::u16string(u"[") + tag + u"] \"" + who + u"\" " + what +
           u" references missing prefab: \"" + last + u"\"";
  }
  std::u16string joined;
  for (size_t i = 0; i < r.chain.size(); ++i) {
    if (i > 0) joined += u" -> ";
    joined += r.chain[i];
  }
  return std::u16string(u"[") + tag + u"] \"" + who + u"\" " + what +
         u" prefab ref cycle: " + joined;
}

}
}
