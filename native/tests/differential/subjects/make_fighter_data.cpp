#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/make_fighter_data.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

lfw::Value g_ctx;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

lfw::Value& slot_value(lfw::Value& owner, const std::u16string& key) {
  lfw::Object* o = lfw::as_object(owner);
  if (o == nullptr) {
    owner = lfw::Value(std::make_shared<lfw::Object>());
    o = lfw::as_object(owner);
  }
  const lfw::Value* cur = o->get(key);
  if (cur == nullptr || lfw::as_object(*cur) == nullptr) {
    o->set(key, lfw::Value(std::make_shared<lfw::Object>()));
  }
  return *const_cast<lfw::Value*>(o->get(key));
}

lfw::Object& slot(lfw::Value& owner, const std::u16string& key) {
  return *lfw::as_object(slot_value(owner, key));
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_make_fighter_data <case-file>\n");
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

    if (op == "reset") {
      g_ctx = lfw::Value(std::make_shared<lfw::Object>());
    } else if (op == "c") {
      const std::u16string key = to_u16(t[i++]);
      lfw::Value v = parse_value(t, i);
      lfw::as_object(g_ctx)->set(key, v);
      if (i != t.size()) {
        std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
        return 2;
      }
    } else if (op == "f") {
      const std::u16string frame_key = to_u16(t[i++]);
      lfw::Value v = parse_value(t, i);
      slot(g_ctx, u"frames").set(frame_key, v);
      if (i != t.size()) {
        std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
        return 2;
      }
    } else if (op == "s") {
      const std::u16string frame_key = to_u16(t[i++]);
      const std::u16string sub_key = to_u16(t[i++]);
      lfw::Value v = parse_value(t, i);
      slot(slot_value(g_ctx, u"frames"), frame_key).set(sub_key, v);
      if (i != t.size()) {
        std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
        return 2;
      }
    } else if (op == "run") {
      lfw::Value ret = lfw::dat_translator::make_fighter_data(g_ctx);
      emit("R " + render(ret));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
