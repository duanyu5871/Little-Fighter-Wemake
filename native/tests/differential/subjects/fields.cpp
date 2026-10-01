#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/fields.h"

#include "trace_util.h"

using trace::esc;
using trace::Line;
using trace::parse_js_string_literal;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_long;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_fields <case-file>\n");
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
    size_t i = 1;

    if (op == "fw") {
      const std::u16string type = parse_js_string_literal(tok[1]);
      const size_t n = static_cast<size_t>(to_long(tok[2]));
      i = 3;
      std::vector<lfw::Value> args;
      for (size_t j = 0; j < n; ++j) args.push_back(parse_value(tok, i));
      Line().add(op).add(to_ascii(render_value(lfw::field_desc(type, args)))).out();

    } else if (op == "ff") {
      const lfw::Value source = parse_value(tok, i);
      Line().add(op).add(to_ascii(render_value(lfw::fields_of(source)))).out();

    } else if (op == "fm") {
      const lfw::Value m = parse_value(tok, i);
      Line().add(op).add(to_ascii(render_value(lfw::fields_map_2_fields_obj(m)))).out();

    } else if (op == "fr") {
      lfw::Value obj = parse_value(tok, i);
      const lfw::Value m = parse_value(tok, i);
      lfw::reorder_fields(obj, m);
      Line().add(op).add(to_ascii(render_value(obj))).out();

    } else if (op == "fa") {
      const lfw::Value v = parse_value(tok, i);
      Line().add(op).add(to_ascii(render_value(lfw::to_array(v)))).out();

    } else if (op == "fas") {
      const lfw::Value v = parse_value(tok, i);
      const lfw::Value r = lfw::to_array(v);
      const lfw::Array* a = lfw::as_array(v);
      Line().add(op).add_bool(a != nullptr && a == lfw::as_array(r)).out();

    } else if (op == "fv") {
      const lfw::Value data = parse_value(tok, i);
      const lfw::Value m = parse_value(tok, i);
      std::vector<std::u16string> errors;
      std::vector<std::u16string> warnings;
      const bool ok = lfw::validate_fields(data, m, &errors, &warnings);
      Line().add(op).add_bool(ok).out();
      for (const std::u16string& e : errors) Line().add(std::string_view("e")).add(esc(e)).out();
      for (const std::u16string& w : warnings) Line().add(std::string_view("w")).add(esc(w)).out();

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }

    if (i != tok.size()) {
      std::fprintf(stderr, "line %d: %zu trailing token(s) after op '%s':\n", lineno,
                   tok.size() - i, op.c_str());
      for (const std::string& s : tok) std::fprintf(stderr, "  %s\n", s.c_str());
      return 2;
    }
  }

  return 0;
}
