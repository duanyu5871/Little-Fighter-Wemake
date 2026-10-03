#include "lfw/state/character_state_falling.h"

#include <cstddef>
#include <memory>

#include "lfw/entity/find_frame_direction.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace state {
namespace {

// JS `obj[key]`: objects by key, everything else undefined.
Value js_at(const Value& holder, const Value& key) {
  const Object* o = as_object(holder);
  if (o == nullptr) return Value();
  const Value* p = o->get(to_string(key));
  return p != nullptr ? *p : Value();
}

// JS `value[i]`: arrays by position (out of range is undefined), objects by key.
Value js_at_index(const Value& holder, double idx) {
  const Array* arr = as_array(holder);
  if (arr != nullptr) {
    if (idx < 0 || idx >= static_cast<double>(arr->size())) return Value();
    return arr->at(static_cast<size_t>(idx));
  }
  return js_at(holder, Value(idx));
}

double value_length(const Value& v) {
  const Array* arr = as_array(v);
  if (arr != nullptr) return static_cast<double>(arr->size());
  const Object* o = as_object(v);
  if (o != nullptr) {
    const Value* p = o->get(u"length");
    if (p != nullptr) return to_number(*p);
  }
  return 0;
}

const std::u16string* as_text(const Value& v) {
  return std::get_if<std::u16string>(&v);
}

void csf_on_landing(IStateEntity& e, const Value& velocity) {
  const Value landing = e.frame_on_landing();
  if (truthy(landing)) {
    e.enter_frame(landing);
    return;
  }
  const Value facing = e.facing();
  const Value frame = e.frame_info();
  double d = entity::find_direction(frame, e.data_indexes_bouncing());
  if (!d) d = entity::find_direction(frame, e.data_indexes_falling());
  if (!d) d = entity::find_direction(frame, e.data_indexes_critical_hit());
  if (!d) d = to_number(facing);
  const Value vy = lfw::field_or(velocity, u"y");
  const Value vx = lfw::field_or(velocity, u"x");
  if (!truthy(e.bounced()) &&
      (to_number(vy) <= to_number(e.world_dataset(u"cha_bc_tst_spd_y")) ||
       abs(to_number(vx)) > to_number(e.world_dataset(u"cha_bc_tst_spd_x")))) {
    const Value bouncing = e.data_indexes_bouncing();
    e.enter_frame_by_id(to_string(js_at_index(js_at(bouncing, Value(d)), 1)));
    e.set_velocity(Value(NullTag{}), e.world_dataset(u"cha_bc_spd"), Value());
    e.set_bounced(Value(true));
  } else {
    const Value lying = e.data_indexes_lying();
    e.enter_frame_by_id(to_string(js_at(lying, Value(d))));
  }
}

}

CharacterState_Falling::CharacterState_Falling(Value state)
    : CharacterState_Base(std::move(state)) {
  enter = [this](IStateEntity& e, const Value& prev_frame) {
    (void)prev_frame;
    e.set_bounced(Value(false));
    e.ctrl_reset_key_list();
    const std::u16string id = to_string(e.data_id());
    bool need_cache = _bouncing_frames_map.find(id) == _bouncing_frames_map.end();
    if (need_cache) {
      const Value bouncing = e.data_indexes_bouncing();
      need_cache = truthy(bouncing);
      if (need_cache) {
        std::set<std::u16string> frames;
        const Value plus = js_at(bouncing, Value(1.0));
        const Value minus = js_at(bouncing, Value(-1.0));
        const Array* a = as_array(plus);
        if (a != nullptr) {
          for (size_t i = 0; i < a->size(); ++i) frames.insert(to_string(a->at(i)));
        }
        const Array* b = as_array(minus);
        if (b != nullptr) {
          for (size_t i = 0; i < b->size(); ++i) frames.insert(to_string(b->at(i)));
        }
        _bouncing_frames_map[id] = std::move(frames);
      }
    }
    if (e.has_catcher()) e.catcher_drop_catching();
    e.drop_holding();

    if (to_number(e.hp()) <= 0) {
      const Value fuse_bys = e.fuse_bys();
      if (value_length(fuse_bys) > 0) {
        const Value vx = e.velocity_x();
        const double vy = e.velocity_y();
        const Value vz = e.velocity_z();
        double next_vx = to_number(vx);
        const Array* arr = as_array(fuse_bys);
        for (size_t i = 0; arr != nullptr && i < arr->size(); ++i) {
          next_vx = next_vx * -1.0;
          e.ref_set_velocity(arr->at(i), Value(next_vx), Value(vy), Value(vz));
        }
        e.dismiss_fusion(e.frame_id());
      }
    }
    e.leave_ground();
  };
  on_landing = &csf_on_landing;
}

bool CharacterState_Falling::is_bouncing_frame(IStateEntity& e) {
  const auto it = _bouncing_frames_map.find(to_string(e.data_id()));
  if (it == _bouncing_frames_map.end()) return false;
  const Value fid = e.frame_id();
  const std::u16string* text = as_text(fid);
  if (text == nullptr) return false;
  return it->second.find(*text) != it->second.end();
}

void CharacterState_Falling::update(IStateEntity& e) {
  if (to_number(e.shaking()) > 0) return;
  if (is_bouncing_frame(e)) {
    update_bouncing(e);
  } else {
    update_falling(e);
  }
}

void CharacterState_Falling::update_bouncing(IStateEntity& e) {
  e.handle_ground_velocity_decay(0.7);
}

void CharacterState_Falling::update_falling(IStateEntity& e) {
  if (to_number(e.wait()) <= 0) {
    const Value vx = e.velocity_x();
    const double vy = e.velocity_y();
    double falling_frame_idx = 1;
    if (vy > 3) falling_frame_idx = 0;
    if (vy < -3) falling_frame_idx = 2;
    const double direction = to_number(vx) / to_number(e.facing()) > 0 ? 1 : -1;
    Object frame;
    frame.set(u"id",
              js_at_index(js_at(e.data_indexes_falling(), Value(direction)), falling_frame_idx));
    e.enter_frame(Value(std::make_shared<Object>(frame)));
  }
}

void CharacterState_Falling::leave(IStateEntity& e, const Value& next_frame) {
  State_Base::leave(e, next_frame);
  e.set_bounced(Value(false));
  e.set_fall_value(e.fall_value_max());
  e.set_defend_value(e.defend_value_max());
  e.set_resting(e.resting_max());
  e.set_fallinjury(Value(0.0));
  e.set_throwinjury(Value(0.0));
}

}
}
