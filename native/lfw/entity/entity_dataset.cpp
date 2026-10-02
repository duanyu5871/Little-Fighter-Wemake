#include "lfw/entity/entity_dataset.h"

#include <string>
#include <variant>

#include "lfw/core/value.h"

namespace lfw {
namespace entity {
namespace {

Value get_field(const Value& v, const std::u16string& key) {
  const Object* o = as_object(v);
  if (o == nullptr) return Value();
  const Value* p = o->get(key);
  return p != nullptr ? *p : Value();
}

bool nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

}

Value entity_dataset(const Value& entity, const std::u16string& name) {
  const Value world = get_field(entity, u"world");
  const Value base = get_field(get_field(entity, u"data"), u"base");
  Value v = get_field(get_field(get_field(entity, u"frame"), u"dataset"), name);
  if (nullish(v)) v = get_field(base, name);
  if (nullish(v)) {
    v = get_field(get_field(get_field(get_field(world, u"bg"), u"data"), u"dataset"), name);
  }
  if (nullish(v)) v = get_field(get_field(world, u"dataset"), name);
  return v;
}

}
}
