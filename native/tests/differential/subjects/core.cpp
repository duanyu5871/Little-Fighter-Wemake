#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/core/js_num.h"
#include "lfw/core/js_string.h"

#include "trace_util.h"

using trace::Line;
using trace::parse_js_string_literal;
using trace::split_ws;
using trace::to_double;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_core <case-file>\n");
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
    if (const auto hash = raw.find('#'); hash != std::string::npos) raw.erase(hash);

    const std::vector<std::string> tok = split_ws(raw);
    if (tok.empty()) continue;

    const std::string& op = tok[0];

    if (op == "to_number") {
      Line().add(op).add_num(lfw::string_to_number(parse_js_string_literal(tok[1]))).out();

    } else if (op == "round") {
      Line().add(op).add_num(lfw::js_round(to_double(tok[1]))).out();

    } else if (op == "floor") {
      Line().add(op).add_num(lfw::js_floor(to_double(tok[1]))).out();

    } else if (op == "ceil") {
      Line().add(op).add_num(lfw::js_ceil(to_double(tok[1]))).out();

    } else if (op == "abs") {
      Line().add(op).add_num(lfw::js_abs(to_double(tok[1]))).out();

    } else if (op == "to_uint32") {
      Line().add(op).add(lfw::js_to_uint32(to_double(tok[1]))).out();

    } else if (op == "to_int32") {
      Line().add(op).add(lfw::js_to_int32(to_double(tok[1]))).out();

    } else if (op == "bits_roundtrip") {
      const double v = to_double(tok[1]);
      Line().add(op).add_num(lfw::f64_from_bits(lfw::f64_bits(v))).out();

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
