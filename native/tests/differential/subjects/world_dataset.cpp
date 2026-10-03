#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/world_dataset.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::WorldDataset;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;

std::string render(const Value& v) { return to_ascii(render_value(v)); }
std::string s_of(const std::u16string& s) { return to_ascii(s); }

std::string join(const std::vector<std::string>& xs) {
  std::string out;
  for (size_t i = 0; i < xs.size(); ++i) {
    if (i) out += ",";
    out += xs[i];
  }
  return out;
}

bool g_pure = false;
std::u16string g_hook_key;
bool g_has_hook = false;
std::unique_ptr<WorldDataset> g_owned;
WorldDataset* g_ds = nullptr;
WorldDataset* g_bound = nullptr;

void bind() {
  if (g_ds == nullptr || g_bound == g_ds) return;
  g_bound = g_ds;
  g_ds->on_dataset_change = [](const std::u16string& key, const Value& curr,
                               const Value& prev) {
    g_log.push_back("dataset_change:" + s_of(key) + ":" + render(curr) + ":" + render(prev));
  };
  if (g_has_hook) {
    const std::u16string key = g_hook_key;
    g_ds->set_field_hook(key, [key](const Value& curr, const Value& prev) {
      g_log.push_back("field_change:" + s_of(key) + ":" + render(curr) + ":" + render(prev));
    });
  }
}

std::string keys_of(const WorldDataset& ds) {
  const std::vector<std::u16string> enumerable = ds.enumerable_keys();
  std::vector<std::string> keys;
  keys.reserve(enumerable.size());
  for (const std::u16string& key : enumerable) keys.push_back(s_of(key));
  return join(keys);
}

std::u16string text_of(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : to_string(v);
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_world_dataset <case-file>\n");
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
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;
    g_log.clear();
    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "pure") {
        g_pure = lfw::truthy(parse_value(t, i));
      } else if (sub == "hook") {
        g_hook_key = text_of(parse_value(t, i));
        g_has_hook = true;
      } else {
        std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      std::printf("env %s\n", sub.c_str());
      continue;
    }
    if (op == "run") {
      const std::string& what = t[i++];
      if (what == "make") {
        g_owned = std::make_unique<WorldDataset>(g_pure);
        g_ds = g_owned.get();
        g_bound = nullptr;
        bind();
        std::printf("run make || pure=%s keys=%s\n", render(Value(g_pure)).c_str(),
                    keys_of(*g_ds).c_str());
      } else if (what == "default") {
        g_owned.reset();
        WorldDataset& a = WorldDataset::default_instance();
        WorldDataset& b = WorldDataset::default_instance();
        g_ds = &b;
        g_bound = nullptr;
        bind();
        std::printf("run default || same=%s keys=%s\n", &a == &b ? "b1" : "b0",
                    keys_of(b).c_str());
      } else if (what == "keys") {
        std::printf("run keys || %s\n", keys_of(*g_ds).c_str());
      } else if (what == "dump") {
        std::printf("run dump || %s\n", render(g_ds->dump_dataset()).c_str());
      } else if (what == "get") {
        const std::u16string key = text_of(parse_value(t, i));
        std::printf("run get %s || %s | v=%s\n", s_of(key).c_str(), join(g_log).c_str(),
                    render(g_ds->get(key)).c_str());
      } else if (what == "has") {
        const std::u16string key = text_of(parse_value(t, i));
        std::printf("run has %s || has=%s\n", s_of(key).c_str(),
                    g_ds->has(key) ? "b1" : "b0");
      } else if (what == "tracked") {
        const std::u16string key = text_of(parse_value(t, i));
        std::printf("run tracked %s || tracked=%s\n", s_of(key).c_str(),
                    g_ds->tracked(key) ? "b1" : "b0");
      } else if (what == "set") {
        const std::u16string key = text_of(parse_value(t, i));
        const Value value = parse_value(t, i);
        g_ds->set(key, value);
        std::printf("run set %s %s || %s\n", s_of(key).c_str(), render(value).c_str(),
                    join(g_log).c_str());
      } else {
        std::fprintf(stderr, "unknown run '%s' at line %d\n", what.c_str(), lineno);
        return 2;
      }
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
