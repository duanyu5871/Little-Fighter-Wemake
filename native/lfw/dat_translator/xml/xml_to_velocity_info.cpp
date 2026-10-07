#include "lfw/dat_translator/xml/xml_to_velocity_info.h"

#include <memory>
#include <optional>

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

const char16_t* const kNumKeys[12] = {u"dvx",   u"dvy",    u"dvz",   u"acc_x", u"acc_y",
                                      u"acc_z", u"vxm",    u"vym",   u"vzm",   u"ctrl_x",
                                      u"ctrl_y", u"ctrl_z"};

// `if (typeof dv?.[i] === 'number') out.k = dv[i];` —— 只认数字分量。
void override_from(const IXMLElement& el, const char16_t* attr, Object& o,
                   const char16_t* keys[3]) {
  const std::optional<std::vector<std::optional<double>>> v = el.nums_attr_soft(attr);
  if (!v) return;
  for (size_t i = 0; i < 3; ++i) {
    if (i >= v->size()) continue;
    if (const std::optional<double> d = (*v)[i]) o.set(std::u16string(keys[i]), Value(*d));
  }
}

}

Value xml_to_velocity_info(const IXMLElement& el, Value out) {
  if (as_object(out) == nullptr) out = Value(std::make_shared<Object>());
  Object* const o = as_object(out);
  for (const char16_t* key : kNumKeys) {
    o->set(std::u16string(key),
           from_opt(get_num_or(el, std::u16string(key), field_or(*o, key))));
  }
  const char16_t* dv[3] = {u"dvx", u"dvy", u"dvz"};
  const char16_t* acc[3] = {u"acc_x", u"acc_y", u"acc_z"};
  const char16_t* vm[3] = {u"vxm", u"vym", u"vzm"};
  const char16_t* ctrl[3] = {u"ctrl_x", u"ctrl_y", u"ctrl_z"};
  override_from(el, u"dv", *o, dv);
  override_from(el, u"acc", *o, acc);
  override_from(el, u"vm", *o, vm);
  override_from(el, u"ctrl", *o, ctrl);
  for (const char16_t* key : kNumKeys) {
    const Value* v = o->get(std::u16string(key));
    if (v != nullptr && std::holds_alternative<std::monostate>(*v)) o->remove(std::u16string(key));
  }
  return out;
}

}
}
}
