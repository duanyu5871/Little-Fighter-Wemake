// `LFW`（门面 4AB）的 C++ 侧台面，op 与 `subjects/lfw.ts` 一一对应。
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/defines/defines_data.h"
#include "lfw/entity/entity.h"
#include "lfw/factory.h"
#include "lfw/lfw.h"
#include "lfw/loader/stage_val_getters.h"
#include "lfw/player_info.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using trace::key_of;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::strip_comment;
using trace::to_ascii;
using trace::to_double;
using trace::to_u16;

std::vector<std::string> g_log;

void push(const std::string& line) { g_log.push_back(line); }

std::string num(double d) { return to_ascii(render_value(lfw::Value(d))); }

std::string s_of(const lfw::Value& v) {
  const std::u16string* const s = std::get_if<std::u16string>(&v);
  return s != nullptr ? to_ascii(*s) : "u";
}

int g_ent_seq = 0;

// 实体宿主（静默）：`attach` 等副作用不发日志。
class EntHost : public lfw::IEntityHost {
 public:
  double game_time() const override { return 0.0; }
  std::u16string new_id() override {
    ++g_ent_seq;
    return u"e" + lfw::number_to_string(static_cast<double>(g_ent_seq));
  }
  void add_entities(lfw::Entity&) override {}
};

EntHost g_ent_host;

class FakeLayers : public lfw::IUiLayers {
 public:
  explicit FakeLayers(lfw::LFW& lfw) : _lfw(&lfw) {}
  void push() override {
    // TS 的 `layers.push()` 会触发 `on_push` → `lfw.callbacks.call("on_ui_changed", curr, prev)`
    // （空 uis ⇒ 两个 undefined）⇒ 端口同款回调一次。
    _lfw->ui_changed(nullptr, nullptr);
  }
  void set_page(const lfw::Value&, double) override {}
  void push_page(const lfw::Value&, double) override {}
  void dispose() override {}
  lfw::ui::UINode* ui() override { return nullptr; }
  std::vector<lfw::IWorldUi*> layer_uis() override { return {}; }

 private:
  lfw::LFW* _lfw;
};

class FakeHost : public lfw::ILfwHost {
 public:
  explicit FakeHost(lfw::LFW** slot) : _slot(slot) {}

  double now() override { return 12345.0; }
  void sounds_init(lfw::LFW&) override { push("snd_init"); }
  void images_init(lfw::LFW&) override { push("img_init"); }
  void keyboard_init(lfw::LFW&) override { push("kbd_init"); }
  void pointings_init(lfw::LFW&) override { push("pt_init"); }
  void keyboard_add_callback(lfw::LFW&) override { push("kbd_cbadd"); }
  void pointings_add_ui_input(lfw::LFW&) override { push("pt_cbadd"); }
  void cache_forget(const std::u16string& type, double version) override {
    push("cache:forget|" + to_ascii(type) + "|" + num(version));
  }
  void zip_forget_stored(const std::u16string& type, double version) override {
    push("zip:forget|" + to_ascii(type) + "|" + num(version));
  }
  void regist_components() override {}
  lfw::IUiLayers* create_layers(lfw::LFW& lfw) override {
    _layers = std::make_unique<FakeLayers>(lfw);
    return _layers.get();
  }
  lfw::IWorldRenderer* create_world_renderer(lfw::LFW&) override {
    push("wr_init");
    return &_renderer;
  }
  void load_img(const std::u16string&) override {}

  bool dev() const override { return false; }
  void warn(const std::vector<lfw::Value>& args) override { push(join("warn", args)); }
  void error(const std::vector<lfw::Value>& args) override { push(join("error", args)); }
  void log(const std::vector<lfw::Value>& args) override { push(join("Log", args)); }
  void debug_msg(const std::vector<lfw::Value>& args) override { push(join("debug", args)); }

  std::function<void()> sounds_play_bgm(const lfw::Value&) override {
    push("snd_bgm");
    return [] {};
  }
  void sounds_stop_bgm() override { push("snd_stop"); }
  void sounds_play(const lfw::Value&, const lfw::Value&, const lfw::Value&,
                   const lfw::Value&) override {
    push("snd_play");
  }
  void sounds_play_with_load(const lfw::Value&) override { push("snd_load"); }
  void sounds_dispose() override { push("snd_dispose"); }
  void keyboard_dispose() override { push("kbd_dispose"); }
  void pointings_dispose() override { push("pt_dispose"); }

  lfw::Value measure_text(const lfw::Value&, const lfw::Value&) override {
    push("measure");
    return lfw::Value(std::u16string());
  }

  void player_cache_get(const std::u16string&, lfw::PlayerInfoCacheEntry& out) override {
    out.missing = true;
  }
  bool player_cache_del(const std::u16string&, std::u16string&) override { return true; }
  void player_cache_put(const lfw::PlayerInfoCachePut&) override {}

  bool import_as_json(const std::vector<std::u16string>&, lfw::Value&, lfw::Value&,
                      std::u16string& error) override {
    error = u"host";
    return false;
  }
  bool import_as_blob_url(const std::vector<std::u16string>&, lfw::Value&, lfw::Value&,
                          std::u16string& error) override {
    error = u"host";
    return false;
  }
  bool import_as_array_buffer(const std::vector<std::u16string>&, lfw::Value&, lfw::Value&,
                              std::u16string& error) override {
    error = u"host";
    return false;
  }
  bool import_as_image_bitmap(const std::vector<std::u16string>&, lfw::Value&, lfw::Value&,
                              std::u16string& error) override {
    error = u"host";
    return false;
  }
  bool import_as_text(const std::vector<std::u16string>&, lfw::Value&, lfw::Value&,
                      std::u16string& error) override {
    error = u"host";
    return false;
  }
  bool xml_parse(const lfw::Value&, lfw::Value&, std::shared_ptr<lfw::IXMLElement>&,
                 std::u16string& error) override {
    error = u"host";
    return false;
  }

  void lang_apply(lfw::LFW& lfw, const std::u16string& lang, const std::u16string&) override {
    std::u16string err;
    lfw.i18n().set_lang(lfw::Value(lang), err);
  }

 private:
  static std::string join(const std::string& prefix, const std::vector<lfw::Value>& args) {
    std::string out = prefix;
    for (std::size_t i = 0; i < args.size(); i++) {
      out += (i == 0 ? "|" : "~") + to_ascii(render_value(args[i]));
    }
    return out;
  }

  lfw::LFW** _slot = nullptr;
  std::unique_ptr<FakeLayers> _layers;
  class Renderer : public lfw::IWorldRenderer {
   public:
    void add_entity(lfw::Entity&) override {}
    void del_entity(lfw::Entity&) override {}
    void render(double) override {}
    void dispose() override {}
  } _renderer;
};

std::vector<std::u16string> g_words;

void listen(lfw::LFW& lfw) {
  auto call = [&](const std::u16string& name, const lfw::LfwCallbacks::Payloads& p) {
    std::vector<lfw::LfwCallbacks::Payloads> packs = {p};
    (void)packs;
    const lfw::LfwCallbackArgs& a = p.empty() ? lfw::LfwCallbackArgs() : p[0];
    std::string line = "cb|" + to_ascii(name);
    if (name == u"on_ui_changed") {
      line += a.curr == nullptr ? "|u" : "|?";
      line += a.prev == nullptr ? "|u" : "|?";
    } else if (name == u"on_progress") {
      line += "|s:" + to_ascii(a.text) + "|n:" + num(a.num);
      if (a.has_num2) line += "|n:" + num(a.num2);
      else line += "|u";
    } else if (name == u"on_broadcast") {
      line += "|s:" + to_ascii(a.text) + "|self";
    } else if (name == u"on_cheat_changed") {
      line += "|s:" + to_ascii(a.text) + "|b:" + (a.flag ? "1" : "0");
    } else if (name == u"on_lang_changed") {
      line += "|s:" + to_ascii(a.text) + "|s:" + to_ascii(a.prev_text) + "|self";
    } else if (name == u"on_extra_zips_changed") {
      line += "|self";
    } else if (name == u"on_dispose") {
      line += "";
    } else if (name == u"controller_detected" || name == u"keyboard_detected") {
      line += "|pl:" + (a.player != nullptr ? to_ascii(a.player->id()) : "u");
    } else if (name == u"on_survival_rank_changed") {
      line += "|" + to_ascii(render_value(a.value)) + "|self";
    } else {
      line += "|?";
    }
    push(line);
  };

  for (const char16_t* const k :
       {u"on_ui_changed", u"on_loading_start", u"on_loading_end", u"on_loading_failed",
        u"on_progress", u"on_bgms_loaded", u"on_bgms_clear", u"on_player_infos_changed",
        u"on_cheat_changed", u"on_stage_pass", u"on_enter_next_stage", u"on_dispose",
        u"on_ui_loaded", u"on_prel_loaded", u"on_lang_changed", u"on_broadcast",
        u"on_survival_rank_changed", u"on_zips_changed", u"on_component_broadcast",
        u"on_extra_zips_changed", u"controller_detected", u"keyboard_detected"}) {
    const std::u16string key(k);
    lfw.callbacks.on(key, [call, key](const lfw::LfwCallbacks::Payloads& p) { call(key, p); });
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_lfw <case-file>\n");
    return 2;
  }
  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  lfw::Factory::register_entity(
      lfw::Value(8.0), [](lfw::World*, const lfw::Value& data, lfw::state::States*) -> lfw::Entity* {
        push("entadd:create|" + s_of(lfw::field_or(data, u"id")));
        return new lfw::Entity(g_ent_host, data);
      });

  lfw::LFW* slot = nullptr;
  FakeHost host(&slot);
  lfw::LFW lfw(host, false);
  slot = &lfw;
  listen(lfw);

  std::string raw;
  while (std::getline(in, raw)) {
    const std::vector<std::string> t = split_ws(strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    if (op == "info") {
      const lfw::Value& info = lfw::LFW::INFO();
      const lfw::Value version = lfw::field_or(info, u"version");
      push("info|" + s_of(lfw::field_or(info, u"title")) + "|" + s_of(lfw::field_or(info, u"type")) +
           "|" + num(lfw::to_number(version)) + "|default=" +
           (lfw::LFW::IS_DEFAULT_INFO() ? "1" : "0") + "|zips=" +
           to_ascii(lfw::number_to_string(static_cast<double>(lfw::LFW::ZIPS().size()))));
    } else if (op == "setinfo") {
      const std::u16string title = key_of(t[i++]);
      lfw::Object paths;
      (void)paths;
      auto arr = std::make_shared<lfw::Array>();
      arr->push_back(lfw::Value(std::u16string(u"prel.zip.json")));
      arr->push_back(lfw::Value(std::u16string(u"data.zip.json")));
      arr->push_back(lfw::Value(std::u16string(u"extra.zip.json")));
      lfw::Object o;
      o.set(u"type", lfw::Value(std::u16string(u"FULL")));
      o.set(u"version", lfw::Value(1.0));
      o.set(u"title", lfw::Value(title));
      o.set(u"description", lfw::Value(std::u16string(u"d")));
      o.set(u"author", lfw::Value(std::u16string(u"a")));
      o.set(u"paths", lfw::Value(std::move(arr)));
      const lfw::Value v(std::make_shared<lfw::Object>(o));
      lfw::LFW::set_INFO(&v);
    } else if (op == "setzips") {
      std::vector<lfw::LFW::ZipItem> zips;
      zips.push_back(lfw::LFW::ZipItem{key_of(t[i++]), nullptr});
      zips.push_back(lfw::LFW::ZipItem{key_of(t[i++]), nullptr});
      lfw::LFW::set_ZIPS(std::move(zips));
    } else if (op == "newid") {
      const double n = to_double(t[i++]);
      for (double k = 0; k < n; ++k) push("newid|" + to_ascii(lfw.new_id()));
    } else if (op == "newteam") {
      const double n = to_double(t[i++]);
      for (double k = 0; k < n; ++k) push("newteam|" + to_ascii(lfw.new_team()));
    } else if (op == "resetids") {
      lfw.reset_new_id();
    } else if (op == "resetteam") {
      lfw.reset_new_team();
    } else if (op == "player") {
      const std::u16string id = key_of(t[i++]);
      lfw::PlayerInfo* const p1 = lfw.player(id);
      lfw::PlayerInfo* const p2 = lfw.player(id);
      push("player|" + to_ascii(p1->id()) + "|" + (p1 == p2 ? "1" : "0") + "|local=" +
           (lfw::truthy(p1->local()) ? "1" : "0") + "|" + s_of(p1->name()) + "|count=" +
           to_ascii(lfw::number_to_string(static_cast<double>(lfw.players().size()))));
    } else if (op == "pkey") {
      const std::u16string pid = key_of(t[i++]);
      const std::u16string name = key_of(t[i++]);
      const std::u16string key = key_of(t[i++]);
      lfw::PlayerInfo* const p = lfw.player(pid);
      std::u16string err;
      const bool ok = p->set_key(lfw::Value(name), lfw::Value(key), false, err);
      push("pkey|" + to_ascii(p->id()) + "|" + (ok ? "1" : "0"));
    } else if (op == "pkeys") {
      lfw::PlayerInfo* const p = lfw.player(key_of(t[i++]));
      push("pkeys|" + to_ascii(p->id()) + "|" + to_ascii(render_value(p->keys())));
    } else if (op == "kbdown") {
      const std::u16string key = key_of(t[i++]);
      const double times = to_double(t[i++]);
      const std::string dev = t[i++];
      lfw::LfwKeyEvent e;
      e.key = key;
      e.times = times;
      if (dev != "-") e.device_type = key_of(dev);
      lfw.on_key_down(e);
      push("kbdown|" + to_ascii(key) + "|" + num(times) + "|" + dev + "|int=" +
           (e.interrupted ? "1" : "0"));
    } else if (op == "kbup") {
      const std::u16string key = key_of(t[i++]);
      lfw::LfwKeyEvent e;
      e.key = key;
      lfw.on_key_up(e);
      push("kbup|" + to_ascii(key));
    } else if (op == "cmds") {
      std::string joined;
      for (size_t k = 0; k < lfw.cmds.size(); ++k) {
        if (k != 0) joined += ";";
        joined += to_ascii(lfw.cmds[k]);
      }
      push("cmds|" + joined);
    } else if (op == "clearcmds") {
      lfw.cmds.clear();
    } else if (op == "ischeat") {
      const std::u16string n1 = key_of(t[i++]);
      const std::u16string n2 = key_of(t[i++]);
      push("ischeat|" + to_ascii(n1) + "|" + (lfw.is_cheat(n2) ? "1" : "0"));
    } else if (op == "setcheat") {
      const std::u16string name = key_of(t[i++]);
      const std::string en = t[i++];
      if (en == "-") lfw.set_cheat(name, std::nullopt);
      else lfw.set_cheat(name, en == "1");
    } else if (op == "ep") {
      const std::u16string c = key_of(t[i++]);
      const double p = to_double(t[i++]);
      lfw.emit_progress(c, p);
    } else if (op == "eps") {
      const std::u16string c = key_of(t[i++]);
      const double p = to_double(t[i++]);
      const std::string s = t[i++];
      if (s == "-") lfw.emit_progress(c, p);
      else lfw.emit_progress_size(c, p, lfw::Value(to_double(s)));
    } else if (op == "bcast") {
      lfw.broadcast(lfw::Value(key_of(t[i++])));
    } else if (op == "dataset") {
      const std::u16string k = key_of(t[i++]);
      lfw.world().dataset.set(k, parse_value(t, i));
    } else if (op == "switchdiff") {
      lfw.switch_difficulty(to_double(t[i++]));
    } else if (op == "lang") {
      push("lang|" + to_ascii(lfw.lang()));
    } else if (op == "setlang") {
      lfw.set_lang(lfw::Value(key_of(t[i++])));
    } else if (op == "setlangbad") {
      lfw.set_lang(parse_value(t, i));
    } else if (op == "canon") {
      push("canon|" + s_of(lfw.canonical_lang(key_of(t[i++]))));
    } else if (op == "i18nadd") {
      const std::u16string lang = key_of(t[i++]);
      const std::u16string key = key_of(t[i++]);
      const std::u16string val = key_of(t[i++]);
      lfw::Object words;
      words.set(key, lfw::Value(val));
      lfw::Object langs;
      langs.set(lang, lfw::Value(std::make_shared<lfw::Object>(words)));
      lfw.i18n().add(lfw::Value(std::make_shared<lfw::Object>(langs)));
      push("i18nadd|" + to_ascii(lang) + "|" + to_ascii(key));
    } else if (op == "str") {
      const std::u16string n = key_of(t[i++]);
      push("str|" + to_ascii(n) + "|" + to_ascii(render_value(lfw.string(lfw::Value(n)))));
    } else if (op == "srank") {
      lfw.srank_mode = true;
      lfw.srank_available = true;
      lfw.survival_rank_2p = true;
      lfw.survival_rank_period = u"week";
      lfw::Object data;
      data.set(u"period", lfw::Value(std::u16string(u"week")));
      data.set(u"list", lfw::Value(std::make_shared<lfw::Array>()));
      data.set(u"mine", lfw::Value(lfw::NullTag{}));
      lfw.set_survival_rank_data(lfw::Value(std::make_shared<lfw::Object>(data)));
      push(std::string("srank|") + (lfw.survival_rank_mode() ? "1" : "0") +
           (lfw.survival_rank_available() ? "1" : "0") + (lfw.survival_rank_2p ? "1" : "0") + "|" +
           to_ascii(lfw.survival_rank_period) + "|cheated=" + (lfw.survival_rank_cheated() ? "1" : "0") +
           "|modded=" + (lfw.survival_rank_modded() ? "1" : "0") + "|invalid=" +
           (lfw.survival_rank_invalid() ? "1" : "0"));
    } else if (op == "randinfo") {
      lfw::Entity e(g_ent_host, lfw::Value(std::make_shared<lfw::Object>([] {
                      lfw::Object o;
                      o.set(u"type", lfw::Value(8.0));
                      o.set(u"id", lfw::Value(std::u16string(u"rx")));
                      return o;
                    }())));
      lfw.random_entity_info(e);
      push("randinfo|" + to_ascii(e.id) + "|" + num(e.facing) + "|" + num(e.position.x) + "|" +
           num(e.position.y) + "|" + num(e.position.z) + "|L" + num(lfw.world().left()) + "," +
           num(lfw.world().right()) + "," + num(lfw.world().near_plane()) + "," +
           num(lfw.world().far_plane()));
    } else if (op == "mtrange") {
      const double a = to_double(t[i++]);
      const double b = to_double(t[i++]);
      push("mtrange|" + num(lfw.mt_ref().range(a, b)));
    } else if (op == "entadd") {
      const std::u16string id = key_of(t[i++]);
      const double n = to_double(t[i++]);
      lfw::Object o;
      o.set(u"type", lfw::Value(8.0));
      o.set(u"id", lfw::Value(id));
      const std::vector<lfw::Entity*> ret =
          lfw.entities_helper().add(lfw::Value(std::make_shared<lfw::Object>(o)), n);
      push("entadd|" + to_ascii(lfw::number_to_string(static_cast<double>(ret.size()))));
    } else if (op == "getter") {
      const std::u16string w = key_of(t[i++]);
      push("getter|" + to_ascii(w) + "|" +
           (lfw::loader::get_val_getter_from_stage(w) != nullptr ? "1" : "0"));
    } else if (op == "endtest") {
      std::vector<std::u16string> words;
      while (i < t.size()) words.push_back(key_of(t[i++]));
      auto arr = std::make_shared<lfw::Array>();
      for (const std::u16string& w : words) arr->push_back(lfw::Value(w));
      lfw::Object owner;
      owner.set(u"end_test", lfw::Value(std::move(arr)));
      const lfw::stage::Expressions<lfw::stage::Stage>::Items items =
          lfw.end_testers(lfw::Value(std::make_shared<lfw::Object>(owner)));
      std::string wj;
      for (size_t k = 0; k < words.size(); ++k) {
        if (k != 0) wj += ",";
        wj += to_ascii(words[k]);
      }
      push("endtest|" + wj + "|" +
           to_ascii(lfw::number_to_string(static_cast<double>(items.size()))));
    } else if (op == "keys2") {
      push(std::string("keys2|") + (lfw.keys() == lfw.keys() ? "1" : "0"));
    } else if (op == "keysgo") {
      lfw::Keys* const k = lfw.create_keys();
      const std::string before = to_ascii(lfw::number_to_string(
          static_cast<double>(lfw.mounted_keys.size())));
      lfw.regist_keys(*k);
      lfw.regist_keys(*k);
      lfw.recycle_keys(*k);
      lfw.regist_keys(*k);
      push("keysgo|" + before + "|" +
           to_ascii(lfw::number_to_string(static_cast<double>(lfw.mounted_keys.size()))));
    } else if (op == "instcount") {
      push("instcount|" + to_ascii(lfw::number_to_string(
                              static_cast<double>(lfw::LFW::instances().size()))));
    } else if (op == "dispose") {
      lfw.dispose();
      push("dispose|" + to_ascii(lfw::number_to_string(
                            static_cast<double>(lfw::LFW::instances().size()))));
    } else {
      std::fprintf(stderr, "unknown op '%s'\n", op.c_str());
      return 2;
    }
  }

  std::string joined;
  for (size_t k = 0; k < g_log.size(); ++k) {
    if (k != 0) joined += "\n";
    joined += g_log[k];
  }
  std::printf("%s\n", joined.c_str());
  return 0;
}
