#include "lfw/entity/entity_state_view.h"

#include "lfw/entity/entity.h"

namespace lfw {

void EntityStateView::assign_position(double x, double y, double z) {
  _e.position.x = x;
  _e.position.y = y;
  _e.position.z = z;
}

// `frame_centery` / `set_frame` / `buffs_*` / `set_outline_*` / `attach` 等
// `Entity` 面由 `EntityViewBase` 提供。

double EntityStateView::velocity_x() const { return _e.velocity.x; }

Value EntityStateView::velocity_z() const { return Value(_e.velocity.z); }

// `this.ground_y` —— 落地判定的读点（`py > this.ground_y`）。
double EntityStateView::ground_y() const { return _e.ground_y(); }

Value EntityStateView::hp_max() const { return Value(_e.hp_max()); }

Value EntityStateView::motionless() const { return Value(_e.motionless); }

void EntityStateView::set_motionless(const Value& v) { _e.motionless = to_number(v); }

Value EntityStateView::shaking() const { return Value(_e.shaking); }

Value EntityStateView::frame_info() const { return _e.frame; }

Value EntityStateView::prev_frame() const { return _e.get_prev_frame(); }

bool EntityStateView::is_on_ground() const { return _e.is_on_ground; }

Value EntityStateView::jumping_x() const { return Value(_e.jumping.x); }

void EntityStateView::dismiss_fusion(const Value& frame_id) {
  // TS 的 `dismiss_fusion(frame_id: string)`；状态层给的是帧 id（字符串），端口按
  // `String(v)` 的口径转一次。
  _e.dismiss_fusion(to_string(frame_id));
}

Value EntityStateView::world_puppets() const { return _e.world_puppets(); }

Value EntityStateView::world_dataset(const std::u16string& key) const {
  return _e.world_dataset(key);
}

Value EntityStateView::facing() const { return Value(_e.facing); }

void EntityStateView::enter_frame(const Value& frame) { _e.enter_frame(frame); }

void EntityStateView::drop_holding() { _e.drop_holding(); }

void EntityStateView::handle_ground_velocity_decay() {
  _e.handle_ground_velocity_decay();
}

void EntityStateView::handle_ground_velocity_decay(double factor) {
  _e.handle_ground_velocity_decay(factor);
}

// `state::IStateEntity` 的实体面：4BX 由 `EntityViewBase` 顶（那时它实现
// `buff::IBuffEntity`）；4BY 接口退役后要在这里显式转发回去。
void EntityStateView::position(double& x, double& y, double& z) const {
  EntityViewBase::position(x, y, z);
}

void EntityStateView::set_position(double x, double y, double z) {
  EntityViewBase::set_position(x, y, z);
}

void EntityStateView::enter_frame_by_id(const std::u16string& id) {
  EntityViewBase::enter_frame_by_id(id);
}

void EntityStateView::set_outline_color(const std::u16string& v) {
  EntityViewBase::set_outline_color(v);
}

Value EntityStateView::data() const { return EntityViewBase::data(); }

Value EntityStateView::data_type() const { return EntityViewBase::data_type(); }

Value EntityStateView::dataset(const std::u16string& key) const {
  return EntityViewBase::dataset(key);
}

Value EntityStateView::data_indexes_in_the_skys() const {
  return EntityViewBase::data_indexes_in_the_skys();
}

Value EntityStateView::hp() const { return EntityViewBase::hp(); }

Value EntityStateView::hp_r() const { return EntityViewBase::hp_r(); }

void EntityStateView::set_hp(const Value& v) { EntityViewBase::set_hp(v); }

void EntityStateView::set_hp_r(const Value& v) { EntityViewBase::set_hp_r(v); }

void EntityStateView::set_toughness(const Value& v) { EntityViewBase::set_toughness(v); }

Value EntityStateView::mp() const { return EntityViewBase::mp(); }

void EntityStateView::set_mp(const Value& v) { EntityViewBase::set_mp(v); }

Value EntityStateView::mp_max() const { return EntityViewBase::mp_max(); }

Value EntityStateView::state() const { return EntityViewBase::state(); }

Value EntityStateView::team() const { return EntityViewBase::team(); }

void EntityStateView::set_team(const Value& v) { EntityViewBase::set_team(v); }

void EntityStateView::set_velocity(const Value& x, const Value& y, const Value& z) {
  EntityViewBase::set_velocity(x, y, z);
}

double EntityStateView::velocity_y() const { return EntityViewBase::velocity_y(); }

}
