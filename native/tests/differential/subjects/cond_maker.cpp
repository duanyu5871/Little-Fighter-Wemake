#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/cond_maker.h"

#include "trace_util.h"

namespace {

using trace::esc;
using trace::parse_value;
using trace::split_ws;
using trace::to_double;
using trace::to_u16;

std::map<std::string, std::unique_ptr<lfw::CondMaker>> g_makers;

void emit_line(const std::string& s) { std::printf("%s\n", s.c_str()); }

lfw::CondMaker* maker_of(const std::string& id, bool reset) {
  std::map<std::string, std::unique_ptr<lfw::CondMaker>>::iterator it = g_makers.find(id);
  if (it == g_makers.end() || reset) {
    g_makers[id] = std::make_unique<lfw::CondMaker>();
    return g_makers[id].get();
  }
  return it->second.get();
}

std::u16string op_of(const std::string& tag) {
  if (tag == "empty") return u"";
  if (tag == "or") return u"||";
  if (tag == "and") return u"&&";
  if (tag == "not") return u"!";
  return to_u16(tag);
}

lfw::Value obj_operand() {
  lfw::Object o;
  o.set(u"a", lfw::Value(1.0));
  return lfw::Value(std::make_shared<lfw::Object>(o));
}

lfw::Value arr_operand() {
  lfw::Array a;
  a.push_back(lfw::Value(1.0));
  a.push_back(lfw::Value(2.0));
  return lfw::Value(std::make_shared<lfw::Array>(a));
}

bool bool_of(const std::string& s) { return s == "b1" || s == "1" || s == "true"; }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_cond_maker <case-file>\n");
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

    if (op == "mk") {
      maker_of(t[i++], true);
    } else if (op == "done") {
      const std::string id = t[i++];
      lfw::CondMaker* m = maker_of(id, false);
      if (!m->ok()) emit_line("E " + id + " " + esc(m->error()));
      else emit_line("D " + id + " " + esc(m->done()));
    } else if (op == "quote") {
      const std::string id = t[i++];
      const bool on = bool_of(t[i++]);
      const std::string tag = t[i++];
      const char16_t q = tag == "sq" ? u'\'' : (tag == "bad" ? u'@' : u'"');
      maker_of(id, false)->quote_strings(on, q);
    } else if (op == "term") {
      const std::string id = t[i++];
      const std::string tag = t[i++];
      if (tag == "wrap") {
        maker_of(id, false)->term_format([](const lfw::Value& a, const std::u16string& o,
                                            const lfw::Value& b) {
          return u"(" + lfw::to_string(a) + o + lfw::to_string(b) + u")";
        });
      }
    } else if (op == "add" || op == "or" || op == "and") {
      const std::string id = t[i++];
      const lfw::Value v1 = parse_value(t, i);
      const std::u16string oper = to_u16(t[i++]);
      const lfw::Value v2 = parse_value(t, i);
      lfw::CondMaker* m = maker_of(id, false);
      if (op == "add") m->add(v1, oper, v2);
      else if (op == "or") m->or_(v1, oper, v2);
      else m->and_(v1, oper, v2);
    } else if (op == "addtag" || op == "addobj" || op == "addarr") {
      const std::string id = t[i++];
      const std::string tag = t[i++];
      const std::u16string oper = op_of(tag);
      lfw::CondMaker* m = maker_of(id, false);
      if (op == "addtag") {
        const lfw::Value v1 = parse_value(t, i);
        const lfw::Value v2 = parse_value(t, i);
        m->add(v1, oper, v2);
      } else if (op == "addobj") {
        const lfw::Value v2 = parse_value(t, i);
        m->add(obj_operand(), oper, v2);
      } else {
        const lfw::Value v2 = parse_value(t, i);
        m->add(arr_operand(), oper, v2);
      }
    } else if (op == "not" || op == "wrap") {
      const std::string id = t[i++];
      const lfw::Value v1 = parse_value(t, i);
      const std::u16string oper = to_u16(t[i++]);
      const lfw::Value v2 = parse_value(t, i);
      lfw::CondMaker* m = maker_of(id, false);
      if (op == "not") {
        m->not_([v1, oper, v2](lfw::CondMaker& c) -> lfw::CondMaker* {
          c.add(v1, oper, v2);
          return nullptr;
        });
      } else {
        m->wrap([v1, oper, v2](lfw::CondMaker& c) -> lfw::CondMaker* {
          c.add(v1, oper, v2);
          return nullptr;
        });
      }
    } else if (op == "nested") {
      const std::string id = t[i++];
      const lfw::Value a = parse_value(t, i);
      const std::u16string oa = to_u16(t[i++]);
      const lfw::Value b = parse_value(t, i);
      const lfw::Value c1 = parse_value(t, i);
      const std::u16string ob = to_u16(t[i++]);
      const lfw::Value d = parse_value(t, i);
      maker_of(id, false)->wrap([a, oa, b, c1, ob, d](lfw::CondMaker& c) -> lfw::CondMaker* {
        c.add(a, oa, b).or_(c1, ob, d);
        return nullptr;
      });
    } else if (op == "oneof" || op == "andoneof" || op == "oroneof" || op == "notin" ||
               op == "andnotin" || op == "ornotin") {
      const std::string id = t[i++];
      const lfw::Value v1 = parse_value(t, i);
      std::vector<lfw::Value> vs;
      while (i < t.size()) vs.push_back(parse_value(t, i));
      if (vs.empty()) {
        std::fprintf(stderr, "line %d: %s needs values\n", lineno, op.c_str());
        return 2;
      }
      lfw::CondMaker* m = maker_of(id, false);
      if (op == "oneof") m->one_of(v1, vs);
      else if (op == "andoneof") m->and_one_of(v1, vs);
      else if (op == "oroneof") m->or_one_of(v1, vs);
      else if (op == "notin") m->not_in(v1, vs);
      else if (op == "andnotin") m->and_not_in(v1, vs);
      else m->or_not_in(v1, vs);
    } else if (op == "oneof0") {
      const std::string id = t[i++];
      const lfw::Value v1 = parse_value(t, i);
      maker_of(id, false)->one_of(v1, {});
    } else if (op == "notin0") {
      const std::string id = t[i++];
      const lfw::Value v1 = parse_value(t, i);
      maker_of(id, false)->not_in(v1, {});
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
