#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/string_matchers.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/utils/type_check.h"

#include "trace_util.h"

namespace {

using lfw::dat_translator::make_arr;
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

lfw::Value field_of(const std::string& id, const char16_t* key) {
  const lfw::Object* o = lfw::as_object(g_objs[id]);
  if (o == nullptr) return lfw::Value();
  const lfw::Value* p = o->get(std::u16string(key));
  return p != nullptr ? *p : lfw::Value();
}

lfw::Value text_value(const lfw::Value& v) {
  if (!lfw::is_str(v)) return lfw::Value(lfw::NullTag{});
  return lfw::Value(lfw::to_string(v));
}

std::u16string str_arg(const lfw::Value& v) { return lfw::to_string(v); }

}
int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_string_matchers <case-file>\n");
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
    } else if (op == "hash") {
      const lfw::Value tv = value_of(t, i);
      if (!lfw::is_str(tv)) {
        emit("hash u");
      } else {
        const std::optional<std::u16string> h =
            lfw::dat_translator::match_hash_end(lfw::to_string(tv));
        emit("hash " + (h.has_value() ? render(lfw::Value(*h)) : std::string("u")));
      }
    } else if (op == "mcv") {
      const std::string id = t[i++];
      const lfw::Value tv = field_of(id, u"text");
      lfw::Array arr;
      if (lfw::is_str(tv)) {
        const std::vector<std::pair<std::u16string, std::u16string>> pairs =
            lfw::dat_translator::match_colon_value(lfw::to_string(tv));
        for (const std::pair<std::u16string, std::u16string>& p : pairs) {
          arr.push_back(make_obj({{u"k", lfw::Value(p.first)}, {u"v", lfw::Value(p.second)}}));
        }
      }
      emit("MCV " + id + " " + render(lfw::Value(std::make_shared<lfw::Array>(arr))));
    } else if (op == "mbo") {
      const std::string id = t[i++];
      const std::u16string start = str_arg(value_of(t, i));
      const std::u16string end = str_arg(value_of(t, i));
      const lfw::Value tv = field_of(id, u"text");
      lfw::Value out(lfw::NullTag{});
      if (lfw::is_str(tv)) {
        const std::optional<std::u16string> body =
            lfw::dat_translator::match_block_once(lfw::to_string(tv), start, end);
        if (body.has_value()) out = lfw::Value(*body);
      }
      emit("MBO " + id + " " + render(out));
    } else if (op == "tb") {
      const std::string id = t[i++];
      const std::u16string start = str_arg(value_of(t, i));
      const std::u16string end = str_arg(value_of(t, i));
      const lfw::Value tv = field_of(id, u"text");
      lfw::Array blocks;
      lfw::Value remains(lfw::NullTag{});
      if (lfw::is_str(tv)) {
        const lfw::dat_translator::TakeBlocksResult r =
            lfw::dat_translator::take_blocks(lfw::to_string(tv), start, end);
        for (const std::u16string& b : r.blocks) blocks.push_back(lfw::Value(b));
        remains = lfw::Value(r.remains);
      }
      emit("TB " + id + " " +
           render(make_obj({{u"blocks", lfw::Value(std::make_shared<lfw::Array>(blocks))},
                            {u"remains", remains}})));
    } else if (op == "du") {
      const std::string id = t[i++];
      lfw::Value& v = g_objs[id];
      if (lfw::as_object(v) != nullptr) lfw::dat_translator::delete_undefined(v);
      emit("DU " + id + " " + render(v));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
