#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/defines/all_enums.h"

#include "trace_util.h"

namespace {

using trace::esc;
using trace::Line;
using trace::to_ascii;

struct Row {
  const char16_t* name;
};

void emit_header(const char16_t* name) {
  Line out;
  out.add(std::string_view("E")).add(to_ascii(name));
  out.out();
}

void emit_number(const lfw::EnumNumberTableRef& t) {
  std::vector<const lfw::EnumNumberEntry*> rows;
  for (const lfw::EnumNumberEntry& e : *t.entries) rows.push_back(&e);
  std::sort(rows.begin(), rows.end(), [](const lfw::EnumNumberEntry* a, const lfw::EnumNumberEntry* b) {
    return std::u16string_view(a->name) < std::u16string_view(b->name);
  });

  emit_header(t.name);
  for (const lfw::EnumNumberEntry* e : rows) {
    Line().add(std::string_view("F")).add(to_ascii(e->name)).add(to_ascii(lfw::number_to_string(e->value))).out();
  }

  std::vector<double> values;
  for (const lfw::EnumNumberEntry* e : rows) {
    if (std::find(values.begin(), values.end(), e->value) == values.end()) values.push_back(e->value);
  }
  std::sort(values.begin(), values.end());
  for (double v : values) {
    const char16_t* name = t.name_of(static_cast<int>(v));
    Line out;
    out.add(std::string_view("R")).add(to_ascii(lfw::number_to_string(v)));
    out.add(name != nullptr ? std::string_view(to_ascii(name)) : std::string_view("-"));
    out.out();
  }
}

void emit_text(const lfw::EnumTextTableRef& t) {
  std::vector<const lfw::EnumTextEntry*> rows;
  for (const lfw::EnumTextEntry& e : *t.entries) rows.push_back(&e);
  std::sort(rows.begin(), rows.end(), [](const lfw::EnumTextEntry* a, const lfw::EnumTextEntry* b) {
    return std::u16string_view(a->name) < std::u16string_view(b->name);
  });

  emit_header(t.name);
  for (const lfw::EnumTextEntry* e : rows) {
    Line().add(std::string_view("F")).add(to_ascii(e->name)).add(esc(e->text)).out();
  }
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_defines <case-file>\n");
    return 2;
  }

  std::ifstream probe(argv[1]);
  if (!probe) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  std::vector<std::u16string> names;
  for (const lfw::EnumNumberTableRef& t : lfw::all_number_enum_tables()) names.emplace_back(t.name);
  for (const lfw::EnumTextTableRef& t : lfw::all_text_enum_tables()) names.emplace_back(t.name);
  std::sort(names.begin(), names.end());

  for (const std::u16string& name : names) {
    for (const lfw::EnumNumberTableRef& t : lfw::all_number_enum_tables()) {
      if (std::u16string_view(t.name) == std::u16string_view(name)) emit_number(t);
    }
    for (const lfw::EnumTextTableRef& t : lfw::all_text_enum_tables()) {
      if (std::u16string_view(t.name) == std::u16string_view(name)) emit_text(t);
    }
  }

  return 0;
}
