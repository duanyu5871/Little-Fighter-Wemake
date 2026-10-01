#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "lfw/defines/fields_gen.h"

#include "trace_util.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_defines_fields <case-file>\n");
    return 2;
  }

  std::ifstream probe(argv[1]);
  if (!probe) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  std::vector<const lfw::FieldTableRef*> rows;
  for (const lfw::FieldTableRef& t : lfw::all_field_table_refs()) rows.push_back(&t);
  std::sort(rows.begin(), rows.end(), [](const lfw::FieldTableRef* a, const lfw::FieldTableRef* b) {
    return std::u16string_view(a->name) < std::u16string_view(b->name);
  });

  for (const lfw::FieldTableRef* t : rows) {
    trace::Line()
        .add(std::string_view("T"))
        .add(trace::to_ascii(t->name))
        .add(trace::to_ascii(trace::render_value(t->get())))
        .out();
  }

  return 0;
}
