#include "lfw/ui/component/ending_page_logic.h"

#include <string>
#include <vector>

#include "lfw/defines/difficulty.h"
#include "lfw/lfw.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uilayer.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/is_class.h"

namespace lfw {
namespace ui {

const ClazzTag* EndingPageLogic::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& EndingPageLogic::TAGS() {
  static const std::vector<std::u16string> tags{u"EndingPageLogic"};
  return tags;
}

const Value& EndingPageLogic::PROPS() {
  static const Value meta = [] {
    auto node_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:UINode"));
      return p;
    };
    auto jalousie_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:Jalousie"));
      return p;
    };
    auto o = std::make_shared<Object>();
    o->set(u"label", Value(node_prop()));
    o->set(u"jalousie", Value(jalousie_prop()));
    return Value(o);
  }();
  return meta;
}

EndingPageLogic::EndingPageLogic(UINode& layout, const std::u16string& f_name,
                                 const Value& info)
    : UIComponent(layout, f_name, info) {}

EndingPageLogic::JalousieCbT::JalousieCbT(EndingPageLogic* o) {
  handlers.push_back({u"on_anim_end", [o](const JalousieCallbacks::Payloads& a) {
                        if (!a.empty() && a[0].self != nullptr) o->on_jalousie_anim_end(*a[0].self);
                      }});
}

UINode* EndingPageLogic::label_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"label"));
}

Jalousie* EndingPageLogic::jalousie() {
  if (!ensure_props()) return nullptr;
  UIComponent* const c = ref_to_comp(props_holder.prop_instance(u"jalousie"));
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), Jalousie::class_tag()) ? static_cast<Jalousie*>(c) : nullptr;
}

double EndingPageLogic::difficulty() {
  return to_number(field_or(world().world_dataset(), u"difficulty"));
}

void EndingPageLogic::on_start() {
  UIComponent::on_start();
  texts_idx_ = -1;
  Jalousie* const j = jalousie();
  if (j != nullptr) j->callbacks.add(&_cb);
}

void EndingPageLogic::on_resume() {
  lfw().sounds_stop_bgm();
  Jalousie* const j = jalousie();
  if (j != nullptr && j->anim().done()) j->set_open(!j->open());
}

void EndingPageLogic::on_stop() {
  UIComponent::on_stop();
  Jalousie* const j = jalousie();
  if (j != nullptr) j->callbacks.del(&_cb);
}

void EndingPageLogic::on_click(LFWPointerEvent& e) {
  (void)e;
  Jalousie* const j = jalousie();
  if (j != nullptr && j->anim().done()) j->set_open(!j->open());
}

void EndingPageLogic::on_key_down(LFWKeyEvent& e) {
  (void)e;
  Jalousie* const j = jalousie();
  if (j != nullptr && j->anim().done()) j->set_open(!j->open());
}

void EndingPageLogic::on_jalousie_anim_end(Jalousie& j) {
  if (j.open()) return;
  UINode* const label = label_node();
  if (label != nullptr) {
    if (texts_idx_ == -1 && difficulty() >= static_cast<double>(Difficulty::Difficult)) {
      texts_idx_ += 2;
    } else {
      texts_idx_ += 1;
    }
    if (texts_idx_ >= static_cast<double>(texts_.size())) {
      UILayer* const layer = node.layer();
      if (layer != nullptr) {
        if (layer->pages().size() > 1) {
          UIPopPageOpts opts;
          opts.until = [](UINode&, double i, const std::vector<UINode*>&) { return i == 0; };
          layer->pop(opts);
        } else {
          UIPushPageOpts opts;
          opts.id = u"main_page";
          opts.has_id = true;
          layer->set(opts);
          lfw().sounds_play_bgm(Value(std::u16string(u"bgm/main.wma.mp3")));
        }
      }
    } else {
      label->set_text(texts_[static_cast<size_t>(texts_idx_)]);
    }
  }
  j.set_open(true);
}

}
}
