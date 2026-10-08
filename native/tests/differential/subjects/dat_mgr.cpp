// `loader/DatMgr` 的 C++ 侧台面，op 与 `subjects/dat_mgr.ts` 一一对应（说明见该文件头部）。
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "lfw/bot/bot_controller.h"
#include "lfw/controller/ball_controller.h"
#include "lfw/controller/creators.h"
#include "lfw/core/value.h"
#include "lfw/ditto/xml/tool_xml.h"
#include "lfw/factory.h"
#include "lfw/loader/dat_mgr.h"
#include "lfw/resources.h"
#include "lfw/utils/math/mersenne_twister.h"
#include "lfw/zip_mgr.h"

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

std::map<std::string, lfw::Value> g_jfiles;
std::map<std::string, std::string> g_tfiles;
std::map<std::string, std::shared_ptr<lfw::IXMLElement>> g_trees;
std::map<std::string, std::string> g_jfails;
std::map<std::string, std::string> g_tfails;
std::set<std::string> g_clear_at;
std::string g_clear_img;

lfw::ToolXML g_xml;
lfw::MersenneTwister g_mt(12345);
lfw::ZipMgr g_zip_mgr;
std::unique_ptr<lfw::Resources> g_resources;
lfw::Factory g_factory;
lfw::loader::DatMgr* g_mgr = nullptr;

bool value_is_u(const lfw::Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<lfw::NullTag>(v);
}

std::string join_ascii(const std::vector<std::u16string>& urls) {
  std::string joined;
  for (size_t i = 0; i < urls.size(); ++i) {
    if (i != 0) joined += ",";
    joined += to_ascii(urls[i]);
  }
  return joined;
}

// 真导入（`file.json()` / Importer）每次都给**新对象** ⇒ 台面也克隆一份（对应 TS 侧
// 的 `JSON.parse(JSON.stringify(v))`；不克隆的话第二次 cook 会撞上 TS 的 `data.xml = …`
// 只读 getter，那是「复用同一实例」才有的假象）。
lfw::Value clone_value(const lfw::Value& v) {
  if (const lfw::Array* const a = lfw::as_array(v)) {
    auto out = std::make_shared<lfw::Array>();
    for (size_t i = 0; i < a->size(); ++i) out->push_back(clone_value(a->at(i)));
    return lfw::Value(std::move(out));
  }
  if (const lfw::Object* const o = lfw::as_object(v)) {
    auto out = std::make_shared<lfw::Object>();
    for (const std::u16string& k : o->keys()) {
      const lfw::Value* const p = o->get(k);
      if (p != nullptr) out->set(k, clone_value(*p));
    }
    return lfw::Value(std::move(out));
  }
  return v;
}

// `xtree` 的树字面量：`o 3 tag s "entity" attrs o <n> … kids a <n> …`（递归）。
std::shared_ptr<lfw::IXMLElement> build_el(const lfw::Value& v) {
  std::shared_ptr<lfw::IXMLElement> el =
      g_xml.create(lfw::to_string(lfw::field_or(v, u"tag")));
  const lfw::Object* const attrs = lfw::as_object(lfw::field_or(v, u"attrs"));
  if (attrs != nullptr) {
    for (const std::u16string& k : attrs->keys()) {
      const lfw::Value* const val = attrs->get(k);
      if (val != nullptr) el->set_attr(k, *val);
    }
  }
  const lfw::Array* const kids = lfw::as_array(lfw::field_or(v, u"kids"));
  if (kids != nullptr) {
    for (size_t i = 0; i < kids->size(); ++i) el->insert(build_el(kids->at(i)));
  }
  return el;
}

std::string render_args(const std::string& prefix, const std::vector<lfw::Value>& args) {
  if (args.empty()) return prefix + ":";
  if (args.size() > 1) {
    return prefix + ":" + to_ascii(render_value(args[0])) + ":" +
           to_ascii(render_value(args[1]));
  }
  return prefix + ":" + to_ascii(render_value(args[0]));
}

// `I.Ditto.Importer` + `I.Ditto.XML` 的假件（资源链走真 `Resources` + 空 `ZipMgr`）。
class FakeHost : public lfw::IResourcesHost {
 public:
  bool import_as_json(const std::vector<std::u16string>& urls, lfw::Value& data, lfw::Value& hit,
                      std::u16string& error) override {
    return do_import(g_jfiles, g_jfails, "json", urls, data, hit, error);
  }
  bool import_as_text(const std::vector<std::u16string>& urls, lfw::Value& data, lfw::Value& hit,
                      std::u16string& error) override {
    std::string joined;
    push("imp:text|" + join_ascii(urls));
    const std::string p = urls.empty() ? std::string() : to_ascii(urls[0]);
    const auto f = g_tfails.find(p);
    if (f != g_tfails.end()) {
      error = to_u16(f->second);
      return false;
    }
    if (g_clear_at.count("text:" + p)) g_mgr->clear();
    const auto v = g_tfiles.find(p);
    if (v == g_tfiles.end()) {
      std::fprintf(stderr, "unscripted import_as_text '%s'\n", p.c_str());
      std::exit(2);
    }
    data = lfw::Value(to_u16(v->second));
    hit = lfw::Value();
    return true;
  }
  bool import_as_blob_url(const std::vector<std::u16string>&, lfw::Value&, lfw::Value&,
                          std::u16string&) override {
    std::fprintf(stderr, "unscripted import_as_blob_url\n");
    std::exit(2);
  }
  bool import_as_array_buffer(const std::vector<std::u16string>&, lfw::Value&, lfw::Value&,
                              std::u16string&) override {
    std::fprintf(stderr, "unscripted import_as_array_buffer\n");
    std::exit(2);
  }
  bool import_as_image_bitmap(const std::vector<std::u16string>&, lfw::Value&, lfw::Value&,
                              std::u16string&) override {
    std::fprintf(stderr, "unscripted import_as_image_bitmap\n");
    std::exit(2);
  }
  bool xml_parse(const lfw::Value& text, lfw::Value& marker,
                 std::shared_ptr<lfw::IXMLElement>& root, std::u16string&) override {
    const std::string key = to_ascii(lfw::to_string(text));
    push("xml:" + key);
    const auto it = g_trees.find(key);
    if (it == g_trees.end()) {
      std::fprintf(stderr, "unscripted xml_parse '%s'\n", key.c_str());
      std::exit(2);
    }
    root = it->second;
    marker = lfw::Value(true);
    return true;
  }

 private:
  bool do_import(const std::map<std::string, lfw::Value>& files,
                 const std::map<std::string, std::string>& fails, const char* kind,
                 const std::vector<std::u16string>& urls, lfw::Value& data, lfw::Value& hit,
                 std::u16string& error) {
    push(std::string("imp:") + kind + "|" + join_ascii(urls));
    const std::string p = urls.empty() ? std::string() : to_ascii(urls[0]);
    const auto f = fails.find(p);
    if (f != fails.end()) {
      error = to_u16(f->second);
      return false;
    }
    if (g_clear_at.count(std::string(kind) + ":" + p)) g_mgr->clear();
    const auto v = files.find(p);
    if (v == files.end()) {
      std::fprintf(stderr, "unscripted import_as_%s '%s'\n", kind, p.c_str());
      std::exit(2);
    }
    data = clone_value(v->second);
    hit = lfw::Value();
    return true;
  }
};

FakeHost g_host;

class FakeMgrHost : public lfw::loader::IDatMgrHost {
 public:
  lfw::Resources& resources() override { return *g_resources; }
  lfw::MersenneTwister& mt_ref() override { return g_mt; }
  void load_img(const std::u16string& path) override {
    const std::string p = to_ascii(path);
    push("img:" + p);
    if (g_clear_img == p) g_mgr->clear();
  }
  void emit_progress(const std::u16string& content, double progress) override {
    push("prog:" + to_ascii(content) + "|" + to_ascii(render_value(lfw::Value(progress))));
  }
  void warn(const std::vector<lfw::Value>& args) override { push(render_args("warn", args)); }
  void error(const std::vector<lfw::Value>& args) override { push(render_args("error", args)); }
};

FakeMgrHost g_mgr_host;

std::string id_of(const lfw::Value& v) { return to_ascii(lfw::to_string(lfw::field_or(v, u"id"))); }

std::string ids_of(const std::vector<lfw::Value>& list) {
  std::string out;
  for (size_t i = 0; i < list.size(); ++i) {
    if (i != 0) out += ",";
    out += id_of(list[i]);
  }
  return out;
}

std::string hit_id(const lfw::Value* v) { return v == nullptr ? "u" : id_of(*v); }

std::string hit_id_value(const lfw::Value& v) { return value_is_u(v) ? "u" : id_of(v); }

std::vector<std::u16string> split_comma(const std::u16string& s) {
  std::vector<std::u16string> out;
  std::u16string cur;
  for (char16_t c : s) {
    if (c == u',') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

void dump() {
  push("dump|inner=" + std::to_string(g_mgr->inner_id()) + "|bots=" + ids_of(g_mgr->bots()) +
       "|moves=" + ids_of(g_mgr->moves()) + "|objects=" + ids_of(g_mgr->objects()) +
       "|fighters=" + ids_of(g_mgr->fighters()) + "|weapons=" + ids_of(g_mgr->weapons()) +
       "|balls=" + ids_of(g_mgr->balls()) + "|entities=" + ids_of(g_mgr->entities()) +
       "|bgs=" + ids_of(g_mgr->backgrounds()) + "|stages=" + ids_of(g_mgr->stages()));
}

void do_load(const std::vector<std::u16string>& paths) {
  std::u16string error;
  if (g_mgr->load(paths, error)) {
    push("load:ok");
  } else {
    push("load:fail:" + to_ascii(error));
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_dat_mgr <case-file>\n");
    return 2;
  }
  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  g_resources = std::make_unique<lfw::Resources>(&g_zip_mgr, &g_host);
  lfw::Factory::set_warn([](const std::u16string& text) {
    push("warn:" + to_ascii(render_value(lfw::Value(text))));
  });
  lfw::loader::DatMgr mgr(&g_mgr_host);
  g_mgr = &mgr;

  std::string raw;
  while (std::getline(in, raw)) {
    const std::vector<std::string> t = split_ws(strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    if (op == "jfile") {
      const std::string p = to_ascii(key_of(t[i++]));
      g_jfiles[p] = parse_value(t, i);
    } else if (op == "xtree") {
      const std::string p = to_ascii(key_of(t[i++]));
      g_tfiles[p] = p;
      g_trees[p] = build_el(parse_value(t, i));
    } else if (op == "jfail") {
      const std::string p = to_ascii(key_of(t[i++]));
      g_jfails[p] = to_ascii(key_of(t[i++]));
    } else if (op == "tfail") {
      const std::string p = to_ascii(key_of(t[i++]));
      g_tfails[p] = to_ascii(key_of(t[i++]));
    } else if (op == "clearat") {
      const std::string m = t[i++];
      g_clear_at.insert(m + ":" + to_ascii(key_of(t[i++])));
    } else if (op == "clearimg") {
      const std::u16string p = key_of(t[i++]);
      g_clear_img = p == u"-" ? std::string() : to_ascii(p);
    } else if (op == "unhook") {
      g_clear_at.clear();
      g_clear_img.clear();
    } else if (op == "spark") {
      std::vector<lfw::Value> empty;
      lfw::Object base;
      base.set(u"name", lfw::Value(u"Spark"));
      lfw::Object obj;
      obj.set(u"id", lfw::Value(u"spark"));
      obj.set(u"type", lfw::Value(4.0));
      obj.set(u"base", lfw::Value(std::make_shared<lfw::Object>(base)));
      g_jfiles["data/spark.obj.json5"] = lfw::Value(std::make_shared<lfw::Object>(obj));
      (void)empty;
    } else if (op == "load") {
      std::vector<std::u16string> paths;
      while (i < t.size()) paths.push_back(key_of(t[i++]));
      do_load(paths);
    } else if (op == "dump") {
      dump();
    } else if (op == "find") {
      push("find:" + hit_id(g_mgr->find(lfw::Value(key_of(t[i++])))));
    } else if (op == "botof") {
      const lfw::Value* const d = g_mgr->find(lfw::Value(key_of(t[i++])));
      const lfw::Value bot =
          d == nullptr ? lfw::Value() : lfw::field_or(lfw::field_or(*d, u"base"), u"bot");
      push("botof:" + hit_id_value(bot));
    } else if (op == "findbot") {
      push("findbot:" + hit_id(g_mgr->find_bot(lfw::Value(key_of(t[i++])))));
    } else if (op == "findmoves") {
      push("findmoves:" + hit_id(g_mgr->find_moves(lfw::Value(key_of(t[i++])))));
    } else if (op == "fwv") {
      push("fwv:" + hit_id(g_mgr->find_weapon(lfw::Value(key_of(t[i++])))));
    } else if (op == "fwpred") {
      const double n = to_double(t[i++]);
      const lfw::loader::DatMgr::FindPredicate pred =
          [n](const lfw::Value&, double idx, const std::vector<lfw::Value>&) { return idx == n; };
      push("fwpred:" + hit_id(g_mgr->find_weapon(pred)));
    } else if (op == "fobjv") {
      push("fobjv:" + hit_id(g_mgr->find_object(lfw::Value(key_of(t[i++])))));
    } else if (op == "fentv") {
      push("fentv:" + hit_id(g_mgr->find_entity(lfw::Value(key_of(t[i++])))));
    } else if (op == "ffv") {
      push("ffv:" + hit_id(g_mgr->find_fighter(lfw::Value(key_of(t[i++])))));
    } else if (op == "fbgv") {
      push("fbgv:" + hit_id(g_mgr->find_background(lfw::Value(key_of(t[i++])))));
    } else if (op == "bgh") {
      const lfw::Value* const d = g_mgr->find_background(lfw::Value(key_of(t[i++])));
      const lfw::Value h =
          d == nullptr ? lfw::Value() : lfw::field_or(lfw::field_or(*d, u"base"), u"height");
      push("bgh:" + (d == nullptr ? std::string("u") : to_ascii(lfw::to_string(h))));
    } else if (op == "stg") {
      const std::u16string id = key_of(t[i++]);
      std::string out = "u";
      for (const lfw::Value& s : g_mgr->stages()) {
        if (lfw::strict_equals(lfw::field_or(s, u"id"), lfw::Value(id))) {
          out = to_ascii(lfw::to_string(lfw::field_or(s, u"name")));
          break;
        }
      }
      push("stg:" + out);
    } else if (op == "stgz") {
      const std::u16string id = key_of(t[i++]);
      std::string out = "u";
      for (const lfw::Value& s : g_mgr->stages()) {
        if (lfw::strict_equals(lfw::field_or(s, u"id"), lfw::Value(id))) {
          out = to_ascii(render_value(s));
          break;
        }
      }
      push("stgz:" + out);
    } else if (op == "botst") {
      const lfw::Value* const b = g_mgr->find_bot(lfw::Value(key_of(t[i++])));
      double n = 0;
      if (b != nullptr) {
        const lfw::Object* const st = lfw::as_object(lfw::field_or(*b, u"states"));
        if (st != nullptr) n = static_cast<double>(st->size());
      }
      push("botst:" + (b == nullptr ? std::string("u") : to_ascii(lfw::to_string(lfw::Value(n)))));
    } else if (op == "objg") {
      push("objg:" + ids_of(g_mgr->get_objects_of_group(key_of(t[i++]))));
    } else if (op == "fg") {
      push("fg:" + ids_of(g_mgr->get_fighters_of_group(key_of(t[i++]))));
    } else if (op == "wg") {
      push("wg:" + ids_of(g_mgr->get_weapons_of_group(key_of(t[i++]))));
    } else if (op == "fng") {
      push("fng:" + ids_of(g_mgr->get_fighters_not_in_group(key_of(t[i++]))));
    } else if (op == "bgg") {
      push("bgg:" + ids_of(g_mgr->get_backgrouds_of_group(key_of(t[i++]))));
    } else if (op == "randg") {
      const lfw::Randoming::Ptr r = g_mgr->get_randoming_by_group(key_of(t[i++]));
      const lfw::Value got = r->get();
      push("randg|" + to_ascii(r->name()) + "|" + ids_of(r->src()) + "|" + hit_id_value(got));
    } else if (op == "randgc") {
      const std::u16string key = key_of(t[i++]);
      const lfw::Randoming::Ptr r1 = g_mgr->get_randoming_by_group(key);
      const lfw::Randoming::Ptr r2 = g_mgr->get_randoming_by_group(key);
      const lfw::Value got = r2->get();
      push("randgc:" + std::string(r1 == r2 ? "1" : "0") + "|" + hit_id_value(got));
    } else if (op == "bgr") {
      const std::vector<std::u16string> groups = split_comma(key_of(t[i++]));
      const lfw::Randoming::Ptr r = g_mgr->get_bg_randoming_of_group(groups);
      const lfw::Value got = r->get();
      push("bgr|" + to_ascii(r->name()) + "|" + ids_of(r->src()) + "|" + hit_id_value(got));
    } else if (op == "rbg") {
      const std::vector<std::u16string> groups = split_comma(key_of(t[i++]));
      push("rbg:" + hit_id_value(g_mgr->get_random_bg(groups)));
    } else if (op == "ctrls") {
      std::string out;
      for (const std::pair<lfw::FactoryKey, const lfw::ICtrlCreator*>& kv :
           lfw::Factory::ctrl_creators()) {
        if (!out.empty()) out += ",";
        out += to_ascii(render_value(kv.first));
      }
      push("ctrls:" + out);
    } else if (op == "mkctrl") {
      const std::u16string oid = key_of(t[i++]);
      lfw::controller::BaseController* const c =
          g_factory.create_ctrl(lfw::Value(oid), u"p1", nullptr);
      if (c == nullptr) {
        push("mkctrl:u");
      } else {
        // 构建关了 RTTI（`/GR-`，见 CMakePresets）⇒ 用 creator 身份当标签。
        const char* label = c->creator() == lfw::controller::ball_controller_creator() ? "ball"
                            : c->creator() == lfw::controller::bot_controller_creator() ? "bot"
                                                                                             : "?";
        push(std::string("mkctrl:") + label + ":" + to_ascii(c->player_id));
      }
    } else if (op == "clear") {
      g_mgr->clear();
      push("clear:" + std::to_string(g_mgr->inner_id()));
    } else if (op == "dispose") {
      g_mgr->dispose();
      push("dispose:" + std::to_string(g_mgr->inner_id()));
    } else if (op == "innerid") {
      push("innerid:" + std::to_string(g_mgr->inner_id()));
    } else {
      std::fprintf(stderr, "unknown op '%s'\n", op.c_str());
      return 2;
    }
  }

  for (const std::string& line : g_log) std::printf("%s\n", line.c_str());
  return 0;
}
