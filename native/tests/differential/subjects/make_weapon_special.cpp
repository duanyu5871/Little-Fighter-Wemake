#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/broken_piece_frames.h"
#include "lfw/dat_translator/make_weapon_special.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::map<std::string, lfw::Value> g_objs;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

lfw::Object& obj_of(const std::string& id) {
  lfw::Value& v = g_objs[id];
  if (lfw::as_object(v) == nullptr) v = lfw::Value(std::make_shared<lfw::Object>());
  return *lfw::as_object(v);
}

lfw::Value piece_of(const std::string& name) {
  namespace bp = lfw::dat_translator;
  if (name == "stone") return bp::stone();
  if (name == "sstone") return bp::sstone();
  if (name == "stick") return bp::stick();
  if (name == "sstick") return bp::sstick();
  if (name == "hoe") return bp::hoe();
  if (name == "s_hoe") return bp::s_hoe();
  if (name == "k_hoe") return bp::k_hoe();
  if (name == "box0") return bp::box0();
  if (name == "box1") return bp::box1();
  if (name == "box2") return bp::box2();
  if (name == "box3") return bp::box3();
  if (name == "baseball") return bp::baseball();
  if (name == "milk1") return bp::milk1();
  if (name == "milk2") return bp::milk2();
  if (name == "milk3") return bp::milk3();
  if (name == "icesword1") return bp::icesword1();
  if (name == "icesword2") return bp::icesword2();
  if (name == "beer1") return bp::beer1();
  if (name == "beer2") return bp::beer2();
  if (name == "boomerang") return bp::boomerang();
  if (name == "armour") return bp::armour();
  return lfw::Value();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_make_weapon_special <case-file>\n");
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

    if (op == "new") {
      g_objs[t[i++]] = lfw::Value(std::make_shared<lfw::Object>());
    } else if (op == "set") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      obj_of(id).set(key, parse_value(t, i));
      if (i != t.size()) {
        std::fprintf(stderr, "line %d: trailing token(s)\n", lineno);
        return 2;
      }
    } else if (op == "dump") {
      const std::string id = t[i++];
      emit("D " + id + " " + render(g_objs[id]));
    } else if (op == "wsp") {
      const std::string id = t[i++];
      lfw::dat_translator::make_weapon_special(g_objs[id]);
      emit("wsp " + id + " " + render(g_objs[id]));
    } else if (op == "piece") {
      const std::string name = t[i++];
      emit("piece " + name + " " + render(piece_of(name)));
    } else if (op == "pmut") {
      const std::string name = t[i++];
      const size_t idx = static_cast<size_t>(trace::to_long(t[i++]));
      const lfw::Value v = parse_value(t, i);
      lfw::Array* a = const_cast<lfw::Array*>(lfw::as_array(piece_of(name)));
      if (a != nullptr && idx < a->size()) a->at(idx) = v;
      emit("pmut " + name + " " + render(piece_of(name)));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
