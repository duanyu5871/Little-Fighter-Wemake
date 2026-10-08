#include "lfw/ui/component/text_input.h"

namespace lfw {
namespace ui {

const ClazzTag* TextInput::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& TextInput::TAGS() {
  static const std::vector<std::u16string> tags{u"TextInput"};
  return tags;
}

std::optional<double> TextInput::max_length() { return props_holder.num(u"maxLength"); }

std::optional<std::u16string> TextInput::default_value() {
  return props_holder.str(u"defaultValue");
}

}
}
