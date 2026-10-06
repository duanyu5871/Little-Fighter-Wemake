#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/player_info.h"
#include "lfw/utils/utf8.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

bool is_nullish(const lfw::Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<lfw::NullTag>(v);
}

struct Entry {
  bool missing = false;
  bool get_threw = false;
  bool del_threw = false;

  enum class DataKind { kFalsy, kBytes, kOther };
  DataKind data_kind = DataKind::kFalsy;
  std::vector<uint8_t> data;
  lfw::Value data_other;

  enum class BlobKind { kFalsy, kBytes, kThrows };
  BlobKind blob_kind = BlobKind::kFalsy;
  std::vector<uint8_t> blob;
};

std::vector<std::string> g_log;
std::map<std::string, Entry> g_entries;
std::map<std::string, std::unique_ptr<lfw::PlayerInfo>> g_players;

void push(const std::string& line) { g_log.push_back(line); }

// TS 台面的 `bytes_of`：字符串 ⇒ UTF-8 字节；其它 ⇒ `String(v)` 的 UTF-8；数组 ⇒ 逐元素
// `Number(x) & 0xff`。
std::vector<uint8_t> bytes_of(const lfw::Value& v) {
  if (const lfw::Array* const a = lfw::as_array(v)) {
    std::vector<uint8_t> out;
    for (size_t i = 0; i < a->size(); ++i) {
      const double n = lfw::to_number(a->at(i));
      out.push_back(std::isnan(n) ? 0 : static_cast<uint8_t>(static_cast<int64_t>(n)));
    }
    return out;
  }
  return lfw::encode_utf8(lfw::to_string(v));
}

std::string storage_key(const std::string& pid) { return "player_info_" + pid; }

class Host : public lfw::IPlayerInfoHost {
 public:
  void cache_get(const std::u16string& name, lfw::PlayerInfoCacheEntry& out) override {
    push("get:" + to_ascii(name));
    const auto it = g_entries.find(to_ascii(name));
    if (it == g_entries.end()) {
      out.missing = true;
      return;
    }
    const Entry& e = it->second;
    if (e.get_threw) {
      out.get_threw = true;
      return;
    }
    if (e.missing) {
      out.missing = true;
      return;
    }
    switch (e.data_kind) {
      case Entry::DataKind::kFalsy: out.data_kind = lfw::PlayerInfoCacheEntry::DataKind::kFalsy; break;
      case Entry::DataKind::kBytes: {
        out.data_kind = lfw::PlayerInfoCacheEntry::DataKind::kBytes;
        out.data = e.data;
        return;
      }
      case Entry::DataKind::kOther: {
        out.data_kind = lfw::PlayerInfoCacheEntry::DataKind::kOther;
        out.data_other = e.data_other;
        return;
      }
    }
    // TS 里 `blob` 只在 `data` 假值时才被读；这里也只在那种情况给。
    switch (e.blob_kind) {
      case Entry::BlobKind::kFalsy:
        out.blob_kind = lfw::PlayerInfoCacheEntry::BlobKind::kFalsy;
        break;
      case Entry::BlobKind::kBytes:
        out.blob_kind = lfw::PlayerInfoCacheEntry::BlobKind::kBytes;
        out.blob = e.blob;
        break;
      case Entry::BlobKind::kThrows:
        out.blob_kind = lfw::PlayerInfoCacheEntry::BlobKind::kThrows;
        break;
    }
  }

  bool cache_del(const std::u16string& name, std::u16string& error) override {
    push("del:" + to_ascii(name));
    const auto it = g_entries.find(to_ascii(name));
    if (it != g_entries.end() && it->second.del_threw) {
      error = u"del failed";
      return false;
    }
    return true;
  }

  void cache_put(const lfw::PlayerInfoCachePut& data) override {
    std::string bytes;
    for (size_t i = 0; i < data.data.size(); ++i) {
      if (i != 0) bytes += ",";
      bytes += std::to_string(static_cast<unsigned>(data.data[i]));
    }
    push("put:" + to_ascii(data.name) + "|" + to_ascii(data.type) + "|" +
         to_ascii(lfw::number_to_string(data.version)) + "|" + bytes);
  }

  void warn(const std::u16string& text) override { push("warn:" + to_ascii(text)); }
};

Host g_host;

void watch(lfw::PlayerInfo& pi) {
  pi.callbacks.on(u"on_name_changed", [](const std::vector<lfw::Value>& args) {
    std::string line = "cb:name:";
    for (size_t i = 0; i < args.size() && i < 2; ++i) {
      if (i != 0) line += "|";
      line += to_ascii(render_value(args[i]));
    }
    push(line);
  });
  pi.callbacks.on(u"on_ctrl_changed", [](const std::vector<lfw::Value>& args) {
    std::string line = "cb:ctrl:";
    for (size_t i = 0; i < args.size() && i < 2; ++i) {
      if (i != 0) line += "|";
      line += to_ascii(render_value(args[i]));
    }
    push(line);
  });
  pi.callbacks.on(u"on_is_com_changed", [](const std::vector<lfw::Value>& args) {
    std::string line = "cb:is_com:";
    for (size_t i = 0; i < args.size() && i < 1; ++i) {
      if (i != 0) line += "|";
      line += to_ascii(render_value(args[i]));
    }
    push(line);
  });
  pi.callbacks.on(u"on_key_changed", [](const std::vector<lfw::Value>& args) {
    std::string line = "cb:key:";
    for (size_t i = 0; i < args.size() && i < 3; ++i) {
      if (i != 0) line += "|";
      line += to_ascii(render_value(args[i]));
    }
    push(line);
  });
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_player_info <case-file>\n");
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

    if (op.rfind("cache_", 0) == 0) {
      const std::string pid = t[i++];
      Entry& e = g_entries[storage_key(pid)];
      const std::string& key = op;
      if (key == "cache_ok") {
        e.data_kind = Entry::DataKind::kBytes;
        const lfw::Value v = parse_value(t, i);
        // TS 台面是 `bytes_of(arg() ?? "")`。
        e.data = is_nullish(v) ? std::vector<uint8_t>() : bytes_of(v);
      } else if (key == "cache_bytes") {
        e.data_kind = Entry::DataKind::kBytes;
        e.data = bytes_of(parse_value(t, i));
      } else if (key == "cache_other") {
        // TS 台面是「原样塞进 `data`」：假值仍走 `if (!data)` 那条路（等于 `cache_nulldata`）。
        const lfw::Value v = parse_value(t, i);
        if (lfw::truthy(v)) {
          e.data_kind = Entry::DataKind::kOther;
          e.data_other = v;
        } else {
          e.data_kind = Entry::DataKind::kFalsy;
        }
      } else if (key == "cache_nulldata") {
        e.data_kind = Entry::DataKind::kFalsy;
      } else if (key == "cache_blob") {
        e.blob_kind = Entry::BlobKind::kBytes;
        const lfw::Value v = parse_value(t, i);
        e.blob = is_nullish(v) ? std::vector<uint8_t>() : bytes_of(v);
      } else if (key == "cache_blobbytes") {
        e.blob_kind = Entry::BlobKind::kBytes;
        e.blob = bytes_of(parse_value(t, i));
      } else if (key == "cache_blobfail") {
        e.blob_kind = Entry::BlobKind::kThrows;
      } else if (key == "cache_blobother") {
        e.blob_kind = Entry::BlobKind::kThrows;
        parse_value(t, i);   // 台面照收这个值（`blob.arrayBuffer` 不是函数 ⇒ 抛）
      } else if (key == "cache_missing") {
        e.missing = true;
      } else if (key == "cache_getfail") {
        e.get_threw = true;
      } else if (key == "cache_delfail") {
        e.del_threw = true;
      } else {
        std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
        return 2;
      }
    } else if (op == "new") {
      const std::string pid = t[i++];
      lfw::Value name;
      lfw::Value local;
      lfw::Value mine;
      if (i < t.size()) name = parse_value(t, i);
      if (i < t.size()) local = parse_value(t, i);
      if (i < t.size()) mine = parse_value(t, i);
      auto pi = std::make_unique<lfw::PlayerInfo>(&g_host, trace::to_u16(pid), name, local, mine);
      watch(*pi);
      pi->loaded();
      push("new:" + pid);
      g_players[pid] = std::move(pi);
    } else {
      const std::string pid = t[i++];
      lfw::PlayerInfo& pi = *g_players[pid];
      if (op == "dump") {
        std::string out = "dump:" + pid + "|info=" + to_ascii(render_value(pi.info())) +
                          "|name=" + to_ascii(render_value(pi.name())) +
                          "|ctrl=" + to_ascii(render_value(pi.ctrl())) +
                          "|local=" + to_ascii(render_value(pi.local())) +
                          "|mine=" + to_ascii(render_value(pi.mine())) +
                          "|is_com=" + (pi.is_com() ? "1" : "0") +
                          "|loaded=" + (pi.loaded() ? "1" : "0") +
                          "|fighter=" + (pi.fighter() != nullptr ? "1" : "0");
        push(out);
      } else if (op == "reload") {
        push("reload:" + pid + ":" + (pi.load() ? "1" : "0"));
      } else if (op == "save") {
        pi.save();
        push("save:" + pid);
      } else if (op == "setname") {
        const lfw::Value v = parse_value(t, i);
        pi.set_name(v, lfw::truthy(parse_value(t, i)));
      } else if (op == "setctrl") {
        const lfw::Value v = parse_value(t, i);
        pi.set_ctrl(v, lfw::truthy(parse_value(t, i)));
      } else if (op == "setiscom") {
        const bool v = lfw::truthy(parse_value(t, i));
        pi.set_is_com(v, lfw::truthy(parse_value(t, i)));
      } else if (op == "setkey") {
        const lfw::Value k = parse_value(t, i);
        const lfw::Value v = parse_value(t, i);
        const bool emit = lfw::truthy(parse_value(t, i));
        std::u16string error;
        const bool ok = pi.set_key(k, v, emit, error);
        push("setkey:" + pid + ":" + (ok ? "ok" : "throw"));
      } else if (op == "getkey") {
        lfw::Value out;
        std::u16string error;
        const lfw::Value k = parse_value(t, i);
        const bool ok = pi.get_key(k, out, error);
        push("getkey:" + pid + ":" + (ok ? to_ascii(render_value(out)) : std::string("throw")));
      } else if (op == "setfighter") {
        static int sentinel = 0;
        pi.set_fighter(lfw::truthy(parse_value(t, i))
                           ? reinterpret_cast<lfw::Entity*>(&sentinel)
                           : nullptr);
      } else {
        std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
        return 2;
      }
    }

    if (i != t.size()) {
      std::fprintf(stderr, "trailing token(s) at line %d: %s\n", lineno, raw.c_str());
      return 2;
    }
    for (const std::string& l : g_log) std::printf("%s\n", l.c_str());
    g_log.clear();
  }
  return 0;
}
