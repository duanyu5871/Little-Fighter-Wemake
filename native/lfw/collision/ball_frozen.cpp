#include "lfw/collision/ball_frozen.h"

#include <cstddef>
#include <memory>
#include <variant>

#include "lfw/defines/entity_group.h"
#include "lfw/defines/frame_id.h"
#include "lfw/defines/gone_frame_info.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/state_enum.h"
#include "lfw/entity/face_helper.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace collision {
namespace {

BallFrozenEnv g_env;

bool group_length_truthy(const Value& group) {
  if (const Array* arr = as_array(group)) return arr->size() != 0;
  if (const std::u16string* s = std::get_if<std::u16string>(&group)) return s->size() != 0;
  return false;
}

bool group_some_equal(const Value& group, const char16_t* name) {
  const Array* arr = as_array(group);
  if (arr == nullptr) return false;
  const Value want{std::u16string(name)};
  for (size_t i = 0; i < arr->size(); ++i) {
    if (equals(arr->at(i), want)) return true;
  }
  return false;
}

bool group_some_not_equal(const Value& group, const char16_t* name) {
  const Array* arr = as_array(group);
  if (arr == nullptr) return false;
  const Value want{std::u16string(name)};
  for (size_t i = 0; i < arr->size(); ++i) {
    if (!equals(arr->at(i), want)) return true;
  }
  return false;
}

Value& freeze_ball_opoint() {
  static Value v = [] {
    Object action;
    action.set(u"id", Value(std::u16string(frame_id::kAuto)));
    Object o;
    o.set(u"oid", Value(std::u16string(oid::kFreezeBall)));
    o.set(u"kind", Value(0.0));
    o.set(u"x", Value(0.0));
    o.set(u"y", Value(0.0));
    o.set(u"action", Value(std::make_shared<Object>(action)));
    o.set(u"z", Value(0.0));
    return Value(std::make_shared<Object>(o));
  }();
  return v;
}

double frame_number(const Value& frame, const char16_t* key) {
  return to_number(field_or(frame, key));
}

}

const BallFrozenEnv& ball_frozen_env() { return g_env; }

void set_ball_frozen_env(const BallFrozenEnv& env) { g_env = env; }

bool handle_ball_frozen(IFrozenEntity& a_ref, IFrozenEntity& v_ref, const Value& itr) {
  IFrozenEntity* a = &a_ref;
  IFrozenEntity* v = &v_ref;

  if (!group_length_truthy(a->group())) return false;
  if (!group_length_truthy(v->group())) return false;

  const Value itr_kind = field_or(itr, u"kind");
  const bool ok = equals(itr_kind, Value(static_cast<double>(ItrKind::Normal))) ||
                  equals(itr_kind, Value(static_cast<double>(ItrKind::CharacterThrew))) ||
                  equals(itr_kind, Value(static_cast<double>(ItrKind::WeaponSwing)));
  if (!ok) return false;

  const Value a_group = a->group();
  const Value v_group = v->group();
  if (group_some_equal(v_group, entity_group::kFreezer) &&
      group_some_equal(a_group, entity_group::kFreezableBall)) {
    IFrozenEntity* temp = a;
    a = v;
    v = temp;
  } else if (group_some_not_equal(a_group, entity_group::kFreezer) ||
             group_some_not_equal(v_group, entity_group::kFreezableBall)) {
    return false;
  }

  if (!equals(v->state(), Value(static_cast<double>(StateEnum::Ball_Flying)))) return false;
  if (!g_env.is_ball(*v)) {
    if (!g_env.is_fighter(*a)) {
      if (!equals(a->state(), Value(static_cast<double>(StateEnum::Weapon_OnHand)))) return false;
    }
  }

  const double x1 = a->position_x();
  const double y1 = a->position_y();
  const double z1 = a->position_z();
  const double x2 = v->position_x();
  const double y2 = v->position_y();
  const double z2 = v->position_z();

  const Value a_frame = a->frame();
  const Value v_frame = v->frame();
  const double a_centerx = frame_number(a_frame, u"centerx");
  const double a_centery = frame_number(a_frame, u"centery");
  const double v_centerx = frame_number(v_frame, u"centerx");
  const double v_centery = frame_number(v_frame, u"centery");
  const double v_width = frame_number(v_frame, u"width");
  const double v_height = frame_number(v_frame, u"height");

  const double a_facing = to_number(a->facing());
  const double v_facing_number = to_number(v->facing());

  const double cy1 = y1 + a_centery;
  const double cx1 = a_facing > 0 ? x1 - a_centerx : x1 + a_centerx - a_centerx;

  const double cy2 = (y2 + v_centery) - v_height / 2.0;
  const double cx2 = v_facing_number > 0 ? (x2 - v_centerx) + v_width / 2.0
                                         : (x2 + v_centerx - v_width) + v_width / 2.0;

  Value& opoint_v = freeze_ball_opoint();
  Object* opoint = as_object(opoint_v);
  opoint->set(u"x", Value(a_facing * round(cx2 - cx1)));
  opoint->set(u"y", Value(round(cy1 - cy2)));
  opoint->set(u"z", Value(round(z2 - z1)));

  const bool freeze_ball = a->spawn(opoint_v, entity::turn_face(v->facing()));
  if (!freeze_ball) return false;
  v->enter_frame(gone_frame_info());
  return true;
}

}
}
