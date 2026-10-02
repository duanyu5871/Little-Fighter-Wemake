#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/decode_lf2_dat.h"
#include "lfw/dat_translator/float_scaling_entity.h"
#include "lfw/dat_translator/helpers.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

std::vector<uint8_t> make_buffer(size_t size, long seed) {
  std::vector<uint8_t> buf(size);
  for (size_t i = 0; i < size; ++i) {
    buf[i] = static_cast<uint8_t>((static_cast<long>(i) * 7 + seed) % 256);
  }
  return buf;
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_translator_tail <case-file>\n");
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

    if (op == "ei") {
      size_t i = 1;
      lfw::Value target = parse_value(t, i);
      std::vector<lfw::Value> edits;
      while (i < t.size()) edits.push_back(parse_value(t, i));
      lfw::Object* o = lfw::as_object(target);
      if (o == nullptr) {
        std::fprintf(stderr, "ei target is not an object at line %d\n", lineno);
        return 2;
      }
      lfw::dat_translator::edit_info(*o, edits);
      emit("ei " + render(target));
      continue;
    }

    if (op == "fse") {
      size_t i = 1;
      lfw::Value data = parse_value(t, i);
      if (i != t.size()) {
        std::fprintf(stderr, "trailing token(s) at line %d\n", lineno);
        return 2;
      }
      emit("fse " + render(lfw::dat_translator::float_scaling_entity(data)));
      continue;
    }

    if (op == "dlf2") {
      if (t.size() != 3) {
        std::fprintf(stderr, "dlf2 takes <size> <seed> at line %d\n", lineno);
        return 2;
      }
      const size_t size = static_cast<size_t>(trace::to_long(t[1]));
      const long seed = trace::to_long(t[2]);
      const std::vector<uint8_t> buf = make_buffer(size, seed);
      const std::u16string text =
          lfw::dat_translator::decode_lf2_dat(buf.data(), buf.size());
      emit("dlf2 " + trace::esc(text));
      continue;
    }

    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
