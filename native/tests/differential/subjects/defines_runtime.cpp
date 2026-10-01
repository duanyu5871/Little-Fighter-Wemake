#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/core/json5.h"
#include "lfw/core/value.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/runtime_gen.h"

#include "trace_util.h"

namespace {

using trace::Line;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_double;
using trace::to_u16;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void out_entry(const lfw::DefinesRuntimeEntry& e) {
  const lfw::Json5Result r = lfw::json5_parse(e.value_json5);
  Line out;
  out.add(std::string_view("E")).add(to_ascii(e.name));
  if (!r.ok) out.add(std::string_view("PERR")).add(to_ascii(r.error));
  else out.add(render(r.value));
  out.out();
}

std::u16string arg_text(const std::vector<std::string>& t, size_t& i) {
  return lfw::to_string(trace::parse_value(t, i));
}

void out_keys(const std::u16string& id) {
  const lfw::Object* o = lfw::defines::get_default_keys(id);
  Line out;
  out.add(std::string_view("K")).add(render(lfw::Value(id)));
  if (o == nullptr) {
    out.add(std::string_view("-"));
  } else {
    static const char16_t* const kFields[] = {u"L", u"R", u"U", u"D", u"a", u"j", u"d"};
    for (const char16_t* f : kFields) {
      const lfw::Value* v = o->get(std::u16string(f));
      out.add(v != nullptr ? render(*v) : std::string("-"));
    }
  }
  out.out();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_defines_runtime <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  const std::vector<lfw::DefinesRuntimeEntry>& entries = lfw::defines_runtime_entries();

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::string line = trace::strip_comment(raw);
    const std::vector<std::string> t = split_ws(line);
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    if (op == "count") {
      Line out;
      out.add(std::string_view("N")).add(std::to_string(entries.size()));
      size_t top = 0;
      for (const lfw::DefinesRuntimeEntry& e : entries) {
        if (e.is_top_level) ++top;
      }
      out.add(std::to_string(top));
      out.out();
    } else if (op == "all") {
      for (const lfw::DefinesRuntimeEntry& e : entries) out_entry(e);
    } else if (op == "top") {
      for (const lfw::DefinesRuntimeEntry& e : entries) {
        if (e.is_top_level) out_entry(e);
      }
    } else if (op == "prefix") {
      const std::u16string p = to_u16(t[i++]);
      for (const lfw::DefinesRuntimeEntry& e : entries) {
        if (e.name.starts_with(p)) out_entry(e);
      }
    } else if (op == "num") {
      const std::u16string n = to_u16(t[i++]);
      Line out;
      out.add(std::string_view("D")).add(to_ascii(n)).add(render(lfw::Value(lfw::defines::num(n.c_str()))));
      out.out();
    } else if (op == "has") {
      const std::u16string n = to_u16(t[i++]);
      Line out;
      out.add(std::string_view("H"))
          .add(to_ascii(n))
          .add(render(lfw::Value(lfw::defines::find(n) != nullptr)));
      out.out();
    } else if (op == "desire") {
      const double ratio = to_double(t[i++]);
      Line out;
      out.add(std::string_view("DS")).add(render(lfw::Value(lfw::defines::desire(ratio))));
      out.out();
    } else if (op == "keys") {
      out_keys(arg_text(t, i));
    } else if (op == "isind") {
      Line out;
      out.add(std::string_view("I")).add(render(lfw::Value(lfw::defines::is_independent(arg_text(t, i)))));
      out.out();
    } else if (op == "ischeat") {
      Line out;
      out.add(std::string_view("C")).add(render(lfw::Value(lfw::defines::is_cheat_type(arg_text(t, i)))));
      out.out();
    } else if (op == "isdiff") {
      Line out;
      out.add(std::string_view("F")).add(render(lfw::Value(lfw::defines::is_difficulty(to_double(t[i++])))));
      out.out();
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
