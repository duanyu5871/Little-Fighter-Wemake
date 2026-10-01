#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/defines/labels.h"

#include "trace_util.h"

namespace {

using trace::Line;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void out_one(const char* tag, const lfw::Value& v) {
  Line out;
  out.add(std::string_view(tag)).add(render(v));
  out.out();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_labels <case-file>\n");
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

    if (op == "dump") {
      const std::map<std::u16string, std::u16string> m = lfw::defines::hit_flag_name_map();
      for (const std::pair<const std::u16string, std::u16string>& kv : m) {
        Line out;
        out.add(std::string_view("M")).add(render(lfw::Value(kv.first))).add(render(lfw::Value(kv.second)));
        out.out();
      }
    } else {
      const lfw::Value v = parse_value(t, i);
      if (op == "hit") out_one("H", lfw::Value(lfw::defines::get_hit_flag_name(v)));
      else if (op == "hitfull") out_one("HF", lfw::Value(lfw::defines::get_hit_flag_full_name(v)));
      else if (op == "hitdesc") out_one("HD", lfw::Value(lfw::defines::get_hit_flag_desc(v)));
      else if (op == "bdy") out_one("B", lfw::defines::bdy_kind_name(v));
      else if (op == "bdyfull") out_one("BF", lfw::Value(lfw::defines::bdy_kind_full_name(v)));
      else if (op == "wp") out_one("W", lfw::defines::wpoint_kind_name(v));
      else if (op == "wpfull") out_one("WF", lfw::Value(lfw::defines::wpoint_kind_full_name(v)));
      else {
        std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
        return 2;
      }
    }
  }

  return 0;
}
