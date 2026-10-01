#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/helpers.h"

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

void reset_obj(const std::string& id) {
  g_objs[id] = lfw::Value(std::make_shared<lfw::Object>());
}

std::string opt_render(const std::optional<double>& d) {
  return d.has_value() ? render(lfw::Value(*d)) : std::string("u");
}

void out_tagged(const char* tag, const std::string& a, const std::string& b) {
  emit(std::string(tag) + " " + a + " " + b);
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_dat_helpers <case-file>\n");
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

    if (op == "new") {
      reset_obj(t[i++]);
    } else if (op == "set") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      obj_of(id).set(key, value_of(t, i));
    } else if (op == "setobj") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      obj_of(id).set(key, g_objs[t[i++]]);
    } else if (op == "setarr") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      lfw::Array arr;
      while (i < t.size()) arr.push_back(value_of(t, i));
      obj_of(id).set(key, lfw::Value(std::make_shared<lfw::Array>(arr)));
    } else if (op == "del") {
      const std::string id = t[i++];
      obj_of(id).remove(to_u16(t[i++]));
    } else if (op == "dump") {
      const std::string id = t[i++];
      out_tagged("D", id, render(g_objs[id]));
    } else if (op == "take") {
      const std::string id = t[i++];
      const std::string key = t[i++];
      out_tagged("T", id + " " + key, render(lfw::dat_translator::take(obj_of(id), to_u16(key))));
    } else if (op == "takestr") {
      const std::string id = t[i++];
      const std::string key = t[i++];
      const std::optional<std::u16string> r = lfw::dat_translator::take_str(obj_of(id), to_u16(key));
      out_tagged("TS", id + " " + key, r.has_value() ? render(lfw::Value(*r)) : std::string("u"));
    } else if (op == "takenum" || op == "takepos" || op == "takenonzero") {
      const std::string id = t[i++];
      const std::string key = t[i++];
      const std::optional<double> r = op == "takenum"
                                          ? lfw::dat_translator::take_num(obj_of(id), to_u16(key))
                                      : op == "takepos"
                                          ? lfw::dat_translator::take_positive_num(obj_of(id), to_u16(key))
                                          : lfw::dat_translator::take_not_zero_num(obj_of(id), to_u16(key));
      out_tagged(op == "takenum" ? "TN" : (op == "takepos" ? "TP" : "TZ"), id + " " + key, opt_render(r));
    } else if (op == "takenumf") {
      const std::string id = t[i++];
      const std::string key = t[i++];
      const double mul = to_double(t[i++]);
      const std::optional<double> r = lfw::dat_translator::take_num(
          obj_of(id), to_u16(key), [mul](double n) { return n * mul; });
      out_tagged("TNF", id + " " + key, opt_render(r));
    } else if (op == "mp") {
      const std::string id = t[i++];
      const std::pair<double, double> r = lfw::dat_translator::take_raw_frame_mp(obj_of(id));
      out_tagged("MP", id, render(lfw::Value(r.first)) + " " + render(lfw::Value(r.second)));
    } else if (op == "flag") {
      const std::string id = t[i++];
      const lfw::Value v = value_of(t, i);
      lfw::dat_translator::set_hit_flag(obj_of(id), v);
      out_tagged("SG", id, render(g_objs[id]));
    } else if (op == "bdykind") {
      const std::string id = t[i++];
      const lfw::Value v = value_of(t, i);
      lfw::dat_translator::set_bdy_kind(obj_of(id), v);
      out_tagged("SB", id, render(g_objs[id]));
    } else if (op == "ff") {
      const lfw::Value nv = value_of(t, i);
      const double n = std::get_if<double>(&nv) != nullptr ? std::get<double>(nv) : 0;
      const double d = i < t.size() ? to_double(t[i++]) : 1;
      emit(std::string("F ") + render(lfw::Value(lfw::dat_translator::fixed_float(n, d))));
    } else if (op == "find") {
      const std::string id = t[i++];
      const std::pair<bool, std::u16string> r = lfw::dat_translator::find_float(g_objs[id]);
      out_tagged("R", id, render(lfw::Value(r.first)) + " " + render(lfw::Value(r.second)));
    } else if (op == "copybdy" || op == "copyitr") {
      const std::string id = t[i++];
      const lfw::Object edit;
      const lfw::Value r = op == "copybdy" ? lfw::dat_translator::copy_bdy_info(g_objs[id], edit)
                                           : lfw::dat_translator::copy_itr_info(g_objs[id], edit);
      out_tagged(op == "copybdy" ? "CB" : "CI", id, render(r));
    } else if (op == "copybdye" || op == "copyitre") {
      const std::string id = t[i++];
      const std::u16string key = to_u16(t[i++]);
      lfw::Object edit;
      edit.set(key, value_of(t, i));
      const lfw::Value r = op == "copybdye" ? lfw::dat_translator::copy_bdy_info(g_objs[id], edit)
                                            : lfw::dat_translator::copy_itr_info(g_objs[id], edit);
      out_tagged(op == "copybdye" ? "CBE" : "CIE", id, render(r));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
