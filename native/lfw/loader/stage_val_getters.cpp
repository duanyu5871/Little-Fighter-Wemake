#include "lfw/loader/stage_val_getters.h"

#include <memory>
#include <string>
#include <utility>

#include "lfw/loader/get_val_from_world.h"
#include "lfw/stage/stage.h"

namespace lfw {
namespace loader {
namespace {

Value v_enemies_cleared(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(const_cast<stage::Stage&>(e).all_fighter_dead() ? 1.0 : 0.0);
}

Value v_dialog_cleared(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(e.dialog_cleared() ? 1.0 : 0.0);
}

Value v_phase_time(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(e.phase_time);
}

Value v_dialog_time(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(e.dialog_time);
}

Value v_press_attack(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(e.lfw()->keys_is_start(u"a") ? 1.0 : 0.0);
}

Value v_press_jump(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(e.lfw()->keys_is_start(u"j") ? 1.0 : 0.0);
}

Value v_press_defend(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(e.lfw()->keys_is_start(u"d") ? 1.0 : 0.0);
}

Value v_press_up(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(e.lfw()->keys_is_start(u"U") ? 1.0 : 0.0);
}

Value v_press_down(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(e.lfw()->keys_is_start(u"D") ? 1.0 : 0.0);
}

Value v_press_left(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(e.lfw()->keys_is_start(u"L") ? 1.0 : 0.0);
}

Value v_press_right(const stage::Stage& e, const std::u16string&, BinOp) {
  return Value(e.lfw()->keys_is_start(u"R") ? 1.0 : 0.0);
}

Value v_broadcast(const stage::Stage& e, const std::u16string&, BinOp) {
  auto arr = std::make_shared<Array>();
  for (const std::u16string& s : e.lfw()->broadcasts_list()) arr->push_back(Value(s));
  return Value(std::move(arr));
}

}  // namespace

ValGetter<stage::Stage> get_val_getter_from_stage(const std::u16string& word) {
  if (word == u"enemies_cleared") return v_enemies_cleared;
  if (word == u"dialog_cleared") return v_dialog_cleared;
  if (word == u"cur_phase_time") return v_phase_time;
  if (word == u"cur_dialog_time") return v_dialog_time;
  if (word == u"press_attack") return v_press_attack;
  if (word == u"press_jump") return v_press_jump;
  if (word == u"press_defend") return v_press_defend;
  if (word == u"press_up") return v_press_up;
  if (word == u"press_down") return v_press_down;
  if (word == u"press_left") return v_press_left;
  if (word == u"press_right") return v_press_right;
  if (word == u"broadcast") return v_broadcast;
  return get_val_from_world<stage::Stage>(word);
}

}  // namespace loader
}  // namespace lfw
