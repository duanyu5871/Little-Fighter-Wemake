#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/i18n.h"
#include "lfw/loader/get_import_fallbacks.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::map<std::string, lfw::I18N> g_insts;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

lfw::I18N& inst(const std::string& id) { return g_insts[id]; }

lfw::Value as_array_value(const std::vector<std::u16string>& xs) {
  lfw::Array a;
  for (const std::u16string& x : xs) a.push_back(lfw::Value(x));
  return lfw::Value(std::make_shared<lfw::Array>(a));
}

std::string message_of(const std::u16string& error) {
  const std::string msg = to_ascii(error);
  return !msg.empty() && msg[0] == '[' ? msg : "-";
}

// JS 的默认参数只对 `undefined` 生效（显式的 `u` 也一样）⇒ 缺省或 `u` 时取当前语言。
lfw::Value lang_arg(const std::vector<std::string>& t, size_t& i, const lfw::I18N& it) {
  lfw::Value v = i < t.size() ? parse_value(t, i) : lfw::Value();
  if (std::holds_alternative<std::monostate>(v)) return lfw::Value(it.lang());
  return v;
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_i18n <case-file>\n");
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

    if (op == "gif") {
      const lfw::Value name = parse_value(t, i);
      std::vector<std::u16string> fallbacks;
      std::u16string suffix;
      const bool ok = lfw::loader::get_import_fallbacks(name, fallbacks, suffix);
      std::string out = std::string("gif ") + (ok ? "ok" : "throw");
      out += ok ? " " + render(as_array_value(fallbacks)) : " -";
      out += " suffix=" + (ok ? render(lfw::Value(suffix)) : std::string("-"));
      if (!ok) out += " msg=-";
      emit(out);
      continue;
    }

    if (op == "new") {
      const std::string id = t[i++];
      g_insts[id] = lfw::I18N();
      emit("new " + id);
      continue;
    }

    const std::string id = t[i++];
    lfw::I18N& it = inst(id);

    if (op == "add") {
      const lfw::Value langs = parse_value(t, i);
      it.add(langs);
      emit("add " + id + " " + render(langs));
      continue;
    }
    if (op == "lang") {
      const lfw::Value v = parse_value(t, i);
      std::u16string error;
      const bool ok = it.set_lang(v, error);
      std::string out = "lang " + id + (ok ? " ok" : " throw");
      out += " cur=" + render(lfw::Value(it.lang()));
      out += " msg=" + (ok ? std::string("-") : message_of(error));
      emit(out);
      continue;
    }
    if (op == "alias" || op == "canonical") {
      const lfw::Value v = lang_arg(t, i, it);
      const lfw::Value got = op == "alias" ? it.alias(v) : it.canonical(v);
      emit(op + " " + id + " " + render(got));
      continue;
    }
    if (op == "str" || op == "strs") {
      const lfw::Value name = parse_value(t, i);
      const lfw::Value lang = lang_arg(t, i, it);
      const lfw::Value got = op == "str" ? it.string(name, lang) : it.strings(name, lang);
      emit(op + " " + id + " " + render(got));
      continue;
    }

    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
