#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/state/state_base_proxy.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Array;
using lfw::Object;
using lfw::Value;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::state::IStateEntity;
using lfw::state::State_Base;
using lfw::state::State_15;
using lfw::state::State_Frozen;
using lfw::state::StateBase_Proxy;
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

struct FakeEnt : IStateEntity {
  std::u16string _id;
  Value _data;
  Value _indexes;
  Value _frames;
  Value _onlanding;
  Value _wdata;
  Value _state;
  Value _hbtype;
  bool _catcher = false;
  bool _on_ground = false;
  double _hp = 50;
  double _px = 0;
  double _py = 0;
  double _pz = 0;
  Value _vx;
  Value _vz;

  explicit FakeEnt(std::u16string id) : _id(std::move(id)) {}

  const std::u16string& id() const override { return _id; }
  Value data() const override { return _data; }
  Value state() const override { return _state; }
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
  Value hp() const override { return Value(_hp); }
  void set_hp(const Value& v) override { _hp = lfw::to_number(v); }
  Value data_frames() const override { return _frames; }
  Value frame_on_landing() const override { return _onlanding; }
  bool is_on_ground() const override { return _on_ground; }
  bool has_catcher() const override { return _catcher; }
  void catcher_drop_catching() override {
    g_log.push_back(s_of(_id) + ":catcher_drop_catching");
  }
  void drop_holding() override { g_log.push_back(s_of(_id) + ":drop_holding"); }
  Value holding_base_type() const override { return _hbtype; }
  Value world_dataset(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":world_dataset:" + s_of(key));
    return lfw::field_or(_wdata, key.c_str());
  }
  bool has_data_indexes() const override { return lfw::truthy(_indexes); }
  Value data_indexes_on_ground() const override {
    return lfw::field_or(_indexes, u"on_ground");
  }
  Value data_indexes_throwings() const override {
    return lfw::field_or(_indexes, u"throwings");
  }
  Value data_indexes_in_the_skys() const override {
    return lfw::field_or(_indexes, u"in_the_skys");
  }
  Value data_indexes_default() const override {
    return lfw::field_or(_indexes, u"default");
  }
  Value data_indexes_heavy_obj_walk() const override {
    return lfw::field_or(_indexes, u"heavy_obj_walk");
  }
  Value data_indexes_landing_1() const override {
    return lfw::field_or(_indexes, u"landing_1");
  }
  Value data_indexes_landing_2() const override {
    return lfw::field_or(_indexes, u"landing_2");
  }
  Value data_indexes_bouncing() const override {
    return lfw::field_or(_indexes, u"bouncing");
  }
  Value data_indexes_lying() const override {
    return lfw::field_or(_indexes, u"lying");
  }
  Value data_indexes_falling() const override {
    return lfw::field_or(_indexes, u"falling");
  }
  Value dataset(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":dataset:" + s_of(key));
    return Value();
  }
  void enter_frame(const Value& frame) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(frame));
  }
  void enter_frame_by_id_fallback(const std::u16string& id, bool fallback) override {
    g_log.push_back(s_of(_id) + ":enter_frame_by_id_fallback:" + render(Value(id)) + ":" +
                    std::string(fallback ? "1" : "0"));
  }
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" +
                    render(z));
  }
  void handle_ground_velocity_decay() override {
    g_log.push_back(s_of(_id) + ":handle_ground_velocity_decay");
  }
  void set_shaking(const Value& v) override {
    g_log.push_back(s_of(_id) + ":set_shaking:" + render(v));
  }
  void set_motionless(const Value& v) override {
    g_log.push_back(s_of(_id) + ":set_motionless:" + render(v));
  }
  void play_sound(const Value& sounds) override {
    g_log.push_back(s_of(_id) + ":play_sound:" + render(sounds));
  }
  void apply_opoints(const std::vector<Value>& opoints) override {
    g_log.push_back(s_of(_id) + ":apply_opoints:" + std::to_string(opoints.size()));
  }
};

FakeEnt* g_ent = nullptr;
Value g_state;
Value g_velocity;
Value g_rid;
StateBase_Proxy* g_obj = nullptr;
std::u16string g_cls = u"proxy";

std::string state_text() {
  const FakeEnt& e = *g_ent;
  return "hp=" + num(e._hp) + " state=" + render(g_obj->state()) + " pos=[" + num(e._px) +
         ":" + num(e._py) + ":" + num(e._pz) + "]";
}

void bind() {
  static BuffEnv keep;
  keep.find_entity = [](const std::u16string&) -> lfw::buff::IBuffEntity* { return nullptr; };
  keep.create_entity = [](const Value&) -> lfw::buff::IBuffEntity* { return nullptr; };
  keep.find_data = [](const std::u16string&) -> Value { return Value(); };
  keep.world_buffs_set = [](const std::u16string&, Buff*) {};
  keep.world_buffs_get = [](const std::u16string&) -> Buff* { return nullptr; };
  keep.create_buff = [](const std::u16string& kind, const std::u16string& id) -> Buff* {
    g_log.push_back("create_buff:" + s_of(kind) + ":" + s_of(id));
    return nullptr;
  };
  lfw::state::StateEnv senv;
  senv.buff_env = &keep;
  lfw::state::set_state_env(senv);
}

std::u16string value_text(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

std::string ret_line(const char* what, const Value& v, const std::vector<std::string>& log) {
  return "run " + std::string(what) + " || " + join(log) + " | r=" + render(v) + " | " +
         state_text();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_state_base_proxy <case-file>\n");
    return 2;
  }
  bind();
  g_ent = new FakeEnt(u"E1");

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
      if (sub == "cls") {
        g_cls = value_text(parse_value(t, i));
      } else if (sub == "state") {
        g_state = parse_value(t, i);
      } else if (sub == "data") {
        g_ent->_data = parse_value(t, i);
      } else if (sub == "indexes") {
        g_ent->_indexes = parse_value(t, i);
      } else if (sub == "frames") {
        g_ent->_frames = parse_value(t, i);
      } else if (sub == "onlanding") {
        g_ent->_onlanding = parse_value(t, i);
      } else if (sub == "wdata") {
        g_ent->_wdata = parse_value(t, i);
      } else if (sub == "vstate") {
        g_ent->_state = parse_value(t, i);
      } else if (sub == "hbtype") {
        g_ent->_hbtype = parse_value(t, i);
      } else if (sub == "catcher") {
        g_ent->_catcher = lfw::truthy(parse_value(t, i));
      } else if (sub == "onground") {
        g_ent->_on_ground = lfw::truthy(parse_value(t, i));
      } else if (sub == "hp") {
        g_ent->_hp = trace::to_double(t[i++]);
      } else if (sub == "pos") {
        const Value v = parse_value(t, i);
        g_ent->_px = lfw::to_number(lfw::field_or(v, u"x"));
        g_ent->_py = lfw::to_number(lfw::field_or(v, u"y"));
        g_ent->_pz = lfw::to_number(lfw::field_or(v, u"z"));
      } else if (sub == "vx") {
        g_ent->_vx = parse_value(t, i);
      } else if (sub == "vz") {
        g_ent->_vz = parse_value(t, i);
      } else if (sub == "vel") {
        g_velocity = parse_value(t, i);
      } else if (sub == "rid") {
        g_rid = parse_value(t, i);
      } else if (sub == "rxyz") {
        const Value v = parse_value(t, i);
        g_ent->_px = lfw::to_number(lfw::field_or(v, u"x"));
        g_ent->_py = lfw::to_number(lfw::field_or(v, u"y"));
        g_ent->_pz = lfw::to_number(lfw::field_or(v, u"z"));
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
        if (g_cls == u"15") {
          g_obj = new State_15();
        } else if (g_cls == u"frozen") {
          g_obj = new State_Frozen(g_state);
        } else {
          g_obj = new StateBase_Proxy(g_state);
        }
        std::printf("run make || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "default") {
        delete g_obj;
        g_obj = new State_Frozen();
        std::printf("run default || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "update") {
        g_obj->update(*g_ent);
        std::printf("run update || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "leave") {
        g_obj->leave(*g_ent, Value());
        std::printf("run leave || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "restrict") {
        g_obj->on_restrict(*g_ent, g_ent->_px, g_ent->_py, g_ent->_pz);
        std::printf("run restrict || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "preupdate") {
        if (g_obj->pre_update) g_obj->pre_update(*g_ent);
        std::printf("run preupdate || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "enter") {
        if (g_obj->enter) g_obj->enter(*g_ent, Value());
        std::printf("run enter || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "dead") {
        if (g_obj->on_dead) g_obj->on_dead(*g_ent);
        std::printf("run dead || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "landing") {
        if (g_obj->on_landing) g_obj->on_landing(*g_ent, g_velocity);
        std::printf("run landing || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "leaveground") {
        if (g_obj->on_leave_ground) g_obj->on_leave_ground(*g_ent);
        std::printf("run leaveground || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "gravity") {
        const Value v = g_obj->get_gravity ? g_obj->get_gravity(*g_ent) : Value();
        std::printf("%s\n", ret_line("gravity", v, g_log).c_str());
      } else if (what == "auto") {
        const Value v = g_obj->get_auto_frame ? g_obj->get_auto_frame(*g_ent) : Value();
        std::printf("%s\n", ret_line("auto", v, g_log).c_str());
      } else if (what == "sdf") {
        const Value v =
            g_obj->get_sudden_death_frame ? g_obj->get_sudden_death_frame(*g_ent) : Value();
        std::printf("%s\n", ret_line("sdf", v, g_log).c_str());
      } else if (what == "cef") {
        const Value v =
            g_obj->get_caught_end_frame ? g_obj->get_caught_end_frame(*g_ent) : Value();
        std::printf("%s\n", ret_line("cef", v, g_log).c_str());
      } else if (what == "ffbi") {
        const Value v =
            g_obj->find_frame_by_id ? g_obj->find_frame_by_id(*g_ent, g_rid) : Value();
        std::printf("%s\n", ret_line("ffbi", v, g_log).c_str());
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
