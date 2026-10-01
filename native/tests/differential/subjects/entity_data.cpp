#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/entity_data.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/utils/container_help/traversal.h"

#include "trace_util.h"

namespace {

using lfw::dat_translator::make_obj;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::map<std::string, lfw::Value> g_objs;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

lfw::Value value_of(const std::vector<std::string>& t, size_t& i) { return parse_value(t, i); }

lfw::Object& obj_of(const std::string& id) {
  lfw::Value& v = g_objs[id];
  if (lfw::as_object(v) == nullptr) v = lfw::Value(std::make_shared<lfw::Object>());
  return *lfw::as_object(v);
}

lfw::Value field_of(const std::string& id, const std::u16string& key) {
  const lfw::Object* o = lfw::as_object(g_objs[id]);
  if (o == nullptr) return lfw::Value();
  const lfw::Value* p = o->get(key);
  return p != nullptr ? *p : lfw::Value();
}

lfw::Value collect_traversal(lfw::Value& target) {
  lfw::Array acc;
  lfw::traversal(target, [&acc](const std::u16string& k, lfw::Value& item) {
    acc.push_back(make_obj({{u"k", lfw::Value(k)}, {u"v", item}}));
  });
  return lfw::Value(std::make_shared<lfw::Array>(acc));
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_entity_data <case-file>\n");
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
      obj_of(id).set(key, value_of(t, i));
    } else if (op == "setarr") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      lfw::Array arr;
      while (i < t.size()) arr.push_back(value_of(t, i));
      obj_of(id).set(key, lfw::Value(std::make_shared<lfw::Array>(arr)));
    } else if (op == "dump") {
      const std::string id = t[i++];
      emit("D " + id + " " + render(g_objs[id]));
    } else if (op == "trav") {
      const std::string id = t[i++];
      lfw::Value& v = g_objs[id];
      emit("TRAV " + id + " " + render(collect_traversal(v)));
    } else if (op == "travk") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      lfw::Value target = field_of(id, key);
      emit("TRAVK " + id + " " + to_ascii(key) + " " + render(collect_traversal(target)));
    } else if (op == "mfs") {
      const std::string id = t[i++];
      lfw::Value& v = g_objs[id];
      lfw::dat_translator::make_frames_special(v);
      emit("MFS " + id + " " + render(v));
    } else if (op == "med") {
      const std::string id = t[i++];
      lfw::Value& v = g_objs[id];
      emit("MED " + id + " " + render(lfw::dat_translator::make_entity_data(v)));
    } else if (op == "ppo") {
      const std::string id = t[i++];
      lfw::Value& v = g_objs[id];
      lfw::dat_translator::post_process_obj_data(v);
      emit("PPO " + id + " " + render(v));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
