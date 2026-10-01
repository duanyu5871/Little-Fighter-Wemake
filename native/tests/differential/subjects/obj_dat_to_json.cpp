#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/obj_dat_to_json.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::u16string g_text;
lfw::Value g_index;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_obj_dat_to_json <case-file>\n");
    return 2;
  }

  g_index = lfw::Value(std::make_shared<lfw::Object>());

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

    if (op == "reset") {
      g_text.clear();
      g_index = lfw::Value(std::make_shared<lfw::Object>());
    } else if (op == "t") {
      const lfw::Value v = parse_value(t, i);
      const std::u16string* s = std::get_if<std::u16string>(&v);
      if (s == nullptr) {
        std::fprintf(stderr, "line %d: t expects a string literal\n", lineno);
        return 2;
      }
      if (!g_text.empty()) g_text.push_back(u'\n');
      g_text += *s;
      if (i != t.size()) {
        std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
        return 2;
      }
    } else if (op == "i") {
      const std::u16string key = to_u16(t[i++]);
      lfw::Value v = parse_value(t, i);
      lfw::as_object(g_index)->set(key, v);
      if (i != t.size()) {
        std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
        return 2;
      }
    } else if (op == "run") {
      const lfw::dat_translator::ObjDatToJsonResult r =
          lfw::dat_translator::obj_dat_to_json(g_text, g_index);
      if (!r.ok) emit("E " + to_ascii(r.error));
      else emit("R " + render(r.data));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
