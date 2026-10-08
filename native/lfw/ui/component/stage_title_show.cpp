#include "lfw/ui/component/stage_title_show.h"

#include <memory>
#include <string>
#include <utility>

#include "lfw/animation/delay.h"
#include "lfw/animation/easing.h"
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

}

const ClazzTag* StageTitleShow::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& StageTitleShow::TAGS() {
  static const std::vector<std::u16string> tags{u"StageTitleShow"};
  return tags;
}

StageTitleShow::StageTitleShow(UINode& layout, const std::u16string& f_name, const Value& info)
    : UIComponent(layout, f_name, info) {
  std::unique_ptr<Easing> fade_in = std::make_unique<Easing>(0, 1);
  fade_in->set_duration(500);
  std::unique_ptr<Delay> hold = std::make_unique<Delay>(1);
  hold->set_duration(1500);
  std::unique_ptr<Easing> fade_out = std::make_unique<Easing>(1, 0);
  fade_out->set_duration(500);
  _opacity = Sequence({fade_in.get(), hold.get(), fade_out.get()});
  _owned.push_back(std::move(fade_in));
  _owned.push_back(std::move(hold));
  _owned.push_back(std::move(fade_out));
}

void StageTitleShow::set_text(const std::u16string& text) {
  node.set_text(text);
  _opacity.start(false);
}

void StageTitleShow::on_chapter_finish() { set_text(u"STAGE CLEAR!"); }

void StageTitleShow::on_stage_change(stage::Stage* stage, stage::Stage* prev) {
  if (prev != nullptr && _stage_remover) {
    _stage_remover();
    _stage_remover = nullptr;
  }
  if (stage != nullptr) {
    _stage_remover = stage->callbacks().on(
        u"on_chapter_finish",
        [this](const stage::StageCallbacks::Payloads&) { on_chapter_finish(); });
  }
  Value t = stage != nullptr ? field(stage->data(), u"title") : Value();
  if (is_nullish(t)) t = stage != nullptr ? stage->bg()->name() : Value();
  if (is_nullish(t)) t = Value(std::u16string());
  set_text(to_string(lfw().string(t)));
}

void StageTitleShow::on_start() {
  if (!_world_remover) {
    _world_remover = world().callbacks.on(
        u"on_stage_change", [this](const WorldCallbacks::Payloads& p) {
          for (const WorldCallbackArgs& a : p) on_stage_change(a.stage, a.prev_stage);
        });
  }
  on_stage_change(world().stage(), nullptr);
}

void StageTitleShow::on_stop() {
  if (_world_remover) {
    _world_remover();
    _world_remover = nullptr;
  }
}

void StageTitleShow::update(double dt) {
  _opacity.update(dt);
  node.set_opacity(_opacity.value());
}

}
}
