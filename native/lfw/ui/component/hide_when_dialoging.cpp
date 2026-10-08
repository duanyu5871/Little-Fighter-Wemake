#include "lfw/ui/component/hide_when_dialoging.h"

#include "lfw/ui/uinode.h"

namespace lfw {
namespace ui {

const ClazzTag* HideWhenDialoging::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& HideWhenDialoging::TAGS() {
  static const std::vector<std::u16string> tags{u"HideWhenDialoging"};
  return tags;
}

HideWhenDialoging::HideWhenDialoging(UINode& layout, const std::u16string& f_name,
                                     const Value& info)
    : UIComponent(layout, f_name, info) {}

void HideWhenDialoging::on_start() { _dialog_listener.start(); }

void HideWhenDialoging::on_stop() { _dialog_listener.stop(); }

void HideWhenDialoging::set_dialog(const Value& d) { node.set_visible(!truthy(d)); }

}
}
