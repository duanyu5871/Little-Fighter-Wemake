#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/collision/ball_frozen.h"
#include "lfw/core/value.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::collision::BallFrozenEnv;
using lfw::collision::IFrozenEntity;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;

std::string render(const Value& v) { return to_ascii(render_value(v)); }
std::string s_of(const std::u16string& s) { return to_ascii(s); }

std::string join(const std::vector<std::string>& xs) {
  std::string out;
  for (size_t i = 0; i < xs.size(); ++i) {
    if (i) out += ",";
    out += xs[i];
  }
  return out;
}

Value make_frame(double cx, double cy, double w, double h) {
  lfw::Object o;
  o.set(u"centerx", Value(cx));
  o.set(u"centery", Value(cy));
  o.set(u"width", Value(w));
  o.set(u"height", Value(h));
  return Value(std::make_shared<lfw::Object>(o));
}

struct Fake : IFrozenEntity {
  std::u16string _id;
  Value _group;
  Value _state;
  Value _type = Value(static_cast<double>(lfw::EntityEnum::Fighter));
  Value _face = Value(1.0);
  double _pos[3] = {0.0, 0.0, 0.0};
  Value _frame = make_frame(0, 0, 0, 0);
  bool _spawn_ok = true;

  explicit Fake(std::u16string id) : _id(std::move(id)) {}

  Value data() const override {
    lfw::Object o;
    o.set(u"type", _type);
    return Value(std::make_shared<lfw::Object>(o));
  }

  Value group() const override { return _group; }
  Value state() const override { return _state; }
  Value frame() const override { return _frame; }
  Value facing() const override { return _face; }

  double position_x() const override { return _pos[0]; }
  double position_y() const override { return _pos[1]; }
  double position_z() const override { return _pos[2]; }

  bool spawn(const Value& opoint, const Value& face) override {
    const Value action = lfw::field_or(opoint, u"action");
    g_log.push_back(s_of(_id) + ":spawn:" + render(lfw::field_or(opoint, u"oid")) + ":" +
                    render(lfw::field_or(opoint, u"kind")) + ":" +
                    render(lfw::field_or(opoint, u"x")) + ":" +
                    render(lfw::field_or(opoint, u"y")) + ":" +
                    render(lfw::field_or(opoint, u"z")) + ":" +
                    render(lfw::field_or(action, u"id")) + ":" + render(face));
    return _spawn_ok;
  }

  void enter_frame(const Value& info) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(lfw::field_or(info, u"id")));
  }
};

Fake g_att(u"A");
Fake g_vic(u"V");
Value g_itr;

void bind() {
  BallFrozenEnv env;
  env.is_ball = [](const IFrozenEntity& e) {
    return lfw::entity::is_ball_data(static_cast<const Fake&>(e).data());
  };
  env.is_fighter = [](const IFrozenEntity& e) {
    return lfw::entity::is_fighter_data(static_cast<const Fake&>(e).data());
  };
  lfw::collision::set_ball_frozen_env(env);
}

std::string side_text(const Fake& f) {
  const std::string id = s_of(f._id);
  return id + ".group=" + render(f._group) + " " + id + ".state=" + render(f._state) + " " + id +
         ".type=" + render(f._type) + " " + id + ".face=" + render(f._face) + " " + id +
         ".pos=" + render(Value(f._pos[0])) + "/" + render(Value(f._pos[1])) + "/" +
         render(Value(f._pos[2])) + " " + id + ".spawn=" + (f._spawn_ok ? "1" : "0");
}

std::string state_text() { return side_text(g_att) + " " + side_text(g_vic); }

void walk_side(Fake& f, const std::string& field, const std::vector<std::string>& t, size_t& i) {
  if (field == "group") {
    f._group = parse_value(t, i);
  } else if (field == "state") {
    f._state = parse_value(t, i);
  } else if (field == "type") {
    f._type = parse_value(t, i);
  } else if (field == "face") {
    f._face = parse_value(t, i);
  } else if (field == "posx") {
    f._pos[0] = trace::to_double(t[i++]);
  } else if (field == "posy") {
    f._pos[1] = trace::to_double(t[i++]);
  } else if (field == "posz") {
    f._pos[2] = trace::to_double(t[i++]);
  } else if (field == "frame") {
    f._frame = parse_value(t, i);
  } else if (field == "spawn") {
    f._spawn_ok = t[i++] == "1";
  } else {
    std::fprintf(stderr, "unknown side field '%s'\n", field.c_str());
    std::exit(2);
  }
}

void run() {
  const bool hit = lfw::collision::handle_ball_frozen(g_att, g_vic, g_itr);
  std::printf("run hit || %s | %s | ret=%s\n", join(g_log).c_str(), state_text().c_str(),
              hit ? "1" : "0");
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_collision_ball_frozen <case-file>\n");
    return 2;
  }
  bind();

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
      if (sub == "itr") {
        g_itr = parse_value(t, i);
      } else if (sub == "a" || sub == "v") {
        const std::string& field = t[i++];
        walk_side(sub == "a" ? g_att : g_vic, field, t, i);
        if (i != t.size()) {
          std::fprintf(stderr, "trailing tokens after side field '%s' at line %d\n", field.c_str(),
                       lineno);
          return 2;
        }
      } else {
        std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      std::printf("env %s\n", sub.c_str());
      continue;
    }
    if (op == "run") {
      run();
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
