#include "lfw/ui/component/bgm_switcher.h"

#include <optional>
#include <vector>

#include "lfw/ui/uinode.h"
#include "lfw/utils/container_help/loop_offset.h"

namespace lfw {
namespace ui {

const ClazzTag* BGMSwitcher::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& BGMSwitcher::TAGS() {
  static const std::vector<std::u16string> tags{u"BGMSwitcher"};
  return tags;
}

void BGMSwitcher::SoundsListener::on_bgm_changed(const std::u16string& bgm,
                                                 const std::u16string& prev) {
  (void)bgm;
  (void)prev;
  owner->node.set_text(u"BGM: " + owner->which());
}

std::u16string BGMSwitcher::which() {
  if (!_which.empty()) return _which;
  const std::u16string bgm = lfw().sounds_bgm();
  return !bgm.empty() ? bgm : u"Silent";
}

double BGMSwitcher::offset() { return num(0).value_or(0.0); }

void BGMSwitcher::on_resume() {
  if (lfw().sounds_is_random()) _which = u"Random";
  if (offset() == 0.0) {
    lfw().sounds_callback_add(&_sounds_listener);
    _sounds_listener.on_bgm_changed(std::u16string(), std::u16string());
  }
}

void BGMSwitcher::on_pause() { lfw().sounds_callback_del(&_sounds_listener); }

void BGMSwitcher::on_click(LFWPointerEvent& e) {
  UIComponent::on_click(e);
  e.stop_immediate_propagation();
  e.stop_propagation();
  const double off = offset();
  std::vector<std::u16string> list{u"Silent", u"Random"};
  for (const std::u16string& b : lfw().bgms) list.push_back(b);
  const std::u16string cur = which();
  std::u16string next;
  if (off != 0.0) {
    const std::optional<std::u16string> n = loop_offset(list, cur, off);
    if (n.has_value()) next = *n;
  } else if (e.button == 0.0) {
    const std::optional<std::u16string> n = loop_offset(list, cur, 1.0);
    if (n.has_value()) next = *n;
  } else if (e.button == 1.0) {
    next = cur == u"Silent" ? u"Random" : u"Silent";
  } else if (e.button == 2.0) {
    const std::optional<std::u16string> n = loop_offset(list, cur, -1.0);
    if (n.has_value()) next = *n;
  } else {
    return;
  }

  if (next == u"Random") {
    _which = u"Random";
    lfw().sounds_play_bgm(Value(u"?"));
  } else if (next == u"Silent") {
    _which = u"Silent";
    lfw().sounds_stop_bgm();
  } else {
    _which.clear();
    if (!next.empty()) lfw().sounds_play_bgm(Value(next));
  }
}

}
}
