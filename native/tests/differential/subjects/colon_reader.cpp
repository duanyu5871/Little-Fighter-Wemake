#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/colon_value_reader.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& s) { std::printf("%s\n", s.c_str()); }

lfw::Value value_of(const std::vector<std::string>& t, size_t& i) { return parse_value(t, i); }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_colon_reader <case-file>\n");
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

    if (op == "read") {
      const std::string spec = t[i++];
      const lfw::Value tv = value_of(t, i);
      const std::u16string* tp = std::get_if<std::u16string>(&tv);
      if (tp == nullptr) {
        std::fprintf(stderr, "line %d: read expects a string literal\n", lineno);
        return 2;
      }
      lfw::dat_translator::ColonValueReader reader;
      if (spec != "-") {
        size_t b = 0;
        while (b <= spec.size()) {
          const size_t e = spec.find(',', b);
          const std::string part = e == std::string::npos ? spec.substr(b) : spec.substr(b, e - b);
          if (part.size() >= 2) {
            const std::u16string nm = to_u16(part.substr(2));
            if (part[0] == 's') reader.str(nm);
            else if (part[0] == 'i') reader.int_(nm);
            else reader.int_2(nm);
          }
          if (e == std::string::npos) break;
          b = e + 1;
        }
      }
      lfw::Object out;
      const std::u16string rem = reader.read(*tp, out);
      emit("R " + render(lfw::Value(std::make_shared<lfw::Object>(out))) + " " +
           render(lfw::Value(rem)));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
