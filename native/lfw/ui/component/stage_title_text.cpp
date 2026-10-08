#include "lfw/ui/component/stage_title_text.h"

#include <memory>
#include <string>

#include "lfw/core/js_string.h"
#include "lfw/lfw.h"
#include "lfw/ui/uinode.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

const Value& field(const Value& obj, const char16_t* key) {
  static const Value kNull;
  const Object* const o = as_object(obj);
  if (o == nullptr) return kNull;
  const Value* const p = o->get(std::u16string(key));
  return p != nullptr ? *p : kNull;
}

// TS 的 `_style` 字面量（插入序 = 键序）。
Value make_style() {
  Value out(std::make_shared<Object>());
  Object* const o = as_object(out);
  o->set(u"fill_style", Value(std::u16string(u"white")));
  o->set(u"font", Value(std::u16string(u"12px Arial")));
  o->set(u"line_width", Value(1.0));
  o->set(u"padding_t", Value(2.0));
  return out;
}

}

const ClazzTag* StageTitleText::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& StageTitleText::TAGS() {
  static const std::vector<std::u16string> tags{u"StageTitleText"};
  return tags;
}

void StageTitleText::set_title(const std::u16string& title) {
  if (_title == title) return;
  _title = title;
  node.set_text(title, make_style());
}

void StageTitleText::set_phase(const Value& phase) {
  Value t = field(phase, u"title");
  if (is_nullish(t)) {
    t = _stage != nullptr ? _stage->title() : Value(std::u16string());
    if (is_nullish(t)) t = Value(std::u16string());
  }
  set_title(to_string(lfw().string(t)));
}

void StageTitleText::set_stage(stage::Stage* stage) {
  if (_stage == stage) return;
  if (_stage_remover) {
    _stage_remover();
    _stage_remover = nullptr;
  }
  if (stage != nullptr) {
    _stage_remover = stage->callbacks().on(
        u"on_phase_changed", [this](const stage::StageCallbacks::Payloads& p) {
          for (const stage::StageCallbackArgs& a : p) set_phase(a.a);
        });
  }
  _stage = stage;
  set_phase(stage != nullptr ? stage->phase() : Value(NullTag{}));
}

void StageTitleText::on_start() {
  _title.clear();
  if (!_world_remover) {
    _world_remover = world().callbacks.on(
        u"on_stage_change", [this](const WorldCallbacks::Payloads& p) {
          for (const WorldCallbackArgs& a : p) set_stage(a.stage);
        });
  }
  set_stage(world().stage());
}

void StageTitleText::on_stop() {
  if (_world_remover) {
    _world_remover();
    _world_remover = nullptr;
  }
  set_stage(nullptr);
}

}
}
