#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/ball_bdy.h"

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

lfw::Value value_of(const std::vector<std::string>& t, size_t& i) { return parse_value(t, i); }

lfw::Object& obj_of(const std::string& id) {
  lfw::Value& v = g_objs[id];
  if (lfw::as_object(v) == nullptr) v = lfw::Value(std::make_shared<lfw::Object>());
  return *lfw::as_object(v);
}

lfw::Value field_of(const std::string& id, const char16_t* key) {
  const lfw::Object* o = lfw::as_object(g_objs[id]);
  if (o == nullptr) return lfw::Value();
  const lfw::Value* p = o->get(std::u16string(key));
  return p != nullptr ? *p : lfw::Value();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_ball_bdy <case-file>\n");
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
      obj_of(id).set(key, value_of(t, i));
    } else if (op == "dump") {
      const std::string id = t[i++];
      emit("D " + id + " " + render(g_objs[id]));
    } else if (op == "bb20" || op == "bb30") {
      const std::string id = t[i++];
      lfw::Value& v = g_objs[id];
      const lfw::Value out = op == "bb20"
                                 ? lfw::dat_translator::cook_ball_bdy_get_hit_to_frame_20(v)
                                 : lfw::dat_translator::cook_ball_bdy_get_hit_to_frame_30(v);
      emit((op == "bb20" ? "B20 " : "B30 ") + id + " " + render(out));
    } else if (op == "bedit") {
      const std::string id = t[i++];
      lfw::Value bdy = field_of(id, u"bdy");
      lfw::dat_translator::edit_bdy_edit(bdy, field_of(id, u"fields"));
      obj_of(id).set(u"bdy", bdy);
      emit("BE " + id + " " + render(g_objs[id]));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
