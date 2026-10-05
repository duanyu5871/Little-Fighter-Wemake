#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/loader/preprocess_bdy.h"
#include "lfw/loader/preprocess_itr.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

std::string probe_of(const lfw::Value& v, const char16_t* key) {
  const lfw::Object* o = lfw::as_object(v);
  if (o == nullptr) return "-";
  const lfw::Value* p = o->get(std::u16string(key));
  if (p == nullptr) return "-";
  return lfw::truthy(*p) ? "s" : "u";
}

std::string tester_probe(const lfw::Value& v) {
  std::string probe = probe_of(v, u"__tester");
  const lfw::Value acts = lfw::field_or(v, u"actions");
  const lfw::Array* a = lfw::as_array(acts);
  if (a == nullptr) return probe;
  for (size_t i = 0; i < a->size(); ++i) probe += "," + probe_of(a->at(i), u"tester");
  return probe;
}

// TS 侧把编译好的 `Expression` 挂在这些键上（端口存的是源串），两端都不可比，剥掉后再比其余字段。
void strip_compiled(lfw::Value& v) {
  if (lfw::Array* a = lfw::as_array(v)) {
    for (size_t i = 0; i < a->size(); ++i) strip_compiled(a->at(i));
    return;
  }
  lfw::Object* const o = lfw::as_object(v);
  if (o == nullptr) return;
  o->remove(u"__tester");
  o->remove(u"__judger");
  o->remove(u"tester");
  const std::vector<std::u16string> keys = o->keys();
  for (const std::u16string& k : keys) {
    const lfw::Value* p = o->get(k);
    if (p == nullptr) continue;
    lfw::Value child = *p;
    strip_compiled(child);
  }
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_loader_frames <case-file>\n");
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

    if (op == "bdy" || op == "itr") {
      lfw::Value ctx = parse_value(t, i);
      std::u16string error;
      const bool ok = op == "bdy" ? lfw::loader::preprocess_bdy(ctx, error)
                                  : lfw::loader::preprocess_itr(ctx, error);
      const lfw::Value result = lfw::field_or(ctx, op == "bdy" ? u"bdy" : u"itr");
      const std::string probe = tester_probe(result);
      strip_compiled(ctx);
      std::string out = op + (ok ? " ok" : " throw");
      out += " " + render(lfw::field_or(ctx, u"data"));
      if (op == "bdy") out += " " + render(lfw::field_or(ctx, u"frame"));
      out += " " + render(result);
      out += " t=" + probe;
      if (!ok) {
        const std::string msg = to_ascii(error);
        out += " msg=" + (!msg.empty() && msg[0] == '[' ? msg : "-");
      }
      emit(out);
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
    }
  }
  return 0;
}
