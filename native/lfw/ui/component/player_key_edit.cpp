#include "lfw/ui/component/player_key_edit.h"

#include <cstddef>
#include <string>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/ui/uinode.h"

namespace lfw {
namespace ui {

const ClazzTag* PlayerKeyEdit::class_tag() {
  static const ClazzTag tag{PlayerKeyText::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& PlayerKeyEdit::TAGS() {
  static const std::vector<std::u16string> tags{u"PlayerKeyEdit"};
  return tags;
}

void PlayerKeyEdit::KeyListener::on_key_down(LfwKeyEvent& e) {
  PlayerKeyEdit& me = *owner;
  if (!me.node.focused()) return;
  if (to_lower_case(e.key) != u"escape") {
    PlayerInfo* const p = me.player();
    if (p != nullptr) {
      std::u16string error;
      p->set_key(Value(me.key_name()), Value(e.key), true, error);
      p->save();
    }
    const std::vector<UIComponent*> others = me.node.root().search_components(
        class_tag(), [&me](UIComponent& v) -> UIFind {
          return static_cast<PlayerKeyEdit&>(v).player() == me.player() ? UIFind::Yes
                                                                        : UIFind::No;
        });
    std::size_t idx = others.size();
    for (std::size_t k = 0; k < others.size(); ++k) {
      if (others[k] == &me) {
        idx = k;
        break;
      }
    }
    if (idx + 1 < others.size()) others[idx + 1]->node.set_focused(true);
  }
  me.node.set_focused(false);
}

void PlayerKeyEdit::PointListener::on_click(LfwPointingEvent& e) {
  (void)e;
  if (owner->_click_me) {
    owner->_click_me = false;
    return;
  }
  owner->node.set_focused(false);
}

void PlayerKeyEdit::on_parent_click(LFWPointerEvent& e) {
  _click_me = true;
  node.set_focused(!node.focused());
  e.stop_immediate_propagation();
}

void PlayerKeyEdit::on_start() {
  node.style.set_font(Value(u"17px Arial"));
  UINode* const parent = node.parent();
  if (parent != nullptr) {
    parent->callbacks.on_click = [this](LFWPointerEvent& e) { on_parent_click(e); };
  }
}

void PlayerKeyEdit::on_stop() {
  UINode* const parent = node.parent();
  if (parent != nullptr) parent->callbacks.on_click = nullptr;
}

void PlayerKeyEdit::on_foucs() {
  on_key_changed();
  lfw().pointings_callback_add(&_point_listener);
  lfw().keyboard_callback_add(&_key_listener);
}

void PlayerKeyEdit::on_blur() {
  on_key_changed();
  lfw().pointings_callback_del(&_point_listener);
  lfw().keyboard_callback_del(&_key_listener);
}

void PlayerKeyEdit::on_key_changed() {
  node.set_text(key_code());
  if (node.focused()) node.style.set_fill_style(Value(u"#5555FF"));
  else if (key_code() == u"None") node.style.set_fill_style(Value(u"gray"));
  else node.style.set_fill_style(Value(u"white"));
}

}
}
