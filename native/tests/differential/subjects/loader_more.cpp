#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/loader/preprocess_ball_frame.h"
#include "lfw/loader/preprocess_bg_data.h"
#include "lfw/loader/resolve_prefab.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

std::string join_chain(const std::vector<std::u16string>& chain) {
  std::string out;
  for (size_t i = 0; i < chain.size(); ++i) {
    if (i > 0) out += " -> ";
    out += to_ascii(chain[i]);
  }
  return out;
}

std::string describe(const lfw::loader::ResolvePrefabResult& r) {
  if (r.ok) return "ok " + render(r.value);
  if (r.cycle) return "cycle " + join_chain(r.chain);
  return "missing " + join_chain(r.chain);
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_loader_more <case-file>\n");
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

    if (op == "bf") {
      lfw::Value ctx = parse_value(t, i);
      lfw::loader::preprocess_ball_frame(ctx);
      emit("bf " + render(ctx));
    } else if (op == "bg") {
      lfw::Value data = parse_value(t, i);
      emit("bg " + render(lfw::loader::preprocess_bg_data(data)));
    } else if (op == "rp") {
      lfw::Value obj = parse_value(t, i);
      lfw::Value prefabs = parse_value(t, i);
      emit("rp " + describe(lfw::loader::resolve_prefab(obj, prefabs)));
    } else if (op == "rpm") {
      const std::u16string tag = lfw::to_string(parse_value(t, i));
      const std::u16string who = lfw::to_string(parse_value(t, i));
      const std::u16string what = lfw::to_string(parse_value(t, i));
      lfw::Value obj = parse_value(t, i);
      lfw::Value prefabs = parse_value(t, i);
      const lfw::loader::ResolvePrefabResult r = lfw::loader::resolve_prefab(obj, prefabs);
      if (r.ok) {
        emit("rpm ");
      } else {
        emit("rpm " + to_ascii(lfw::loader::prefab_error_message(tag, who, what, r)));
      }
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
    if (i != t.size() && op != "bf" && op != "bg") {
      std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
      return 2;
    }
  }

  return 0;
}
