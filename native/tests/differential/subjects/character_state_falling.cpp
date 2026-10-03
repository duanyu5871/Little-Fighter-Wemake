#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/state/character_state_falling.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Array;
using lfw::Object;
using lfw::Value;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::state::CharacterState_Falling;
using lfw::state::IStateEntity;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;

std::string render(const Value& v) { return to_ascii(render_value(v)); }
std::string s_of(const std::u16string& s) { return to_ascii(s); }
std::string num(double d) { return render(Value(d)); }

std::string join(const std::vector<std::string>& xs) {
  std::string out;
  for (size_t i = 0; i < xs.size(); ++i) {
    if (i) out += ",";
    out += xs[i];
  }
  return out;
}

std::u16string value_text(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

// `fighter.set_velocity(...)` on a fused fighter is observed through the fighter
// object itself, so the log renders it the way the TS side renders its fighters.
std::string fighter_text(const Value& who) {
  Object o;
  o.set(u"key", Value(value_text(who)));
  return render(Value(std::make_shared<Object>(o)));
}

struct FakeEnt : IStateEntity {
  std::u16string _id;
  double _px = 0;
  double _py = 0;
  double _pz = 0;
  Value _dataid;
  Value _frameid;
  Value _onlanding;
  Value _idxbounce;
  Value _idxfalling;
  Value _idxcritical;
  Value _idxlying;
  Value _shaking;
  Value _wait;
  Value _hp;
  Value _facing;
  Value _vx;
  double _vy = 0;
  Value _vz;
  Value _bounced;
  bool _catcher = false;
  Value _fuse;
  Value _fall;
  Value _fallmax;
  Value _defend;
  Value _defendmax;
  Value _rest;
  Value _restmax;
  Value _finj;
  Value _tinj;
  Value _dvals;
  Value _state;

  explicit FakeEnt(std::u16string id) : _id(std::move(id)) {}

  const std::u16string& id() const override { return _id; }
  void position(double& x, double& y, double& z) const override {
    x = _px;
    y = _py;
    z = _pz;
  }
  void set_position(double x, double y, double z) override {
    _px = x;
    _py = y;
    _pz = z;
  }
  double frame_centery() const override { return 0; }
  double frame_height() const override { return 0; }
  double frame_pic_h() const override { return 0; }
  void set_frame(const Value& info) override {
    g_log.push_back(s_of(_id) + ":set_frame:" + render(lfw::field_or(info, u"id")));
  }
  void buffs_set(const std::u16string& key, Buff*) override {
    g_log.push_back(s_of(_id) + ":buffs_set:" + s_of(key));
  }
  void buffs_delete(const std::u16string& key) override {
    g_log.push_back(s_of(_id) + ":buffs_delete:" + s_of(key));
  }
  void set_outline_alpha(double v) override {
    g_log.push_back(s_of(_id) + ":outline_alpha:" + num(v));
  }
  void set_outline_width(double v) override {
    g_log.push_back(s_of(_id) + ":outline_width:" + num(v));
  }
  void set_outline_color(const std::u16string& v) override {
    g_log.push_back(s_of(_id) + ":outline_color:" + render(Value(v)));
  }
  void enter_frame_by_id(const std::u16string& id) override {
    g_log.push_back(s_of(_id) + ":enter_frame_by_id:" + render(Value(id)));
  }
  void attach(bool on) override {
    g_log.push_back(s_of(_id) + ":attach:" + std::string(on ? "1" : "0"));
  }
  Value velocity_x() const override { return _vx; }
  Value velocity_z() const override { return _vz; }
  double velocity_y() const override { return _vy; }
  Value hp() const override { return _hp; }
  Value facing() const override { return _facing; }
  Value wait() const override { return _wait; }
  Value shaking() const override { return _shaking; }
  Value bounced() const override { return _bounced; }
  void set_bounced(const Value& v) override { _bounced = v; }
  Value fall_value() const override { return _fall; }
  void set_fall_value(const Value& v) override { _fall = v; }
  Value fall_value_max() const override { return _fallmax; }
  Value defend_value_max() const override { return _defendmax; }
  void set_defend_value(const Value& v) override { _defend = v; }
  Value resting_max() const override { return _restmax; }
  void set_resting(const Value& v) override { _rest = v; }
  void set_fallinjury(const Value& v) override { _finj = v; }
  void set_throwinjury(const Value& v) override { _tinj = v; }
  Value data_id() const override { return _dataid; }
  Value frame_id() const override { return _frameid; }
  Value frame_info() const override {
    Object o;
    o.set(u"id", _frameid);
    return Value(std::make_shared<Object>(o));
  }
  Value frame_on_landing() const override { return _onlanding; }
  Value data_indexes_bouncing() const override { return _idxbounce; }
  Value data_indexes_falling() const override { return _idxfalling; }
  Value data_indexes_critical_hit() const override { return _idxcritical; }
  Value data_indexes_lying() const override { return _idxlying; }
  void ctrl_reset_key_list() override { g_log.push_back(s_of(_id) + ":ctrl_reset_key_list"); }
  bool has_catcher() const override { return _catcher; }
  void catcher_drop_catching() override {
    g_log.push_back(s_of(_id) + ":catcher_drop_catching");
  }
  void drop_holding() override { g_log.push_back(s_of(_id) + ":drop_holding"); }
  void leave_ground() override { g_log.push_back(s_of(_id) + ":leave_ground"); }
  void dismiss_fusion(const Value& frame_id) override {
    g_log.push_back(s_of(_id) + ":dismiss_fusion:" + render(frame_id));
  }
  Value fuse_bys() const override { return _fuse; }
  void ref_set_velocity(const Value& who, const Value& x, const Value& y,
                        const Value& z) override {
    g_log.push_back(s_of(_id) + ":ref_set_velocity:" + fighter_text(who) + ":" + render(x) +
                    ":" + render(y) + ":" + render(z));
  }
  void handle_ground_velocity_decay() override {
    g_log.push_back(s_of(_id) + ":handle_ground_velocity_decay:" + num(1.0));
  }
  void handle_ground_velocity_decay(double factor) override {
    g_log.push_back(s_of(_id) + ":handle_ground_velocity_decay:" + num(factor));
  }
  Value world_dataset(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":world_dataset:" + s_of(key));
    return lfw::field_or(_dvals, key.c_str());
  }
  void enter_frame(const Value& frame) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(frame));
  }
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" +
                    render(z));
  }
};

FakeEnt* g_ent = nullptr;
Value g_state;
Value g_velocity;
CharacterState_Falling* g_obj = nullptr;

std::string state_text() {
  const FakeEnt& e = *g_ent;
  return "pos=[" + num(e._px) + "," + num(e._py) + "," + num(e._pz) + "] vel=[" +
         render(e._vx) + "," + num(e._vy) + "," + render(e._vz) + "] dataid=" +
         render(e._dataid) + " frameid=" + render(e._frameid) + " hp=" + render(e._hp) +
         " facing=" + render(e._facing) + " bounced=" + render(e._bounced) + " fall=" +
         render(e._fall) + "/" + render(e._fallmax) + " defend=" + render(e._defend) + "/" +
         render(e._defendmax) + " rest=" + render(e._rest) + "/" + render(e._restmax) +
         " finj=" + render(e._finj) + " tinj=" + render(e._tinj);
}

void bind() {
  static BuffEnv keep;
  keep.find_entity = [](const std::u16string&) -> lfw::buff::IBuffEntity* { return nullptr; };
  keep.create_entity = [](const Value&) -> lfw::buff::IBuffEntity* { return nullptr; };
  keep.find_data = [](const std::u16string&) -> Value { return Value(); };
  keep.world_buffs_set = [](const std::u16string&, Buff*) {};
  keep.world_buffs_get = [](const std::u16string&) -> Buff* { return nullptr; };
  keep.create_buff = [](const std::u16string&, const std::u16string&) -> Buff* { return nullptr; };
  lfw::state::StateEnv senv;
  senv.buff_env = &keep;
  lfw::state::set_state_env(senv);
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_character_state_falling <case-file>\n");
    return 2;
  }
  bind();
  g_ent = new FakeEnt(u"E1");
  g_state = Value(0.0);

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;
    g_log.clear();
    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "state") {
        g_state = parse_value(t, i);
      } else if (sub == "pos") {
        const Value v = parse_value(t, i);
        g_ent->_px = lfw::to_number(lfw::field_or(v, u"x"));
        g_ent->_py = lfw::to_number(lfw::field_or(v, u"y"));
        g_ent->_pz = lfw::to_number(lfw::field_or(v, u"z"));
      } else if (sub == "dataid") {
        g_ent->_dataid = parse_value(t, i);
      } else if (sub == "frameid") {
        g_ent->_frameid = parse_value(t, i);
      } else if (sub == "onlanding") {
        g_ent->_onlanding = parse_value(t, i);
      } else if (sub == "idxbounce") {
        g_ent->_idxbounce = parse_value(t, i);
      } else if (sub == "idxfalling") {
        g_ent->_idxfalling = parse_value(t, i);
      } else if (sub == "idxcritical") {
        g_ent->_idxcritical = parse_value(t, i);
      } else if (sub == "idxlying") {
        g_ent->_idxlying = parse_value(t, i);
      } else if (sub == "shaking") {
        g_ent->_shaking = parse_value(t, i);
      } else if (sub == "wait") {
        g_ent->_wait = parse_value(t, i);
      } else if (sub == "hp") {
        g_ent->_hp = parse_value(t, i);
      } else if (sub == "facing") {
        g_ent->_facing = parse_value(t, i);
      } else if (sub == "vx") {
        g_ent->_vx = parse_value(t, i);
      } else if (sub == "vy") {
        g_ent->_vy = trace::to_double(t[i++]);
      } else if (sub == "vz") {
        g_ent->_vz = parse_value(t, i);
      } else if (sub == "bounced") {
        g_ent->_bounced = parse_value(t, i);
      } else if (sub == "catcher") {
        g_ent->_catcher = lfw::truthy(parse_value(t, i));
      } else if (sub == "fuse") {
        g_ent->_fuse = parse_value(t, i);
      } else if (sub == "fall") {
        g_ent->_fall = parse_value(t, i);
      } else if (sub == "fallmax") {
        g_ent->_fallmax = parse_value(t, i);
      } else if (sub == "defend") {
        g_ent->_defend = parse_value(t, i);
      } else if (sub == "defmax") {
        g_ent->_defendmax = parse_value(t, i);
      } else if (sub == "rest") {
        g_ent->_rest = parse_value(t, i);
      } else if (sub == "restmax") {
        g_ent->_restmax = parse_value(t, i);
      } else if (sub == "finj") {
        g_ent->_finj = parse_value(t, i);
      } else if (sub == "tinj") {
        g_ent->_tinj = parse_value(t, i);
      } else if (sub == "dvals") {
        g_ent->_dvals = parse_value(t, i);
      } else if (sub == "vel") {
        g_velocity = parse_value(t, i);
      } else {
        std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      std::printf("env %s\n", sub.c_str());
      continue;
    }
    if (op == "run") {
      const std::string& what = t[i++];
      if (what == "make") {
        delete g_obj;
        g_obj = new CharacterState_Falling(g_state);
        std::printf("run make || %s | s=%s | %s\n", join(g_log).c_str(),
                    render(g_obj->state()).c_str(), state_text().c_str());
      } else if (what == "default") {
        delete g_obj;
        g_obj = new CharacterState_Falling();
        std::printf("run default || %s | s=%s | %s\n", join(g_log).c_str(),
                    render(g_obj->state()).c_str(), state_text().c_str());
      } else if (what == "enter") {
        if (g_obj->enter) g_obj->enter(*g_ent, Value());
        std::printf("run enter || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "update") {
        g_obj->update(*g_ent);
        std::printf("run update || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "landing") {
        if (g_obj->on_landing) g_obj->on_landing(*g_ent, g_velocity);
        std::printf("run landing || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "leave") {
        g_obj->leave(*g_ent, Value());
        std::printf("run leave || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else {
        std::fprintf(stderr, "unknown run '%s' at line %d\n", what.c_str(), lineno);
        return 2;
      }
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  delete g_obj;
  return 0;
}
