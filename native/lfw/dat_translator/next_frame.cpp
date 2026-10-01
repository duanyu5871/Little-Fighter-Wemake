#include "lfw/dat_translator/next_frame.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/frame_id.h"

namespace lfw {
namespace dat_translator {
namespace {

Value next_frame_const(const char16_t* name) {
  const Value* v = defines::find(std::u16string(name));
  return v != nullptr ? *v : Value();
}

std::u16string js_string_of(const Value& v) { return to_string(v); }

void drop_falsy(Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  if (v != nullptr && !truthy(*v)) o.remove(std::u16string(key));
}

Value cook_new_frame(Object o, const std::u16string& type, const Object* costs) {
  Value nf = Value(std::make_shared<Object>(std::move(o)));
  return cook_next_frame_cost(nf, type, costs);
}

}

Value cook_next_frame_cost(Value& ret, const std::u16string& type, const Object* costs) {
  if (costs == nullptr) return ret;
  if (std::holds_alternative<std::monostate>(ret) || std::holds_alternative<NullTag>(ret)) return ret;
  Object* obj = as_object(ret);
  if (obj == nullptr) return ret;
  {
    const Value* idv = obj->get(u"id");
    if (idv == nullptr) return ret;
    std::u16string id;
    if (const std::u16string* s = std::get_if<std::u16string>(idv)) {
      id = *s;
    } else if (truthy(*idv)) {
      const Array* a = as_array(*idv);
      if (a == nullptr || a->size() == 0) return ret;
      id = js_string_of(a->at(0));
    }
    if (id.empty()) return ret;
    const Value* cost = costs->get(id);
    double mp = 0;
    double hp = 0;
    if (cost != nullptr) {
      if (const Object* co = as_object(*cost)) {
        if (const Value* v = co->get(u"mp")) mp = to_number(*v);
        if (const Value* v = co->get(u"hp")) hp = to_number(*v);
      }
    }
    if (type == u"hit") {
      obj->set(u"mp", Value(mp));
      obj->set(u"hp", Value(hp));
    } else if (type == u"next") {
      if (mp < 0) obj->set(u"mp", Value(-mp));
      if (hp < 0) obj->set(u"hp", Value(-hp));
    }
    drop_falsy(*obj, u"mp");
    drop_falsy(*obj, u"hp");
  }
  return ret;
}

Value get_next_frame_by_raw_id(const Value& id, const std::u16string& zero_as,
                               const std::u16string& type, const Object* costs) {
  const std::u16string raw = js_string_of(id);
  if (raw == u"1000") return next_frame_const(u"Defines.NEXT_FRAME_GONE");
  if (raw == u"999") return next_frame_const(u"Defines.NEXT_FRAME_AUTO");
  if (raw == u"-999") return next_frame_const(u"Defines.NEXT_FRAME_AUTO_BACKWARD");
  if (raw == u"0") {
    Object o;
    if (zero_as == u"frame") o.set(u"id", Value(raw));
    return Value(std::make_shared<Object>(o));
  }
  if (const double* d = std::get_if<double>(&id)) {
    const double n = *d;
    if (n >= 1100 && n <= 1299) return next_frame_const(u"Defines.NEXT_FRAME_AUTO");
    if (n <= -1100 && n >= -1299) return next_frame_const(u"Defines.NEXT_FRAME_AUTO_BACKWARD");
    if (n < 0) {
      Object o;
      o.set(u"id", Value(number_to_string(-n)));
      o.set(u"facing", Value(static_cast<double>(FacingFlag::Backward)));
      return cook_new_frame(std::move(o), type, costs);
    }
  }
  if (const std::u16string* s = std::get_if<std::u16string>(&id)) {
    if (s->size() > 0 && (*s)[0] == u'-') {
      Object o;
      o.set(u"id", Value(s->substr(1)));
      o.set(u"facing", Value(static_cast<double>(FacingFlag::Backward)));
      return cook_new_frame(std::move(o), type, costs);
    }
  }
  Object o;
  o.set(u"id", Value(raw));
  return cook_new_frame(std::move(o), type, costs);
}

Value add_next_frame(const Value& src, const std::vector<Value>& items) {
  if (items.empty()) return src;
  Array out;
  if (const Array* a = as_array(src)) {
    for (size_t i = 0; i < a->size(); ++i) out.push_back(a->at(i));
  } else if (truthy(src)) {
    out.push_back(src);
  }
  for (const Value& v : items) out.push_back(v);
  return Value(std::make_shared<Array>(out));
}

Value edit_next_frame(Value& nexts, const std::function<void(Value&, size_t)>& fn) {
  if (std::holds_alternative<std::monostate>(nexts) || std::holds_alternative<NullTag>(nexts)) {
    return nexts;
  }
  if (Array* a = as_array(nexts)) {
    for (size_t i = 0; i < a->size(); ++i) fn(a->at(i), i);
  } else {
    fn(nexts, 0);
  }
  return nexts;
}

}

}
