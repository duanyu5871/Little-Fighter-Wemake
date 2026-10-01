#include "lfw/loader/preprocess_action.h"

#include <string>
#include <variant>

#include "lfw/core/value.h"
#include "lfw/loader/preprocess_next_frame.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace loader {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

std::u16string to_upper_ascii(const std::u16string& s) {
  std::u16string out = s;
  for (size_t i = 0; i < out.size(); ++i) {
    if (out[i] >= u'a' && out[i] <= u'z') {
      out[i] = static_cast<char16_t>(out[i] - 32);
    }
  }
  return out;
}

bool is_sound_type(const std::u16string& t) { return t == u"A_SOUND" || t == u"V_SOUND"; }

bool is_next_frame_type(const std::u16string& t) {
  return t == u"A_NEXT_FRAME" || t == u"V_NEXT_FRAME" || t == u"A_DEFEND" ||
         t == u"V_DEFEND" || t == u"A_BROKEN_DEFEND" || t == u"V_BROKEN_DEFEND";
}

Value field_of(const Value& v, const char16_t* key) {
  const Object* o = as_object(v);
  const Value* p = o != nullptr ? o->get(std::u16string(key)) : nullptr;
  return p != nullptr ? *p : Value();
}

bool sound_path_iterable(const Value& data) {
  if (is_nullish(data)) return false;
  const Value path = field_of(data, u"path");
  return is_str(path) || as_array(path) != nullptr;
}

}

bool preprocess_action(Value& action) {
  Object* a = as_object(action);
  if (a == nullptr) return false;
  const Value* tv = a->get(u"type");
  if (tv == nullptr || !is_str(*tv)) return false;
  const std::u16string type = to_upper_ascii(std::get<std::u16string>(*tv));
  a->set(u"type", Value(type));
  const Value data = field_of(action, u"data");
  if (is_sound_type(type)) return sound_path_iterable(data);
  if (!is_next_frame_type(type)) return true;
  Value nf = data;
  return preprocess_next_frame(nf);
}

}
}
