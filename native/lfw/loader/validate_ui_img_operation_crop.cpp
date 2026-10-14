#include "lfw/loader/validate_ui_img_operation_crop.h"

#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/type_check.h"

namespace lfw {

namespace {

const char16_t* const kFn = u"validate_ui_img_operation_crop";

using OkPred = bool (*)(const Value&);

struct CropField {
  const char16_t* name;
  OkPred ok;
  const char16_t* must_be;
};

// TS 里 x/y/w/h/dw/dh 六条同样形状的检查（告警文本连 `undefiend` 的拼写都照抄）。
const CropField kCropFields[] = {
    {u"x", is_non_nagative_int, u"a non-negative integer or undefiend"},
    {u"y", is_non_nagative_int, u"a non-negative integer or undefiend"},
    {u"w", is_positive_int, u"a positive integer or undefiend"},
    {u"h", is_positive_int, u"a positive integer or undefiend"},
    {u"dw", is_positive_int, u"a positive integer or undefiend"},
    {u"dh", is_positive_int, u"a positive integer or undefiend"},
};

}  // namespace

bool validate_ui_img_operation_crop(const Value& any, std::vector<std::u16string>* errors) {
  const Object* const obj = as_object(any);
  if (obj == nullptr) {
    if (errors != nullptr) {
      errors->push_back(std::u16string(kFn) + u"] must be an object, but got " + to_string(any));
    }
    return false;
  }
  bool ret = true;
  const Value* const type = obj->get(u"type");
  if (type == nullptr || !strict_equals(*type, Value(u"crop"))) {
    ret = false;
    if (errors != nullptr) {
      errors->push_back(u"[" + std::u16string(kFn) + u"]type must be \"crop\" or, but got " +
                        to_string(type != nullptr ? *type : Value()));
    }
  }
  for (const CropField& field : kCropFields) {
    const Value* const value = obj->get(field.name);
    if (value == nullptr || field.ok(*value)) continue;
    ret = false;
    if (errors != nullptr) {
      errors->push_back(u"[" + std::u16string(kFn) + u"]" + field.name + u" must be " +
                        field.must_be + u", but got " + to_string(*value));
    }
  }
  return ret;
}

}
