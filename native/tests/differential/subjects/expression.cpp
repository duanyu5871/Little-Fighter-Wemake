#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "lfw/base/expression.h"

#include "trace_util.h"

using trace::esc;
using trace::Line;
using trace::parse_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;
using trace::vtag;

using Ctx = int;

namespace {

std::map<std::u16string, lfw::Value> g_table;
std::vector<lfw::Expression<Ctx>> g_exprs;

lfw::Value getter_impl(const Ctx&, const std::u16string& word, lfw::BinOp) {
  const auto it = g_table.find(word);
  if (it == g_table.end()) return lfw::Value();
  return it->second;
}

lfw::ValGetter<Ctx> get_val_getter(const std::u16string& word) {
  if (g_table.find(word) == g_table.end()) return nullptr;
  return &getter_impl;
}

std::u16string join_tokens(const std::vector<std::string>& tok, size_t from) {
  std::u16string out;
  for (size_t i = from; i < tok.size(); ++i) {
    if (i != from) out.push_back(u' ');
    out += to_u16(tok[i]);
  }
  return out;
}

void walk(const lfw::Expression<Ctx>& e, size_t depth) {
  const std::string before_s =
      e.before.empty() ? std::string("-") : std::string(1, static_cast<char>(e.before[0]));
  Line out;
  out.add(std::string_view("node"));
  out.add(static_cast<unsigned long long>(depth));
  out.add(before_s);
  out.add_bool(e.not_);
  out.add(static_cast<unsigned long long>(e.children.size()));
  out.add(e.has_op ? to_ascii(e.op_text) : std::string("-"));
  out.add(vtag(e.val_1));
  out.add(vtag(e.val_2));
  out.add(esc(e.text));
  out.add(e.has_err ? esc(e.err) : std::string("-"));
  out.out();
  for (const lfw::Expression<Ctx>& c : e.children) walk(c, depth + 1);
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_expression <case-file>\n");
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

    if (op == "g") {
      size_t i = 2;
      g_table[to_u16(tok[1])] = parse_value(tok, i);
      Line().add(op).add(tok[1]).out();

    } else if (op == "x") {
      g_table.erase(to_u16(tok[1]));
      Line().add(op).add(tok[1]).out();

    } else if (op == "b") {
      g_exprs.emplace_back(join_tokens(tok, 1), &get_val_getter);
      Line().add(op).add(static_cast<unsigned long long>(g_exprs.size() - 1)).out();
      walk(g_exprs.back(), 0);

    } else if (op == "r") {
      const size_t idx = static_cast<size_t>(std::strtol(tok[1].c_str(), nullptr, 10));
      if (idx >= g_exprs.size()) {
        std::fprintf(stderr, "line %d: expression %zu out of range\n", lineno, idx);
        return 2;
      }
      Line out;
      out.add(op);
      out.add(static_cast<unsigned long long>(idx));
      out.add_bool(g_exprs[idx].run(0));
      out.out();

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
