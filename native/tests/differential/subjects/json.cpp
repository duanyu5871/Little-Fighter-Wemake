#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/json.h"

#include "trace_util.h"

using trace::esc;
using trace::Line;
using trace::num_hex;
using trace::parse_js_string_literal;
using trace::parse_value;
using trace::split_ws;
using trace::vtag;

namespace {

void dump(const lfw::Value& v, size_t depth) {
  Line out;
  out.add(std::string_view("j"));
  out.add(static_cast<unsigned long long>(depth));
  out.add(vtag(v));
  if (const double* d = std::get_if<double>(&v)) {
    out.add(num_hex(*d));
  } else if (const bool* b = std::get_if<bool>(&v)) {
    out.add_bool(*b);
  } else if (const std::u16string* s = std::get_if<std::u16string>(&v)) {
    out.add(esc(*s));
  } else if (const lfw::Object* o = lfw::as_object(v)) {
    std::u16string joined;
    const std::vector<std::u16string> ks = o->keys();
    for (size_t k = 0; k < ks.size(); ++k) {
      if (k != 0) joined.push_back(u',');
      joined += ks[k];
    }
    out.add(esc(joined));
  } else {
    out.add(std::string_view("-"));
  }
  out.out();

  if (const lfw::Array* a = lfw::as_array(v)) {
    for (const lfw::Value& item : a->items()) dump(item, depth + 1);
  } else if (const lfw::Object* o = lfw::as_object(v)) {
    for (const std::u16string& k : o->keys()) {
      const lfw::Value* p = o->get(k);
      if (p != nullptr) dump(*p, depth + 1);
    }
  }
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_json <case-file>\n");
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
    raw = trace::strip_comment(raw);

    const std::vector<std::string> tok = split_ws(raw);
    if (tok.empty()) continue;

    const std::string& op = tok[0];

    if (op == "jstr") {
      size_t i = 1;
      const lfw::Value v = parse_value(tok, i);
      const std::optional<std::u16string> s = lfw::json_stringify(v);
      Line out;
      out.add(op);
      out.add(s.has_value() ? esc(*s) : std::string("-"));
      out.out();

    } else if (op == "jparse") {
      const std::u16string text = parse_js_string_literal(tok[1]);
      const std::optional<lfw::Value> v = lfw::json_parse(text);
      Line().add(op).add(std::string_view(v.has_value() ? "ok" : "err")).out();
      if (v.has_value()) dump(*v, 0);

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
