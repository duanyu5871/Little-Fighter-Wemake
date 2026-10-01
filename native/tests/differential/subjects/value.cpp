#include <cstddef>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"

#include "trace_util.h"

using trace::esc;
using trace::Line;
using trace::parse_js_string_literal;
using trace::split_ws;
using trace::to_double;
using trace::to_long;

namespace {

std::vector<lfw::Value> g_handles;

lfw::Value parse_value(const std::vector<std::string>& t, size_t& i) {
  if (i >= t.size()) {
    std::fprintf(stderr, "unexpected end of value literal\n");
    std::exit(2);
  }
  const std::string kind = t[i++];
  if (kind == "u") return lfw::Value();
  if (kind == "z") return lfw::Value(lfw::NullTag{});
  if (kind == "b") return lfw::Value(t[i++] == "1");
  if (kind == "n") return lfw::Value(to_double(t[i++]));
  if (kind == "s") return lfw::Value(parse_js_string_literal(t[i++]));
  if (kind == "a") {
    const size_t n = static_cast<size_t>(to_long(t[i++]));
    auto arr = std::make_shared<lfw::Array>();
    for (size_t j = 0; j < n; ++j) arr->push_back(parse_value(t, i));
    return lfw::Value(arr);
  }
  std::fprintf(stderr, "bad value literal '%s'\n", kind.c_str());
  std::exit(2);
}

size_t add_handle(lfw::Value v) {
  g_handles.push_back(std::move(v));
  return g_handles.size() - 1;
}

void print_handle_op(std::string_view op, lfw::Value v) {
  Line().add(op).add(add_handle(std::move(v))).out();
}
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_value <case-file>\n");
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
    if (const auto hash = raw.find('#'); hash != std::string::npos) raw.erase(hash);

    const std::vector<std::string> tok = split_ws(raw);
    if (tok.empty()) continue;

    const std::string& op = tok[0];

    if (op == "typeof" || op == "truthy" || op == "is_array" || op == "length") {
      size_t i = 1;
      const lfw::Value v = parse_value(tok, i);
      Line out;
      out.add(op);
      if (op == "typeof") {
        out.add(std::string(lfw::type_of(v)));
      } else if (op == "truthy") {
        out.add_bool(lfw::truthy(v));
      } else if (op == "is_array") {
        out.add_bool(lfw::is_array(v));
      } else {
        const lfw::Array* a = lfw::as_array(v);
        if (a) out.add(static_cast<unsigned long long>(a->size()));
        else out.add(std::string_view("-"));
      }
      out.out();

    } else if (op == "hold") {
      size_t i = 1;
      print_handle_op(op, parse_value(tok, i));

    } else if (op == "dup") {
      const size_t idx = static_cast<size_t>(to_long(tok[1]));
      if (idx >= g_handles.size()) {
        std::fprintf(stderr, "line %d: handle %zu out of range\n", lineno, idx);
        return 2;
      }
      print_handle_op(op, g_handles[idx]);

    } else if (op == "same") {
      const size_t ia = static_cast<size_t>(to_long(tok[1]));
      const size_t ib = static_cast<size_t>(to_long(tok[2]));
      if (ia >= g_handles.size() || ib >= g_handles.size()) {
        std::fprintf(stderr, "line %d: handle out of range\n", lineno);
        return 2;
      }
      const lfw::Array* pa = lfw::as_array(g_handles[ia]);
      const lfw::Array* pb = lfw::as_array(g_handles[ib]);
      Line out;
      out.add(op);
      if (pa && pb) out.add_bool(pa == pb);
      else out.add(std::string_view("-"));
      out.out();

    } else if (op == "elem") {
      const size_t ih = static_cast<size_t>(to_long(tok[1]));
      const size_t idx = static_cast<size_t>(to_long(tok[2]));
      if (ih >= g_handles.size()) {
        std::fprintf(stderr, "line %d: handle %zu out of range\n", lineno, ih);
        return 2;
      }
      const lfw::Value held = g_handles[ih];
      const lfw::Array* a = lfw::as_array(held);
      Line out;
      out.add(op);
      if (!a || idx >= a->size()) {
        out.add(std::string_view("-"));
      } else {
        out.add(static_cast<unsigned long long>(add_handle(a->at(idx))));
      }
      out.out();

    } else if (op == "eq" || op == "seq") {
      size_t i = 1;
      const lfw::Value x = parse_value(tok, i);
      const lfw::Value y = parse_value(tok, i);
      Line out;
      out.add(op);
      out.add_bool(op == "eq" ? lfw::equals(x, y) : lfw::strict_equals(x, y));
      out.out();

    } else if (op == "ton") {
      size_t i = 1;
      const lfw::Value x = parse_value(tok, i);
      Line out;
      out.add(op);
      out.add(trace::num_hex(lfw::to_number(x)));
      out.out();

    } else if (op == "tos") {
      size_t i = 1;
      const lfw::Value x = parse_value(tok, i);
      Line out;
      out.add(op);
      out.add(esc(lfw::to_string(x)));
      out.out();

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
