#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "lfw/utils/cross_bounding.h"
#include "lfw/utils/easing/ease_in_out_quint.h"
#include "lfw/utils/easing/ease_in_out_sine.h"
#include "lfw/utils/easing/ease_linearity.h"
#include "lfw/utils/times.h"
#include "lfw/utils/type_cast.h"
#include "lfw/utils/type_check.h"
#include "lfw/utils/utf8.h"

#include "trace_util.h"

using trace::Line;
using trace::split_ws;
using trace::to_double;

namespace {

double arg(const std::vector<std::string>& tok, size_t i, double fallback) {
  return i < tok.size() ? to_double(tok[i]) : fallback;
}

bool has(const std::vector<std::string>& tok, size_t i) { return i < tok.size(); }

template <typename F1, typename F2, typename F3>
double ease_at(const std::vector<std::string>& tok, F1 f1, F2 f2, F3 f3) {
  const size_t n = tok.size() > 0 ? tok.size() - 1 : 0;
  if (n <= 1) return f1(arg(tok, 1, 0));
  if (n == 2) return f2(arg(tok, 1, 0), arg(tok, 2, 0));
  return f3(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 1));
}

uint32_t hex_arg(const std::string& t) {
  return static_cast<uint32_t>(std::strtoul(t.c_str(), nullptr, 16));
}

void emit_times(const char* op, const lfw::Times& t) {
  Line().add(op)
      .add_bits(t.value())
      .add_bits(t.min())
      .add_bits(t.max())
      .add_bits(t.lifes())
      .add_bits(t.remains())
      .out();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_utils <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  lfw::Times times;

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    raw = trace::strip_comment(raw);

    const std::vector<std::string> tok = split_ws(raw);
    if (tok.empty()) continue;

    const std::string& op = tok[0];

    if (op == "ease_linearity") {
      Line().add(op).add_bits(ease_at(tok,
          [](double a) { return lfw::ease_linearity(a); },
          [](double a, double b) { return lfw::ease_linearity(a, b); },
          [](double a, double b, double c) { return lfw::ease_linearity(a, b, c); })).out();

    } else if (op == "ease_linearity_backward") {
      Line().add(op).add_bits(ease_at(tok,
          [](double a) { return lfw::ease_linearity_backward(a); },
          [](double a, double b) { return lfw::ease_linearity_backward(a, b); },
          [](double a, double b, double c) { return lfw::ease_linearity_backward(a, b, c); })).out();

    } else if (op == "ease_in_out_sine") {
      Line().add(op).add_qbits(ease_at(tok,
          [](double a) { return lfw::ease_in_out_sine(a); },
          [](double a, double b) { return lfw::ease_in_out_sine(a, b); },
          [](double a, double b, double c) { return lfw::ease_in_out_sine(a, b, c); })).out();

    } else if (op == "ease_in_out_sine_backward") {
      Line().add(op).add_qbits(ease_at(tok,
          [](double a) { return lfw::ease_in_out_sine_backward(a); },
          [](double a, double b) { return lfw::ease_in_out_sine_backward(a, b); },
          [](double a, double b, double c) { return lfw::ease_in_out_sine_backward(a, b, c); })).out();

    } else if (op == "ease_in_out_quint") {
      Line().add(op).add_qbits(ease_at(tok,
          [](double a) { return lfw::ease_in_out_quint(a); },
          [](double a, double b) { return lfw::ease_in_out_quint(a, b); },
          [](double a, double b, double c) { return lfw::ease_in_out_quint(a, b, c); })).out();

    } else if (op == "ease_in_out_quint_backward") {
      Line().add(op).add_qbits(ease_at(tok,
          [](double a) { return lfw::ease_in_out_quint_backward(a); },
          [](double a, double b) { return lfw::ease_in_out_quint_backward(a, b); },
          [](double a, double b, double c) { return lfw::ease_in_out_quint_backward(a, b, c); })).out();

    } else if (op == "cross_bounding") {
      lfw::Bounding a{arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0), arg(tok, 5, 0), arg(tok, 6, 0)};
      lfw::Bounding b{arg(tok, 7, 0), arg(tok, 8, 0), arg(tok, 9, 0), arg(tok, 10, 0), arg(tok, 11, 0), arg(tok, 12, 0)};
      const lfw::Bounding r = lfw::cross_bounding(a, b);
      Line().add(op).add_bits(r.left).add_bits(r.right).add_bits(r.top)
          .add_bits(r.bottom).add_bits(r.near).add_bits(r.far).out();

    } else if (op == "utf8_encode") {
      std::u16string s;
      for (size_t i = 1; i < tok.size(); ++i) s.push_back(static_cast<char16_t>(hex_arg(tok[i])));
      const std::vector<uint8_t> bytes = lfw::encode_utf8(s);
      Line L;
      L.add(op).add(bytes.size());
      for (uint8_t b : bytes) L.add(static_cast<unsigned long long>(b));
      L.out();

    } else if (op == "utf8_decode") {
      std::vector<uint8_t> bytes;
      for (size_t i = 1; i < tok.size(); ++i) bytes.push_back(static_cast<uint8_t>(hex_arg(tok[i])));
      const std::u16string s = lfw::decode_utf8(bytes);
      Line L;
      L.add(op).add(s.size());
      for (char16_t c : s) L.add(static_cast<unsigned long long>(c));
      L.out();

    } else if (op == "times_new") {
      if (has(tok, 2)) times = lfw::Times(arg(tok, 1, 0), arg(tok, 2, 0));
      else if (has(tok, 1)) times = lfw::Times(arg(tok, 1, 0));
      else times = lfw::Times();
      emit_times(op.c_str(), times);

    } else if (op == "times_set_range") {
      times.set_range(arg(tok, 1, 0), arg(tok, 2, 0));
      emit_times(op.c_str(), times);

    } else if (op == "times_set_lifes") {
      if (has(tok, 1)) times.set_lifes(arg(tok, 1, 0));
      else times.set_lifes();
      emit_times(op.c_str(), times);

    } else if (op == "times_set_min") {
      times.set_min(arg(tok, 1, 0));
      emit_times(op.c_str(), times);

    } else if (op == "times_set_max") {
      times.set_max(arg(tok, 1, 0));
      emit_times(op.c_str(), times);

    } else if (op == "times_set_value") {
      times.set_value(arg(tok, 1, 0));
      emit_times(op.c_str(), times);

    } else if (op == "times_reset") {
      times.reset();
      emit_times(op.c_str(), times);

    } else if (op == "times_reborn") {
      times.reborn();
      emit_times(op.c_str(), times);

    } else if (op == "times_state") {
      emit_times(op.c_str(), times);

    } else if (op == "times_is_max" || op == "times_is_min") {
      const bool r = op == "times_is_max" ? times.is_max() : times.is_min();
      Line().add(op).add_bool(r).out();

    } else if (op == "times_add") {
      const bool r = has(tok, 1) ? times.add(arg(tok, 1, 0)) : times.add();
      Line().add(op).add_bool(r)
          .add_bits(times.value()).add_bits(times.min()).add_bits(times.max())
          .add_bits(times.lifes()).add_bits(times.remains()).out();

    } else if (op == "chk") {
      size_t i = 2;
      const lfw::Value v = trace::parse_value(tok, i);
      const std::string& name = tok[1];
      bool r = false;
      if (name == "is_num") r = lfw::is_num(v);
      else if (name == "is_positive") r = lfw::is_positive(v);
      else if (name == "not_zero_num") r = lfw::not_zero_num(v);
      else if (name == "is_int") r = lfw::is_int(v);
      else if (name == "is_str") r = lfw::is_str(v);
      else if (name == "is_non_empty_str") r = lfw::is_non_empty_str(v);
      else {
        std::fprintf(stderr, "line %d: unknown chk '%s'\n", lineno, name.c_str());
        return 2;
      }
      Line().add(op).add(name).add_bool(r).out();

    } else if (op == "tonum") {
      size_t i = 1;
      const lfw::Value v = trace::parse_value(tok, i);
      const std::optional<double> r = lfw::to_num(v);
      Line L;
      L.add(op);
      if (r.has_value()) L.add_bits(*r);
      else L.add(std::string_view("u"));
      L.out();

    } else if (op == "tonum_or") {
      size_t i = 1;
      const lfw::Value v = trace::parse_value(tok, i);
      const double or_value = i < tok.size() ? to_double(tok[i]) : std::numeric_limits<double>::quiet_NaN();
      Line().add(op).add_bits(lfw::to_num(v, or_value)).out();

    } else if (op == "times_write_nums") {
      std::vector<double> nums(5, 0.0);
      times.write_nums(nums, 0);
      Line L;
      L.add(op);
      for (double v : nums) L.add_bits(v);
      L.out();

    } else if (op == "times_read_nums") {
      std::vector<double> nums(5, 0.0);
      for (size_t i = 0; i < 5; ++i) nums[i] = arg(tok, i + 1, 0);
      times.read_nums(nums, 0);
      emit_times(op.c_str(), times);

    } else if (op == "times_snapshot") {
      const std::array<double, 5> s = times.to_snapshot();
      Line L;
      L.add(op);
      for (double v : s) L.add_bits(v);
      L.out();

    } else if (op == "times_read_snapshot") {
      std::array<double, 5> s{};
      for (size_t i = 0; i < 5; ++i) s[i] = arg(tok, i + 1, 0);
      times.read_snapshot(s);
      emit_times(op.c_str(), times);

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
