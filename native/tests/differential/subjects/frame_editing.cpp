#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/frame_editing.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::map<std::string, lfw::Value> g_objs;
lfw::Value g_costs;

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
    std::fprintf(stderr, "usage: lfw_trace_frame_editing <case-file>\n");
    return 2;
  }
  g_costs = lfw::Value(std::make_shared<lfw::Object>());

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
      if (id == "C") {
        lfw::Object* c = lfw::as_object(g_costs);
        if (c != nullptr) c->set(key, parse_value(t, i));
      } else {
        obj_of(id).set(key, parse_value(t, i));
      }
      if (i != t.size()) {
        std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
        return 2;
      }
    } else if (op == "dump") {
      const std::string id = t[i++];
      emit("D " + id + " " + render(g_objs[id]));
    } else if (op == "kd" || op == "ht" || op == "sq") {
      const std::string id = t[i++];
      const lfw::Value key = parse_value(t, i);
      std::vector<lfw::Value> nexts;
      while (i < t.size()) nexts.push_back(parse_value(t, i));
      lfw::dat_translator::FrameEditing ed(g_objs[id], lfw::as_object(g_costs));
      if (op == "kd") ed.keydown(key, nexts);
      else if (op == "ht") ed.hit(key, nexts);
      else ed.seq(std::holds_alternative<std::u16string>(key)
                      ? std::get<std::u16string>(key)
                      : std::u16string(),
                  nexts);
      emit(op + " " + id + " " + render(g_objs[id]));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
