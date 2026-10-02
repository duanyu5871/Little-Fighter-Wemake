#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "lfw/ground.h"

#include "trace_util.h"

using trace::Line;
using trace::split_ws;
using trace::to_ascii;
using trace::to_double;

namespace {

lfw::Ground g;
std::vector<lfw::ITerrainInfo> terrain;

bool check(const std::vector<std::string>& tok, int lineno, size_t lo, size_t hi) {
  if (tok.size() >= lo && tok.size() <= hi) return true;
  std::fprintf(stderr, "line %d: op '%s' expects %zu..%zu tokens, got %zu\n", lineno,
               tok[0].c_str(), lo, hi, tok.size());
  return false;
}

double at(const std::vector<std::string>& tok, size_t i) { return to_double(tok[i]); }

std::optional<double> opt(const std::vector<std::string>& tok, size_t i) {
  return i < tok.size() ? std::optional<double>(to_double(tok[i])) : std::nullopt;
}

void print_seg(Line& L, const lfw::ITerrainInfo& s) {
  L.add(s.id.empty() ? std::string_view("-") : std::string_view(to_ascii(s.id)));
  L.add(s.type);
  L.add_num(s.x1).add_num(s.x2).add_num(s.z1).add_num(s.z2);
  L.add_num(s.h1).add_num(s.h2);
}

lfw::ITerrainInfo make_seg(const std::vector<std::string>& tok) {
  lfw::ITerrainInfo s;
  if (tok.size() > 8) s.id = trace::to_u16(tok[8]);
  s.type = static_cast<int>(to_double(tok[1]));
  s.x1 = at(tok, 2);
  s.x2 = at(tok, 3);
  s.z1 = at(tok, 4);
  s.z2 = at(tok, 5);
  s.h1 = at(tok, 6);
  s.h2 = at(tok, 7);
  return s;
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_ground <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  g.set_terrain(&terrain);

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    raw = trace::strip_comment(raw);

    const std::vector<std::string> tok = split_ws(raw);
    if (tok.empty()) continue;

    const std::string& op = tok[0];

    if (op == "clear") {
      if (!check(tok, lineno, 1, 1)) return 2;
      terrain.clear();
      Line().add(op).add(terrain.size()).out();

    } else if (op == "seg") {
      if (!check(tok, lineno, 8, 9)) return 2;
      terrain.push_back(make_seg(tok));
      Line L;
      L.add(op).add(terrain.size() - 1);
      print_seg(L, terrain.back());
      L.out();

    } else if (op == "base") {
      if (!check(tok, lineno, 1, 1)) return 2;
      Line L;
      L.add(op);
      print_seg(L, g.base());
      L.out();

    } else if (op == "abyss") {
      if (!check(tok, lineno, 1, 1)) return 2;
      Line L;
      L.add(op);
      print_seg(L, lfw::Ground::abyss());
      L.out();

    } else if (op == "y") {
      if (!check(tok, lineno, 4, 4)) return 2;
      const size_t idx = static_cast<size_t>(to_double(tok[1]));
      if (idx >= terrain.size()) {
        std::fprintf(stderr, "line %d: segment index %zu out of range\n", lineno, idx);
        return 2;
      }
      Line().add(op).add_num(lfw::Ground::y(terrain[idx], at(tok, 2), at(tok, 3))).out();

    } else if (op == "segment") {
      if (!check(tok, lineno, 3, 3)) return 2;
      Line L;
      L.add(op);
      print_seg(L, g.segment(at(tok, 1), at(tok, 2)));
      L.out();

    } else if (op == "enterable") {
      if (!check(tok, lineno, 5, 5)) return 2;
      const size_t idx = static_cast<size_t>(to_double(tok[1]));
      if (idx >= terrain.size()) {
        std::fprintf(stderr, "line %d: segment index %zu out of range\n", lineno, idx);
        return 2;
      }
      const std::optional<double> r =
          g.enterable(terrain[idx], at(tok, 2), at(tok, 3), at(tok, 4));
      Line L;
      L.add(op);
      if (r.has_value()) L.add_num(*r);
      else L.add("-");
      L.out();

    } else if (op == "block") {
      if (!check(tok, lineno, 5, 8)) return 2;
      const size_t idx = static_cast<size_t>(to_double(tok[1]));
      if (idx >= terrain.size()) {
        std::fprintf(stderr, "line %d: segment index %zu out of range\n", lineno, idx);
        return 2;
      }
      const std::vector<lfw::BlockPoint>& r =
          g.block(terrain[idx], at(tok, 2), at(tok, 3), at(tok, 4), opt(tok, 5), opt(tok, 6),
                  opt(tok, 7));
      Line L;
      L.add(op).add(r.size());
      for (const lfw::BlockPoint& p : r) L.add_num(p.x).add_num(p.z);
      L.out();

    } else if (op == "intersect") {
      if (!check(tok, lineno, 7, 7)) return 2;
      const lfw::GroundHit h = g.intersect(at(tok, 1), at(tok, 2), at(tok, 3), at(tok, 4),
                                           at(tok, 5), at(tok, 6));
      Line L;
      L.add(op);
      L.add_num(h.point.x).add_num(h.point.y).add_num(h.point.z);
      print_seg(L, h.segment);
      L.out();

    } else if (op == "wall") {
      if (!check(tok, lineno, 7, 7)) return 2;
      const std::optional<lfw::BlockPoint> r = g.intersect_wall(
          at(tok, 1), at(tok, 2), at(tok, 3), at(tok, 4), at(tok, 5), at(tok, 6));
      Line L;
      L.add(op);
      if (r.has_value()) L.add_num(r->x).add_num(r->z);
      else L.add("-");
      L.out();

    } else if (op == "step") {
      if (!check(tok, lineno, 1, 1)) return 2;
      Line().add(op).add_num(g.step()).out();

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
