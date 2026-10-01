#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "lfw/core/js_num.h"
#include "lfw/core/mersenne_twister.h"

namespace {

std::string hex16(uint64_t v) {
  char buf[17];
  std::snprintf(buf, sizeof buf, "%016llx",
                static_cast<unsigned long long>(v));
  return std::string(buf);
}

std::string bits_hex(double d) { return hex16(lfw::f64_bits(d)); }

std::string q_bits(double d) { return bits_hex(lfw::round_float(d)); }

class Line {
 public:
  Line& add(std::string_view v) {
    if (!_s.empty()) _s += ' ';
    _s += v;
    return *this;
  }
  Line& add(long long v) { return add(std::to_string(v)); }
  Line& add(unsigned long long v) { return add(std::to_string(v)); }
  Line& add_u32(uint32_t v) { return add(static_cast<unsigned long long>(v)); }
  Line& add_opt(bool has_value, double v) {
    return add(has_value ? q_bits(v) : std::string_view("-"));
  }
  void out() const { std::printf("%s\n", _s.c_str()); }

 private:
  std::string _s;
};

std::vector<std::string> split_ws(const std::string& s) {
  std::vector<std::string> out;
  size_t i = 0;
  const size_t n = s.size();
  while (i < n) {
    while (i < n && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    size_t j = i;
    while (j < n && !std::isspace(static_cast<unsigned char>(s[j]))) ++j;
    if (j > i) out.push_back(s.substr(i, j - i));
    i = j;
  }
  return out;
}

double to_double(const std::string& t) { return std::strtod(t.c_str(), nullptr); }

long to_long(const std::string& t) { return std::strtol(t.c_str(), nullptr, 10); }

}

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

    if (const auto hash = raw.find('#'); hash != std::string::npos)
      raw.erase(hash);

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
        Line().add("int").add_u32(mt.next_int()).out();
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
