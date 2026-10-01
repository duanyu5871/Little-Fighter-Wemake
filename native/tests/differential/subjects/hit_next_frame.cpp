#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/hit_next_frame.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::map<std::string, lfw::Value> g_objs;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

lfw::Object& obj_of(const std::string& id) {
  lfw::Value& v = g_objs[id];
  if (lfw::as_object(v) == nullptr) v = lfw::Value(std::make_shared<lfw::Object>());
  return *lfw::as_object(v);
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_hit_next_frame <case-file>\n");
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

    if (op == "new") {
      g_objs[t[i++]] = lfw::Value(std::make_shared<lfw::Object>());
    } else if (op == "set") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      obj_of(id).set(key, parse_value(t, i));
      if (i != t.size()) {
        std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
        return 2;
      }
    } else if (op == "dump") {
      const std::string id = t[i++];
      emit("D " + id + " " + render(g_objs[id]));
    } else if (op == "turn_back") {
      const std::string id = t[i++];
      lfw::Value back;
      if (i < t.size()) back = parse_value(t, i);
      lfw::Value& v = g_objs[id];
      lfw::dat_translator::hit_next_frame_turn_back(v, back);
      emit("turn_back " + id + " " + render(v));
    } else if (op == "drink" || op == "super_punch" || op == "punch" || op == "jump" ||
               op == "defend" || op == "weapon_atk" || op == "jump_atk") {
      lfw::Value out;
      if (op == "drink") out = lfw::dat_translator::hit_next_frame_drink();
      else if (op == "super_punch") out = lfw::dat_translator::hit_next_frame_super_punch();
      else if (op == "punch") out = lfw::dat_translator::hit_next_frame_punch();
      else if (op == "jump") out = lfw::dat_translator::hit_next_frame_jump();
      else if (op == "defend") out = lfw::dat_translator::hit_next_frame_defend();
      else if (op == "weapon_atk") out = lfw::dat_translator::hit_next_frame_weapon_atk();
      else out = lfw::dat_translator::hit_next_frame_jump_atk();
      emit(op + " " + render(out));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
