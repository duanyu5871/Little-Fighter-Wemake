#include <cstdint>
#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "lfw/cases.h"
#include "lfw/core/js_num.h"
#include "lfw/utils/math/mersenne_twister.h"

#include "trace_util.h"

using trace::bits_hex;
using trace::esc;
using trace::hex16;
using trace::key_of;
using trace::Line;
using trace::parse_value;
using trace::q_bits;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
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
      const bool dbg = tok.size() > 2 && tok[2] == "d";
      mt.reset(seed, dbg);
      Line out;
      out.add("run").add(bits_hex(seed));
      if (dbg) out.add("debug=1");
      out.out();

    } else if (op == "state") {
      Line().add("state").add(hex16(mt.state_hash())).out();

    } else if (op == "mark") {
      mt.mark = key_of(tok[1]);
      Line().add("mark").add(esc(mt.mark)).out();

    } else if (op == "debug") {
      mt.debugging = (tok[1] == "1");
      Line().add("debug").add(mt.debugging ? "1" : "0").out();

    } else if (op == "case") {
      std::vector<lfw::Value> args;
      size_t i = 1;
      while (i < tok.size()) args.push_back(parse_value(tok, i));
      mt.log_case(args);
      Line().add("case").add(args.size()).out();

    } else if (op == "cases") {
      const std::u16string text = lfw::mt_cases().submit();
      Line().add("cases").add(esc(text)).add(lfw::mt_cases().cases().size()).out();

    } else if (op == "cinfo") {
      Line()
          .add("cinfo")
          .add(esc(lfw::mt_cases().name()))
          .add(esc(lfw::mt_cases().separator()))
          .out();

    } else if (op == "creset") {
      lfw::mt_cases().reset();
      Line().add("creset").add(lfw::mt_cases().cases().size()).out();

    } else if (op == "pure") {
      const lfw::MersenneTwisterInfo p = mt.pure();
      Line out;
      out.add("pure")
          .add(hex16(p.matrix))
          .add(hex16(p.upper_mask))
          .add(hex16(p.lower_mask))
          .add(p.index)
          .add(bits_hex(p.seed))
          .add(p.times)
          .add(p.mt.size())
          .add(hex16(p.mt[0]))
          .add(hex16(p.mt[1]))
          .add(hex16(p.mt[623]))
          .add(esc(p.mark));
      out.out();

    } else if (op == "load") {
      const std::string& field = tok[1];
      lfw::MersenneTwisterInfo info = mt.pure();
      const double v = tok.size() > 2 ? to_double(tok[2]) : 0.0;
      if (field == "matrix") info.matrix = lfw::js_to_uint32(v);
      else if (field == "upper") info.upper_mask = lfw::js_to_uint32(v);
      else if (field == "lower") info.lower_mask = lfw::js_to_uint32(v);
      else if (field == "index") info.index = static_cast<int>(v);
      else if (field == "seed") info.seed = v;
      else if (field == "times") info.times = static_cast<uint64_t>(v);
      else if (field == "mt") info.mt[to_long(tok[2])] = lfw::js_to_uint32(to_double(tok[3]));
      else if (field == "mark") info.mark = key_of(tok[2]);
      else {
        std::fprintf(stderr, "line %d: unknown load field '%s'\n", lineno, field.c_str());
        return 2;
      }
      mt.load(info);

      Line out;
      out.add("load");
      for (size_t i = 1; i < tok.size(); ++i) out.add(tok[i]);
      out.add(hex16(mt.state_hash())).add(esc(mt.mark));
      out.out();

    } else if (op == "pickv" || op == "takev") {
      size_t i = 1;
      lfw::Value a = parse_value(tok, i);
      const lfw::Value v = (op == "takev") ? mt.take_value(a) : mt.pick_value(a);
      Line out;
      out.add(op).add(to_ascii(render_value(v))).add(to_ascii(render_value(a)));
      out.out();

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

      Line out;
      out.add(op).add_opt(v.has_value(), v.value_or(0.0)).add(
          static_cast<unsigned long long>(arr.size()));
      for (double item : arr) out.add_qbits(item);
      out.out();

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
