#include <cstdint>
#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "lfw/base/graves.h"
#include "lfw/utils/array/loop_arr.h"
#include "lfw/utils/array/make_arr.h"
#include "lfw/utils/array/map_arr.h"
#include "lfw/utils/container_help/ensure.h"
#include "lfw/utils/container_help/filter.h"
#include "lfw/utils/container_help/find.h"
#include "lfw/utils/container_help/fisrt.h"
#include "lfw/utils/container_help/loop_offset.h"
#include "lfw/utils/container_help/map_no_void.h"
#include "lfw/utils/container_help/nested_map.h"
#include "lfw/utils/container_help/nested_multi_map.h"

#include "trace_util.h"

using trace::Line;
using trace::split_ws;
using trace::to_double;

namespace {

double arg(const std::vector<std::string>& tok, size_t i, double fallback) {
  return i < tok.size() ? to_double(tok[i]) : fallback;
}

size_t bar_index(const std::vector<std::string>& tok) {
  for (size_t i = 1; i < tok.size(); ++i) {
    if (tok[i] == "|") return i;
  }
  return tok.size();
}

std::vector<double> items(const std::vector<std::string>& tok, size_t from, size_t to) {
  std::vector<double> out;
  for (size_t i = from; i < to; ++i) out.push_back(to_double(tok[i]));
  return out;
}

void emit_list(const char* op, const std::vector<double>& v) {
  Line L;
  L.add(op).add(v.size());
  for (double x : v) L.add_bits(x);
  L.out();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_collections <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  lfw::Graves<double> graves;
  lfw::NestedMap<double, double, double> nested;
  lfw::NestedMultiMap<double, double, double> multi;

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    raw = trace::strip_comment(raw);

    const std::vector<std::string> tok = split_ws(raw);
    if (tok.empty()) continue;

    const std::string& op = tok[0];

    if (op == "graves_new") {
      graves = lfw::Graves<double>();
      Line().add(op).add(graves.l().size()).out();

    } else if (op == "graves_add") {
      graves.add(arg(tok, 1, 0));
      Line().add(op).add(graves.l().size()).out();

    } else if (op == "graves_take") {
      const std::optional<double> r = graves.take();
      Line L;
      L.add(op);
      if (r.has_value()) L.add_bits(*r);
      else L.add("-");
      L.out();

    } else if (op == "graves_l") {
      Line L;
      L.add(op).add(graves.l().size());
      for (const auto& v : graves.l()) {
        if (v.has_value()) L.add_bits(*v);
        else L.add("-");
      }
      L.out();

    } else if (op == "filter_gt") {
      const double t = arg(tok, 1, 0);
      const std::vector<double> a = items(tok, 2, tok.size());
      emit_list(op.c_str(), lfw::filter(a, [t](double v) { return v > t; }));

    } else if (op == "find_gt") {
      const double t = arg(tok, 1, 0);
      const std::vector<double> a = items(tok, 2, tok.size());
      const std::optional<double> r = lfw::find(a, [t](double v) { return v > t; });
      Line L;
      L.add(op);
      if (r.has_value()) L.add_bits(*r);
      else L.add("-");
      L.out();

    } else if (op == "find_last_gt") {
      const double t = arg(tok, 1, 0);
      const std::vector<double> a = items(tok, 2, tok.size());
      const std::optional<double> r = lfw::find_last(a, [t](double v) { return v > t; });
      Line L;
      L.add(op);
      if (r.has_value()) L.add_bits(*r);
      else L.add("-");
      L.out();

    } else if (op == "fisrt_gt") {
      const double t = arg(tok, 1, 0);
      const std::vector<double> a = items(tok, 2, tok.size());
      const std::optional<double> r = lfw::fisrt(a, [t](double v) -> std::optional<double> {
        return v > t ? std::optional<double>(v) : std::nullopt;
      });
      Line L;
      L.add(op);
      if (r.has_value()) L.add_bits(*r);
      else L.add("-");
      L.out();

    } else if (op == "last_gt") {
      const double t = arg(tok, 1, 0);
      const std::vector<double> a = items(tok, 2, tok.size());
      const std::optional<double> r = lfw::last(a, [t](double v) -> std::optional<double> {
        return v > t ? std::optional<double>(v) : std::nullopt;
      });
      Line L;
      L.add(op);
      if (r.has_value()) L.add_bits(*r);
      else L.add("-");
      L.out();

    } else if (op == "map_no_void_gt") {
      const double t = arg(tok, 1, 0);
      const std::vector<double> a = items(tok, 2, tok.size());
      emit_list(op.c_str(), lfw::map_no_void(a, [t](double v) -> std::optional<double> {
        return v > t ? std::optional<double>(v * 2.0) : std::nullopt;
      }));

    } else if (op == "intersection") {
      const size_t bar = bar_index(tok);
      const std::vector<double> a = items(tok, 1, bar);
      const std::vector<double> b = items(tok, bar + 1, tok.size());
      emit_list(op.c_str(), lfw::intersection(a, b));

    } else if (op == "ensure") {
      const size_t bar = bar_index(tok);
      const std::vector<double> existing = items(tok, 1, bar);
      const std::vector<double> fresh = items(tok, bar + 1, tok.size());
      std::optional<std::vector<double>> out =
          existing.empty() ? std::nullopt : std::optional<std::vector<double>>(existing);
      emit_list(op.c_str(), lfw::ensure(out, fresh));

    } else if (op == "loop_offset") {
      const double current = arg(tok, 1, 0);
      const double offset = arg(tok, 2, 0);
      const std::vector<double> a = items(tok, 3, tok.size());
      const std::optional<double> r = lfw::loop_offset(a, current, offset);
      Line L;
      L.add(op);
      if (r.has_value()) L.add_bits(*r);
      else L.add("-");
      L.out();

    } else if (op == "make_arr") {
      const double size = arg(tok, 1, 0);
      emit_list(op.c_str(), lfw::make_arr(size, [](double i) { return i; }));

    } else if (op == "map_arr_mul") {
      const double k = arg(tok, 1, 0);
      const std::vector<double> a = items(tok, 2, tok.size());
      emit_list(op.c_str(), lfw::map_arr(a, [k](double v, auto, const std::vector<double>&) { return v * k; }));

    } else if (op == "map_arr_scalar") {
      const double k = arg(tok, 1, 0);
      const double v = arg(tok, 2, 0);
      emit_list(op.c_str(), lfw::map_arr(v, [k](double x, auto, const std::vector<double>&) { return x * k; }));

    } else if (op == "loop_arr_idx" || op == "loop_arr_scalar") {
      std::vector<double> idxs;
      auto fn = [&idxs](double, auto idx, const std::vector<double>&) { idxs.push_back(static_cast<double>(idx)); };
      if (op == "loop_arr_idx") lfw::loop_arr(items(tok, 1, tok.size()), fn);
      else lfw::loop_arr(arg(tok, 1, 0), fn);
      Line L;
      L.add(op).add(idxs.size());
      for (double i : idxs) L.add(static_cast<long long>(i));
      L.out();

    } else if (op == "nested_set") {
      nested.set(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0));
      Line().add(op).add_bits(arg(tok, 3, 0)).out();

    } else if (op == "nested_get") {
      const std::optional<double> r = nested.get(arg(tok, 1, 0), arg(tok, 2, 0));
      Line L;
      L.add(op);
      if (r.has_value()) L.add_bits(*r);
      else L.add("-");
      L.out();

    } else if (op == "nested_has") {
      Line().add(op).add_bool(nested.has(arg(tok, 1, 0), arg(tok, 2, 0))).out();

    } else if (op == "nested_del") {
      Line().add(op).add_bool(nested.remove(arg(tok, 1, 0), arg(tok, 2, 0))).out();

    } else if (op == "nested_clear") {
      nested.clear();
      Line().add(op).out();

    } else if (op == "nested_multi_add") {
      multi.add(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0));
      Line().add(op).add(multi.collect(arg(tok, 1, 0), arg(tok, 2, 0)).size()).out();

    } else if (op == "nested_multi_first") {
      const std::optional<double> r = multi.first(arg(tok, 1, 0), arg(tok, 2, 0));
      Line L;
      L.add(op);
      if (r.has_value()) L.add_bits(*r);
      else L.add("-");
      L.out();

    } else if (op == "nested_multi_has") {
      Line().add(op).add_bool(multi.has(arg(tok, 1, 0), arg(tok, 2, 0))).out();

    } else if (op == "nested_multi_collect") {
      emit_list(op.c_str(), multi.collect(arg(tok, 1, 0), arg(tok, 2, 0)));

    } else if (op == "nested_multi_del") {
      Line().add(op).add_bool(multi.remove(arg(tok, 1, 0), arg(tok, 2, 0))).out();

    } else if (op == "nested_multi_clear") {
      multi.clear();
      Line().add(op).out();

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
