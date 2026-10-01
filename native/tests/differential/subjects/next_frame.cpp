#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/next_frame.h"

#include "trace_util.h"

namespace {

using trace::Line;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_double;
using trace::to_u16;

std::map<std::string, lfw::Value> g_objs;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& s) { std::printf("%s\n", s.c_str()); }

lfw::Value value_of(const std::vector<std::string>& t, size_t& i) {
  const std::string& tok = t[i];
  if (tok == "nan") {
    ++i;
    return lfw::Value(std::numeric_limits<double>::quiet_NaN());
  }
  if (tok == "inf") {
    ++i;
    return lfw::Value(std::numeric_limits<double>::infinity());
  }
  if (tok == "ninf") {
    ++i;
    return lfw::Value(-std::numeric_limits<double>::infinity());
  }
  return parse_value(t, i);
}

lfw::Object& obj_of(const std::string& id) {
  lfw::Value& v = g_objs[id];
  if (lfw::as_object(v) == nullptr) v = lfw::Value(std::make_shared<lfw::Object>());
  return *lfw::as_object(v);
}

lfw::Object make_costs(const std::string& key, const std::string& mp, const std::string& hp) {
  lfw::Object inner;
  inner.set(u"mp", lfw::Value(to_double(mp)));
  inner.set(u"hp", lfw::Value(to_double(hp)));
  lfw::Object costs;
  costs.set(to_u16(key), lfw::Value(std::make_shared<lfw::Object>(inner)));
  return costs;
}

std::u16string type_of_token(const std::string& tok) { return tok == "-" ? std::u16string() : to_u16(tok); }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_next_frame <case-file>\n");
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

    if (op == "nf") {
      const lfw::Value id = value_of(t, i);
      const std::u16string zero_as = to_u16(t[i++]);
      emit("NF " + render(lfw::dat_translator::get_next_frame_by_raw_id(id, zero_as, u"", nullptr)));
    } else if (op == "nfc") {
      const lfw::Value id = value_of(t, i);
      const std::u16string zero_as = to_u16(t[i++]);
      const std::u16string type = type_of_token(t[i++]);
      const std::string key = t[i++];
      const std::string mp = t[i++];
      const std::string hp = t[i++];
      const lfw::Object costs = make_costs(key, mp, hp);
      emit("NFC " + render(lfw::dat_translator::get_next_frame_by_raw_id(id, zero_as, type, &costs)));
    } else if (op == "new") {
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
    } else if (op == "setarrobj") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      lfw::Array arr;
      arr.push_back(g_objs[t[i++]]);
      obj_of(id).set(key, lfw::Value(std::make_shared<lfw::Array>(arr)));
    } else if (op == "newarrobj") {
      const std::string id = t[i++];
      lfw::Array arr;
      while (i < t.size()) arr.push_back(g_objs[t[i++]]);
      g_objs[id] = lfw::Value(std::make_shared<lfw::Array>(arr));
    } else if (op == "dump") {
      const std::string id = t[i++];
      emit("D " + id + " " + render(g_objs[id]));
    } else if (op == "cook") {
      const std::string id = t[i++];
      const std::u16string type = type_of_token(t[i++]);
      const std::string key = t[i++];
      const std::string mp = t[i++];
      const std::string hp = t[i++];
      const lfw::Object costs = make_costs(key, mp, hp);
      lfw::Value v = g_objs[id];
      lfw::dat_translator::cook_next_frame_cost(v, type, &costs);
      emit("CK " + id + " " + render(g_objs[id]));
    } else if (op == "addnext") {
      const std::string srcid = t[i++];
      std::vector<lfw::Value> items;
      while (i < t.size()) items.push_back(value_of(t, i));
      const lfw::Value src = srcid == "-" ? lfw::Value() : g_objs[srcid];
      emit("AN " + render(lfw::dat_translator::add_next_frame(src, items)));
    } else if (op == "editnext") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      const lfw::Value val = value_of(t, i);
      lfw::Value v = g_objs[id];
      lfw::dat_translator::edit_next_frame(v, [key, val](lfw::Value& item, size_t) {
        if (lfw::Object* o = lfw::as_object(item)) o->set(key, val);
      });
      emit("EN " + id + " " + render(g_objs[id]));
    } else if (op == "nfmut") {
      const lfw::Value id = value_of(t, i);
      const std::u16string key = to_u16(t[i++]);
      const lfw::Value val = value_of(t, i);
      lfw::Value v = lfw::dat_translator::get_next_frame_by_raw_id(id, u"repeat", u"", nullptr);
      if (lfw::Object* o = lfw::as_object(v)) o->set(key, val);
      emit("NFM " + render(v));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
