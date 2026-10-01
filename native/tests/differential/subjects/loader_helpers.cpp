#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/make_buring_smoke.h"
#include "lfw/loader/preprocess_pic.h"
#include "lfw/loader/preprocess_stage.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_loader_helpers <case-file>\n");
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

    if (op == "b") {
      lfw::Value v = parse_value(t, i);
      emit("b " + render(lfw::dat_translator::make_buring_smoke(
                  static_cast<int>(std::get<double>(v)))));
      if (i != t.size()) {
        std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
        return 2;
      }
    } else if (op == "pic" || op == "fpic" || op == "wp" || op == "phase" || op == "stage") {
      lfw::Value v = parse_value(t, i);
      lfw::Value out;
      if (op == "pic") out = lfw::loader::preprocess_pic(v);
      else if (op == "fpic") out = lfw::loader::preprocess_frame_pic(v);
      else if (op == "wp") out = lfw::loader::preprocess_wpoint(v);
      else if (op == "phase") out = lfw::loader::preprocess_stage_phase(v);
      else out = lfw::loader::preprocess_stage(v);
      emit(op + " " + render(out));
      if (i != t.size()) {
        std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
        return 2;
      }
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
