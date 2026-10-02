#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "lfw/transform.h"

#include "trace_util.h"

using trace::Line;
using trace::split_ws;
using trace::to_double;

namespace {

lfw::Transform t;

std::optional<double> num(const std::vector<std::string>& tok, size_t i) {
  return i < tok.size() ? std::optional<double>(to_double(tok[i])) : std::nullopt;
}

bool check(const std::vector<std::string>& tok, int lineno, size_t lo, size_t hi) {
  if (tok.size() >= lo && tok.size() <= hi) return true;
  std::fprintf(stderr, "line %d: op '%s' expects %zu..%zu tokens, got %zu\n", lineno,
               tok[0].c_str(), lo, hi, tok.size());
  return false;
}

void state(const std::string& op) {
  Line L;
  L.add(op);
  L.add_num(t.x()).add_num(t.y()).add_num(t.z()).add_num(t.rotation());
  L.add_num(t.scale_x()).add_num(t.scale_y()).add_num(t.scale_z());
  L.add_num(t.d().x).add_num(t.d().y).add_num(t.d().z).add_num(t.d().rotation);
  L.add_num(t.d().scale_x).add_num(t.d().scale_y).add_num(t.d().scale_z);
  L.add_bool(t.is_smoothing());
  L.out();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_transform <case-file>\n");
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

    if (op == "new") {
      if (!check(tok, lineno, 1, 1)) return 2;
      t = lfw::Transform();
      state(op);

    } else if (op == "pos") {
      if (!check(tok, lineno, 1, 4)) return 2;
      t.set_position(num(tok, 1), num(tok, 2), num(tok, 3));
      state(op);

    } else if (op == "scale") {
      if (!check(tok, lineno, 1, 4)) return 2;
      t.set_scale(num(tok, 1), num(tok, 2), num(tok, 3));
      state(op);

    } else if (op == "rot") {
      if (!check(tok, lineno, 1, 2)) return 2;
      t.set_rotation(num(tok, 1));
      state(op);

    } else if (op == "sx") {
      if (!check(tok, lineno, 2, 2)) return 2;
      t.set_x(to_double(tok[1]));
      state(op);

    } else if (op == "sy") {
      if (!check(tok, lineno, 2, 2)) return 2;
      t.set_y(to_double(tok[1]));
      state(op);

    } else if (op == "sz") {
      if (!check(tok, lineno, 2, 2)) return 2;
      t.set_z(to_double(tok[1]));
      state(op);

    } else if (op == "rx") {
      if (!check(tok, lineno, 2, 2)) return 2;
      t.set_rotation_value(to_double(tok[1]));
      state(op);

    } else if (op == "ssx") {
      if (!check(tok, lineno, 2, 2)) return 2;
      t.set_scale_x(to_double(tok[1]));
      state(op);

    } else if (op == "ssy") {
      if (!check(tok, lineno, 2, 2)) return 2;
      t.set_scale_y(to_double(tok[1]));
      state(op);

    } else if (op == "ssz") {
      if (!check(tok, lineno, 2, 2)) return 2;
      t.set_scale_z(to_double(tok[1]));
      state(op);

    } else if (op == "move") {
      if (!check(tok, lineno, 1, 5)) return 2;
      t.move_to(num(tok, 1), num(tok, 2), num(tok, 3), num(tok, 4));
      state(op);

    } else if (op == "scale_to") {
      if (!check(tok, lineno, 1, 5)) return 2;
      t.scale_to(num(tok, 1), num(tok, 2), num(tok, 3), num(tok, 4));
      state(op);

    } else if (op == "rotate_to") {
      if (!check(tok, lineno, 1, 3)) return 2;
      t.rotate_to(num(tok, 1), num(tok, 2));
      state(op);

    } else if (op == "update") {
      if (!check(tok, lineno, 1, 2)) return 2;
      t.update(num(tok, 1));
      state(op);

    } else if (op == "arrived") {
      if (!check(tok, lineno, 1, 2)) return 2;
      Line().add(op).add_bool(t.is_arrived(num(tok, 1))).out();

    } else if (op == "snap") {
      if (!check(tok, lineno, 1, 1)) return 2;
      const lfw::TransformData s = t.snapshot();
      Line L;
      L.add(op);
      L.add_num(s.x).add_num(s.y).add_num(s.z).add_num(s.rotation);
      L.add_num(s.scale_x).add_num(s.scale_y).add_num(s.scale_z);
      L.add_bool(t.is_smoothing());
      L.out();

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
