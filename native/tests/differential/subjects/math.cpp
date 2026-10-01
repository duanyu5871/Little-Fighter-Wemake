#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/utils/math/base.h"
#include "lfw/utils/math/calc_plane.h"
#include "lfw/utils/math/clamp.h"
#include "lfw/utils/math/clamp_add.h"
#include "lfw/utils/math/float_equal.h"
#include "lfw/utils/math/line_plane_intersection.h"
#include "lfw/utils/math/normalize.h"
#include "lfw/utils/math/normalize_plane.h"
#include "lfw/utils/math/probability.h"
#include "lfw/utils/math/project_to_line.h"
#include "lfw/utils/math/range.h"

#include "trace_util.h"

using trace::Line;
using trace::split_ws;
using trace::to_double;
using trace::to_flag;

namespace {

double arg(const std::vector<std::string>& tok, size_t i, double fallback) {
  return i < tok.size() ? to_double(tok[i]) : fallback;
}

void emit_plane(const char* op, const lfw::Plane* p) {
  Line L;
  L.add(op);
  if (!p) {
    L.add("null");
  } else {
    L.add_bits(p->a).add_bits(p->b).add_bits(p->c).add_bits(p->d);
  }
  L.out();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_math <case-file>\n");
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

    if (op == "clamp") {
      Line().add("clamp").add_bits(lfw::clamp(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0))).out();

    } else if (op == "clamp_add") {
      Line().add("clamp_add")
          .add_bits(lfw::clamp_add(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0)))
          .out();

    } else if (op == "normalize") {
      Line().add("normalize").add_bits(lfw::normalize(arg(tok, 1, 0), arg(tok, 2, 1000.0))).out();

    } else if (op == "float_equal" || op == "equal" || op == "eqgt" || op == "eqlt") {
      const double x = arg(tok, 1, 0);
      const double y = arg(tok, 2, 0);
      bool r = false;
      if (op == "float_equal") r = lfw::float_equal(x, y);
      else if (op == "equal") r = lfw::equal(x, y);
      else if (op == "eqgt") r = lfw::eqgt(x, y);
      else r = lfw::eqlt(x, y);
      Line().add(op).add_bool(r).out();

    } else if (op == "range") {
      const auto r = lfw::range(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 1.0));
      Line L;
      L.add("range");
      if (!r) {
        L.add("null");
      } else {
        L.add(r->size());
        for (double v : *r) L.add_bits(v);
      }
      L.out();

    } else if (op == "probability") {
      Line().add("probability").add_qbits(lfw::probability(arg(tok, 1, 0), arg(tok, 2, 0))).out();

    } else if (op == "normalize_plane") {
      emit_plane("normalize_plane", &lfw::normalize_plane(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0)));

    } else if (op == "calc_plane") {
      emit_plane("calc_plane", lfw::calc_plane(
        arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0),
        arg(tok, 4, 0), arg(tok, 5, 0), arg(tok, 6, 0),
        arg(tok, 7, 0), arg(tok, 8, 0), arg(tok, 9, 0)));

    } else if (op == "line_plane") {
      const lfw::Vec3* r = lfw::line_plane_intersection(
        arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0),
        arg(tok, 5, 0), arg(tok, 6, 0), arg(tok, 7, 0),
        arg(tok, 8, 0), arg(tok, 9, 0), arg(tok, 10, 0),
        tok.size() > 11 ? to_flag(tok[11]) : false,
        tok.size() > 12 ? to_flag(tok[12]) : false);
      Line L;
      L.add("line_plane");
      if (!r) L.add("null");
      else L.add_bits(r->x).add_bits(r->y).add_bits(r->z);
      L.out();

    } else if (op == "project_to_line") {
      const auto r = lfw::project_to_line(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0));
      Line L;
      L.add("project_to_line");
      if (!r) L.add("null");
      else L.add_bits((*r)[0]).add_bits((*r)[1]);
      L.out();

    } else if (op == "alias_normalize_plane") {
      const lfw::Plane& r1 = lfw::normalize_plane(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0));
      const lfw::Plane& r2 = lfw::normalize_plane(arg(tok, 5, 0), arg(tok, 6, 0), arg(tok, 7, 0), arg(tok, 8, 0));
      Line().add("alias_normalize_plane")
          .add_bits(r1.a).add_bits(r1.b).add_bits(r1.c).add_bits(r1.d)
          .add_bits(r2.a).add_bits(r2.b).add_bits(r2.c).add_bits(r2.d)
          .out();

    } else if (op == "alias_calc_plane") {
      const lfw::Plane* r1 = lfw::calc_plane(
        arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0),
        arg(tok, 4, 0), arg(tok, 5, 0), arg(tok, 6, 0),
        arg(tok, 7, 0), arg(tok, 8, 0), arg(tok, 9, 0));
      const lfw::Plane* r2 = lfw::calc_plane(
        arg(tok, 10, 0), arg(tok, 11, 0), arg(tok, 12, 0),
        arg(tok, 13, 0), arg(tok, 14, 0), arg(tok, 15, 0),
        arg(tok, 16, 0), arg(tok, 17, 0), arg(tok, 18, 0));
      Line L;
      L.add("alias_calc_plane");
      if (!r1 || !r2) {
        L.add("null");
      } else {
        L.add_bits(r1->a).add_bits(r1->b).add_bits(r1->c).add_bits(r1->d)
         .add_bits(r2->a).add_bits(r2->b).add_bits(r2->c).add_bits(r2->d);
      }
      L.out();

    } else if (op == "alias_line_plane") {
      const lfw::Vec3* r1 = lfw::line_plane_intersection(
        arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0),
        arg(tok, 5, 0), arg(tok, 6, 0), arg(tok, 7, 0),
        arg(tok, 8, 0), arg(tok, 9, 0), arg(tok, 10, 0));
      const lfw::Vec3* r2 = lfw::line_plane_intersection(
        arg(tok, 11, 0), arg(tok, 12, 0), arg(tok, 13, 0), arg(tok, 14, 0),
        arg(tok, 15, 0), arg(tok, 16, 0), arg(tok, 17, 0),
        arg(tok, 18, 0), arg(tok, 19, 0), arg(tok, 20, 0));
      Line L;
      L.add("alias_line_plane");
      if (!r1 || !r2) {
        L.add("null");
      } else {
        L.add_bits(r1->x).add_bits(r1->y).add_bits(r1->z)
         .add_bits(r2->x).add_bits(r2->y).add_bits(r2->z);
      }
      L.out();

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
