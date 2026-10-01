#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/cookers.h"
#include "lfw/dat_translator/frame_behavior.h"
#include "lfw/dat_translator/make_frame_state.h"
#include "lfw/dat_translator/next_frame.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::map<std::string, lfw::Value> g_objs;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& s) { std::printf("%s\n", s.c_str()); }

lfw::Value value_of(const std::vector<std::string>& t, size_t& i) {
  const std::string& tok = t[i];
  if (tok == "nan") {
    ++i;
    return lfw::Value(std::numeric_limits<double>::quiet_NaN());
  }
  if (tok == "inf") {
    ++i;
    return lfw::Value(std::numeric_limits<double>::infinity());
  }
  if (tok == "ninf") {
    ++i;
    return lfw::Value(-std::numeric_limits<double>::infinity());
  }
  return parse_value(t, i);
}

lfw::Object& obj_of(const std::string& id) {
  lfw::Value& v = g_objs[id];
  if (lfw::as_object(v) == nullptr) v = lfw::Value(std::make_shared<lfw::Object>());
  return *lfw::as_object(v);
}

const lfw::Value kNoFrame;

void dispatch_fb(const std::string& fn, lfw::Value& v, std::optional<lfw::Value> x,
                 std::optional<lfw::Value> y) {
  using namespace lfw::dat_translator;
  if (fn == "bat_chase_start") make_fb_bat_chase_start(v);
  else if (fn == "bat_chase") make_fb_bat_chase(v);
  else if (fn == "boomerang") make_fb_boomerang(v);
  else if (fn == "chasing_same_enemy") make_fb_chasing_same_enemy(v, x.has_value() ? to_string(*x) : std::u16string(u"x"));
  else if (fn == "dennis_chase") make_fb_dennis_chase(v);
  else if (fn == "firzen_disater_start") make_fb_firzen_disater_start(v, x, y);
  else if (fn == "firzen_volcano_start") make_fb_firzen_volcano_start(v, x, y);
  else if (fn == "jan_angle_blessing") make_fb_jan_angle_blessing(v);
  else if (fn == "jan_chase_start") make_fb_jan_chase_start(v, x, y);
  else if (fn == "jan_chaseh_start") make_fb_jan_chaseh_start(v, x, y);
  else if (fn == "john_chase_leaving") make_fb_john_chase_leaving(v);
  else if (fn == "john_chase") make_fb_john_chase(v);
  else if (fn == "julian_ball_start") make_fb_julian_ball_start(v);
  else if (fn == "julian_ball") make_fb_julian_ball(v);
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_cookers <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::string line = trace::strip_comment(raw);
    const std::vector<std::string> t = split_ws(line);
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    if (op == "nf") {
      const lfw::Value id = value_of(t, i);
      const std::u16string zero_as = to_u16(t[i++]);
      emit("NF " + render(lfw::dat_translator::get_next_frame_by_raw_id(id, zero_as, u"", nullptr)));
    } else if (op == "new") {
      g_objs[t[i++]] = lfw::Value(std::make_shared<lfw::Object>());
    } else if (op == "set") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      obj_of(id).set(key, value_of(t, i));
    } else if (op == "setarr") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      lfw::Array arr;
      while (i < t.size()) arr.push_back(value_of(t, i));
      obj_of(id).set(key, lfw::Value(std::make_shared<lfw::Array>(arr)));
    } else if (op == "setobj") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      obj_of(id).set(key, g_objs[t[i++]]);
    } else if (op == "dump") {
      const std::string id = t[i++];
      emit("D " + id + " " + render(g_objs[id]));
    } else if (op == "copo") {
      const std::string id = t[i++];
      const std::string fid = t[i++];
      lfw::Value& v = g_objs[id];
      lfw::dat_translator::cook_opoint(v, g_objs[fid]);
      emit("CO " + id + " " + render(v));
    } else if (op == "mfstate") {
      const std::string id = t[i++];
      lfw::Value& v = g_objs[id];
      lfw::dat_translator::make_frame_state(v);
      emit("MS " + id + " " + render(v));
    } else if (op == "fb") {
      const std::string id = t[i++];
      const std::string fn = t[i++];
      lfw::Value& v = g_objs[id];
      std::optional<lfw::Value> x;
      std::optional<lfw::Value> y;
      if (t[i] != "-") x = value_of(t, i);
      else ++i;
      if (t[i] != "-") y = value_of(t, i);
      else ++i;
      dispatch_fb(fn, v, x, y);
      emit("FB " + id + " " + render(v));
    } else if (op == "fbd") {
      const std::string id = t[i++];
      const std::string oid = t[i++];
      lfw::Value& v = g_objs[id];
      lfw::dat_translator::make_frame_behavior(v, to_u16(oid));
      emit("FBD " + id + " " + render(v));
    } else if (op == "bdy" || op == "wp" || op == "cp" || op == "itr" || op == "fsitr") {
      const std::string id = t[i++];
      lfw::Value& v = g_objs[id];
      if (op == "bdy") lfw::dat_translator::cook_bdy(v, kNoFrame);
      else if (op == "wp") lfw::dat_translator::cook_wpoint(v, kNoFrame);
      else if (op == "cp") lfw::dat_translator::cook_cpoint(v, kNoFrame);
      else if (op == "itr") lfw::dat_translator::cook_itr(v, kNoFrame);
      else lfw::dat_translator::float_scaling_itr(v);
      const char* tag = op == "bdy" ? "CB" : op == "wp" ? "CW" : op == "cp" ? "CC" : op == "itr" ? "CI" : "FS";
      emit(std::string(tag) + " " + id + " " + render(v));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
