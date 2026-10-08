#include "lfw/ui/component/extra_zip_text.h"

#include "lfw/lfw.h"
#include "lfw/ui/uinode.h"

namespace lfw {
namespace ui {

const ClazzTag* ExtraZipText::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& ExtraZipText::TAGS() {
  static const std::vector<std::u16string> tags{u"ExtraZipText"};
  return tags;
}

void ExtraZipText::on_start() {
  if (!_remove_zips) {
    _remove_zips = lfw().callbacks.on(
        u"on_extra_zips_changed", [this](const LfwCallbacks::Payloads&) { render(); });
  }
  if (!_remove_lang) {
    _remove_lang = lfw().callbacks.on(
        u"on_lang_changed", [this](const LfwCallbacks::Payloads&) { render(); });
  }
  render();
}

void ExtraZipText::on_stop() {
  if (_remove_zips) {
    _remove_zips();
    _remove_zips = nullptr;
  }
  if (_remove_lang) {
    _remove_lang();
    _remove_lang = nullptr;
  }
}

void ExtraZipText::render() {
  const std::u16string extra_zips = to_string(lfw().string(u"DATA_LIST"));
  if (!extra_zips.empty()) {
    const std::u16string head = to_string(lfw().string(u"extra_data"));
    node.set_text(head + u":\n" + extra_zips);
  } else {
    node.set_text(u" ");
  }
}

}
}
