#include "lfw/loader/preprocess_pic.h"

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace loader {

namespace {

Value field_at(const Value& v, const char16_t* key) {
  const Object* o = as_object(v);
  if (o == nullptr) return Value();
  const Value* p = o->get(std::u16string(key));
  return p != nullptr ? *p : Value();
}

bool is_number(const Value& v) { return std::holds_alternative<double>(v); }

}

Value preprocess_pic(Value& pic) {
  Object* p = as_object(pic);
  if (p == nullptr) return pic;
  const Value rad = field_at(pic, u"rad");
  const Value deg = field_at(pic, u"deg");
  if (is_number(rad)) {
    p->set(u"deg", Value(std::get<double>(rad) * 180.0 / PI));
    const double r = std::get<double>(rad);
    p->set(u"__cos_r", Value(cos(r)));
    p->set(u"__sin_r", Value(sin(r)));
  } else if (is_number(deg)) {
    const double d = std::get<double>(deg) * PI / 180.0;
    p->set(u"rad", Value(d));
    p->set(u"__cos_r", Value(cos(d)));
    p->set(u"__sin_r", Value(sin(d)));
  }
  return pic;
}

Value preprocess_frame_pic(Value& frame) {
  Object* f = as_object(frame);
  if (f == nullptr) return Value();
  const Value* pic = f->get(u"pic");
  if (pic == nullptr || !truthy(*pic)) return pic != nullptr ? *pic : Value();
  Value copy = *pic;
  return preprocess_pic(copy);
}

Value preprocess_wpoint(Value& wpoint) { return wpoint; }

}
}
