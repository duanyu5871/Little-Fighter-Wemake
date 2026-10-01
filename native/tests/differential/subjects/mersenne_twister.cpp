#include <cstdint>
#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/js_num.h"
#include "lfw/utils/math/mersenne_twister.h"

#include "trace_util.h"

using trace::bits_hex;
using trace::hex16;
using trace::Line;
using trace::q_bits;
using trace::split_ws;
using trace::to_double;
using trace::to_long;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_mt_trace <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  lfw::MersenneTwister mt(0.0);

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;

    raw = trace::strip_comment(raw);

    const std::vector<std::string> tok = split_ws(raw);
    if (tok.empty()) continue;

    const std::string& op = tok[0];

    if (op == "seed") {
      if (tok.size() < 2) {
        std::fprintf(stderr, "line %d: bad seed\n", lineno);
        return 2;
      }
      const double seed = to_double(tok[1]);
      mt.reset(seed);
      Line().add("run").add(bits_hex(seed)).out();

    } else if (op == "state") {
      Line().add("state").add(hex16(mt.state_hash())).out();

    } else if (op == "int") {
      const long count = to_long(tok[1]);
      for (long i = 0; i < count; ++i) {
        Line().add("int").add(mt.next_int()).out();
      }

    } else if (op == "float") {
      const long count = to_long(tok[1]);
      for (long i = 0; i < count; ++i) {
        Line().add("float").add(q_bits(mt.next_float())).out();
      }

    } else if (op == "range") {
      const double min = to_double(tok[1]);
      const double max = to_double(tok[2]);
      const long count = to_long(tok[3]);
      for (long i = 0; i < count; ++i) {
        const double r = mt.range(min, max);
        Line()
            .add("range")
            .add(bits_hex(min))
            .add(bits_hex(max))
            .add(q_bits(r))
            .out();
      }

    } else if (op == "pick" || op == "take") {
      std::vector<double> arr;
      arr.reserve(tok.size());
      for (size_t i = 1; i < tok.size(); ++i) arr.push_back(to_double(tok[i]));

      const bool is_take = (op == "take");
      const std::optional<double> v = is_take ? mt.take(arr) : mt.pick(arr);

      Line()
          .add(op)
          .add_opt(v.has_value(), v.value_or(0.0))
          .add(static_cast<unsigned long long>(arr.size()))
          .out();

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
