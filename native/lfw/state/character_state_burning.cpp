#include "lfw/state/character_state_burning.h"

#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace state {
namespace {

Value index_by(const Value& holder, const std::u16string& k) {
  const Object* o = as_object(holder);
  if (o != nullptr) {
    const Value* p = o->get(k);
    if (p != nullptr) return *p;
  }
  const Array* arr = as_array(holder);
  if (arr != nullptr) {
    const double index = to_number(Value(k));
    if (!(index >= 0) || index >= static_cast<double>(arr->size())) return Value();
    return arr->at(static_cast<size_t>(index));
  }
  return Value();
}

void csb_on_landing(IStateEntity& e, const Value& velocity) {
  const Value landing = e.frame_on_landing();
  if (truthy(landing)) {
    e.enter_frame(landing);
    return;
  }
  if (!truthy(e.bounced())) {
    const Value vy = lfw::field_or(velocity, u"y");
    if (to_number(vy) <= to_number(e.world_dataset(u"cha_bc_tst_spd_y")) ||
        abs(to_number(lfw::field_or(velocity, u"x"))) >
            to_number(e.world_dataset(u"cha_bc_tst_spd_x"))) {
      e.enter_frame_by_id(
          to_string(index_by(index_by(e.data_indexes_bouncing(), u"-1"), u"1")));
      e.set_velocity(Value(NullTag{}), e.world_dataset(u"cha_bc_spd"), Value());
      e.set_bounced(Value(true));
      return;
    }
  }
  e.enter_frame_by_id(to_string(index_by(e.data_indexes_lying(), u"-1")));
}

void csb_enter_body(IStateEntity& e) {
  e.set_bounced(Value(false));
  if (e.has_catcher()) e.catcher_drop_catching();
}

}

CharacterState_Burning::CharacterState_Burning() : CharacterState_Base(Value(static_cast<double>(
                                                           StateEnum::Burning))) {
  enter = [this](IStateEntity& e, const Value& prev_frame) {
    (void)prev_frame;
    this->CharacterState_Base::update(e);
    csb_enter_body(e);
  };
  on_landing = &csb_on_landing;
}

void CharacterState_Burning::update(IStateEntity& e) {
  CharacterState_Base::update(e);
  const Value vx = e.velocity_x();
  if (truthy(vx)) e.set_facing(Value(to_number(vx) > 0 ? -1.0 : 1.0));
}

void CharacterState_Burning::leave(IStateEntity& e, const Value& next_frame) {
  CharacterState_Base::leave(e, next_frame);
  e.set_bounced(Value(false));
}

}
}
