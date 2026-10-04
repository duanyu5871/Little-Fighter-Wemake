#include "lfw/entity/entity_state_view.h"

#include <variant>

#include "lfw/entity/entity.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {

const std::u16string& EntityStateView::id() const { return _e.id; }

void EntityStateView::position(double& x, double& y, double& z) const {
  x = _e.position.x;
  y = _e.position.y;
  z = _e.position.z;
}

void EntityStateView::set_position(double x, double y, double z) {
  (void)x;
  (void)y;
  (void)z;
}

double EntityStateView::frame_centery() const { return 0; }

double EntityStateView::frame_height() const { return 0; }

double EntityStateView::frame_pic_h() const { return 0; }

void EntityStateView::set_frame(const Value& info) { (void)info; }

void EntityStateView::buffs_set(const std::u16string& key, buff::Buff* b) {
  _e.buffs[key] = b;
}

void EntityStateView::buffs_delete(const std::u16string& key) { _e.buffs.erase(key); }

void EntityStateView::set_outline_alpha(double v) { _e.set_outline_alpha(v); }

void EntityStateView::set_outline_width(double v) { _e.set_outline_width(v); }

void EntityStateView::set_outline_color(const std::u16string& v) {
  _e.set_outline_color(v);
}

void EntityStateView::enter_frame_by_id(const std::u16string& id) { (void)id; }

void EntityStateView::attach(bool on) { (void)on; }

Value EntityStateView::velocity_x() const { return Value(_e.velocity.x); }

Value EntityStateView::velocity_z() const { return Value(_e.velocity.z); }

double EntityStateView::velocity_y() const { return _e.velocity.y; }

Value EntityStateView::hp() const { return Value(_e.hp()); }

Value EntityStateView::hp_r() const { return Value(_e.hp_r()); }

void EntityStateView::set_hp_r(const Value& v) { _e.set_hp_r(to_number(v)); }

Value EntityStateView::hp_max() const { return Value(_e.hp_max()); }

Value EntityStateView::mp() const { return Value(_e.mp()); }

Value EntityStateView::motionless() const { return Value(_e.motionless); }

void EntityStateView::set_motionless(const Value& v) { _e.motionless = to_number(v); }

Value EntityStateView::shaking() const { return Value(_e.shaking); }

Value EntityStateView::state() const { return _e.state(); }

Value EntityStateView::frame_info() const { return _e.frame; }

Value EntityStateView::prev_frame() const { return _e.get_prev_frame(); }

bool EntityStateView::is_on_ground() const { return _e.is_on_ground; }

Value EntityStateView::team() const { return Value(_e.team()); }

Value EntityStateView::data_type() const { return field_or(_e.data(), u"type"); }

Value EntityStateView::jumping_x() const { return Value(_e.jumping.x); }

}
