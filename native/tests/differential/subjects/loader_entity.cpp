#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/loader/preprocess_entity_data.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

// TS 侧把编译好的 `Expression` / `ValExpression` 挂在这些键上（`__tester` / `action.tester` /
// `__judger` / bot 动作的 `judger` / `opoint.__gen_*`；端口存的是源串或不写），两端都不可比，
// 剥掉后再比其余字段。
void strip_compiled(lfw::Value& v) {
  if (lfw::Array* a = lfw::as_array(v)) {
    for (size_t i = 0; i < a->size(); ++i) strip_compiled(a->at(i));
    return;
  }
  lfw::Object* const o = lfw::as_object(v);
  if (o == nullptr) return;
  o->remove(u"__tester");
  o->remove(u"__judger");
  o->remove(u"judger");
  o->remove(u"tester");
  for (const char16_t* const key : {u"__gen_x", u"__gen_y", u"__gen_z", u"__gen_dvx", u"__gen_dvy",
                                    u"__gen_dvz", u"__gen_spread_x", u"__gen_spread_y",
                                    u"__gen_spread_z", u"__gen_facing"}) {
    o->remove(std::u16string(key));
  }
  const std::vector<std::u16string> keys = o->keys();
  for (const std::u16string& k : keys) {
    const lfw::Value* p = o->get(k);
    if (p == nullptr) continue;
    lfw::Value child = *p;
    strip_compiled(child);
  }
}

// 与 TS 侧对称：`lfw` 只用来往 `jobs` 里塞加载任务，端口整个不落地（见 DESIGN §66）；用例没写
// `lfw` 时补一个桩，免得 `const { images, sounds } = lfw` 那一关先把端口拦下来。
void ensure_lfw_stub(lfw::Value& ctx) {
  lfw::Object* const o = lfw::as_object(ctx);
  if (o == nullptr || o->get(u"lfw") != nullptr) return;
  const lfw::Value empty = lfw::dat_translator::make_obj({});
  o->set(u"lfw", lfw::dat_translator::make_obj({{u"images", empty}, {u"sounds", empty}}));
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_loader_entity <case-file>\n");
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

    if (op == "ed") {
      lfw::Value ctx = parse_value(t, i);
      ensure_lfw_stub(ctx);
      std::u16string error;
      const bool ok = lfw::loader::preprocess_entity_data(ctx, error);
      const lfw::Value data = lfw::field_or(ctx, u"data");
      strip_compiled(ctx);
      std::string out = std::string("ed ") + (ok ? "ok" : "throw");
      out += " " + render(data);
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
