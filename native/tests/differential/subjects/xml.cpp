// `lfw/ditto/xml`（IXMLElement / IXML 缝的 tool 实现）的 C++ 侧台面，
// op 与 `subjects/xml.ts` 一一对应。用例：`cases/xml/*.txt`。
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/tool_xml.h"
#include "lfw/ditto/xml/tool_xml_element.h"

#include "trace_util.h"

namespace {

using trace::esc;
using trace::key_of;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::strip_comment;
using trace::to_ascii;
using trace::to_double;
using trace::to_u16;
using trace::vtag;

std::vector<std::string> g_log;

void push(const std::string& line) { g_log.push_back(line); }

lfw::ToolXML g_xml;
// 根（工厂造出来、台面自己持有）用 `owner` 掌所有权；`bytag` / `bytagi` 出来的别名
// 是父元素持有的裸指针（读用；`ins` 只受理根）。
struct Entry {
  std::shared_ptr<lfw::IXMLElement> owner;
  lfw::IXMLElement* ptr = nullptr;
};

std::map<std::string, Entry> g_els;

lfw::IXMLElement& el(const std::string& id) { return *g_els.at(id).ptr; }

lfw::ToolXMLElement& tel(const std::string& id) {
  return *static_cast<lfw::ToolXMLElement*>(g_els.at(id).ptr);
}

void put_root(const std::string& id, const std::shared_ptr<lfw::IXMLElement>& e) {
  g_els[id] = Entry{e, e.get()};
}

std::string render_opt_str(const std::optional<std::u16string>& v) {
  return v ? esc(*v) : "u";
}

std::string render_opt_num(const std::optional<double>& v) {
  return v ? to_ascii(render_value(lfw::Value(*v))) : "u";
}

std::string render_opt_bool(const std::optional<bool>& v) {
  return v ? to_ascii(render_value(lfw::Value(*v))) : "u";
}

lfw::Value strs_to_value(const std::vector<std::u16string>& v) {
  auto arr = std::make_shared<lfw::Array>();
  for (const std::u16string& s : v) arr->push_back(lfw::Value(s));
  return lfw::Value(arr);
}

lfw::Value strs_soft_to_value(const std::vector<std::optional<std::u16string>>& v) {
  auto arr = std::make_shared<lfw::Array>();
  for (const std::optional<std::u16string>& s : v) {
    if (s) arr->push_back(lfw::Value(*s));
    else arr->push_back(lfw::Value());
  }
  return lfw::Value(arr);
}

lfw::Value nums_to_value(const std::vector<double>& v) {
  auto arr = std::make_shared<lfw::Array>();
  for (double d : v) arr->push_back(lfw::Value(d));
  return lfw::Value(arr);
}

lfw::Value nums_soft_to_value(const std::vector<std::optional<double>>& v) {
  auto arr = std::make_shared<lfw::Array>();
  for (const std::optional<double>& d : v) {
    if (d) arr->push_back(lfw::Value(*d));
    else arr->push_back(lfw::Value());
  }
  return lfw::Value(arr);
}

std::string render_child_tags(const std::vector<lfw::IXMLElement*>& kids) {
  std::string out;
  for (size_t i = 0; i < kids.size(); ++i) {
    if (i != 0) out += ",";
    out += esc(kids[i]->tag());
  }
  return out;
}

void dump_node(lfw::IXMLElement& node, size_t depth) {
  std::string attrs;
  for (size_t i = 0; i < node.attrs().size(); ++i) {
    if (i != 0) attrs += ",";
    attrs += esc(node.attrs()[i].name);
    attrs += "=";
    attrs += esc(node.attrs()[i].value);
  }
  push("d|" + std::to_string(depth) + "|" + esc(node.tag()) + "|" +
       std::to_string(node.attrs().size()) + "|" + attrs + "|" + esc(node.text()));
  for (lfw::IXMLElement* child : node.children()) dump_node(*child, depth + 1);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_xml <case-file>\n");
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
    const std::vector<std::string> t = split_ws(strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    if (op == "new") {
      put_root(t[i], g_xml.create(key_of(t[i + 1])));
      i += 2;
    } else if (op == "fromstr") {
      const std::string eid = t[i++];
      const std::u16string tag = key_of(t[i++]);
      put_root(eid, g_xml.from_string(key_of(t[i++]), tag));
    } else if (op == "fromnum") {
      const std::string eid = t[i++];
      const std::u16string tag = key_of(t[i++]);
      put_root(eid, g_xml.from_number(to_double(t[i++]), tag));
    } else if (op == "frombool") {
      const std::string eid = t[i++];
      const std::u16string tag = key_of(t[i++]);
      put_root(eid, g_xml.from_boolean(t[i++] == "1", tag));
    } else if (op == "fromarr") {
      const std::string eid = t[i++];
      const std::u16string tag = key_of(t[i++]);
      const lfw::Value v = parse_value(t, i);
      if (vtag(v).rfind("a", 0) != 0) {
        std::fprintf(stderr, "fromarr expects an array literal at line %d\n", lineno);
        return 2;
      }
      put_root(eid, g_xml.from_array(v, tag));
    } else if (op == "fromobj") {
      const std::string eid = t[i++];
      const std::u16string tag = key_of(t[i++]);
      const lfw::Value v = parse_value(t, i);
      if (vtag(v).rfind("o", 0) != 0) {
        std::fprintf(stderr, "fromobj expects an object literal at line %d\n", lineno);
        return 2;
      }
      put_root(eid, g_xml.from_object(v, tag));
    } else if (op == "bytag") {
      lfw::IXMLElement& parent = el(t[i++]);
      const std::u16string name = key_of(t[i++]);
      const std::string alias = t[i++];
      lfw::IXMLElement* child = parent.child_by_tag(name);
      if (child == nullptr) {
        std::fprintf(stderr, "bytag: no child '%s' at line %d\n", to_ascii(name).c_str(), lineno);
        return 2;
      }
      g_els[alias] = Entry{nullptr, child};
    } else if (op == "bytagi") {
      lfw::IXMLElement& parent = el(t[i++]);
      const std::u16string name = key_of(t[i++]);
      const size_t idx = static_cast<size_t>(trace::to_long(t[i++]));
      const std::string alias = t[i++];
      const std::vector<lfw::IXMLElement*> kids = parent.children_by_tag(name);
      if (idx >= kids.size()) {
        std::fprintf(stderr, "bytagi: index %zu out of range at line %d\n", idx, lineno);
        return 2;
      }
      g_els[alias] = Entry{nullptr, kids[idx]};
    } else if (op == "text") {
      tel(t[i++]).set_text(key_of(t[i++]));
    } else if (op == "attr") {
      lfw::ToolXMLElement& e = tel(t[i++]);
      const std::u16string name = key_of(t[i++]);
      e.set_attr(name, parse_value(t, i));
    } else if (op == "dattr") {
      tel(t[i++]).del_attr(key_of(t[i++]));
    } else if (op == "sattr") {
      lfw::ToolXMLElement& e = tel(t[i++]);
      const std::u16string name = key_of(t[i++]);
      e.set_arr_attr_soft(name, parse_value(t, i));
    } else if (op == "ins") {
      lfw::ToolXMLElement& parent = tel(t[i++]);
      const std::string cid = t[i++];
      const std::string idx = t[i++];
      if (idx == "-") {
        parent.insert(g_els.at(cid).owner, std::nullopt);
      } else {
        parent.insert(g_els.at(cid).owner, static_cast<size_t>(trace::to_long(idx)));
      }
    } else if (op == "rm") {
      lfw::ToolXMLElement& parent = tel(t[i++]);
      const bool ok = parent.remove(g_els.at(t[i++]).ptr);
      push(std::string("rm:") + (ok ? "true" : "false"));
    } else if (op == "rmself") {
      const bool ok = el(t[i++]).remove_self();
      push(std::string("rmself:") + (ok ? "true" : "false"));
    } else if (op == "rmall") {
      tel(t[i++]).remove_all();
    } else if (op == "dump") {
      dump_node(el(t[i++]), 0);
    } else if (op == "rd") {
      lfw::ToolXMLElement& e = tel(t[i++]);
      const std::string what = t[i++];
      std::string out = "rd|" + what + "|";
      if (what == "tag") {
        out += esc(e.tag());
      } else if (what == "text") {
        out += esc(e.text());
      } else if (what == "parent") {
        out += e.parent() == nullptr ? "u" : esc(e.parent()->tag());
      } else if (what == "children") {
        out += "n=" + std::to_string(e.children().size()) + "|" + render_child_tags(e.children());
      } else if (what == "attrs") {
        std::string attrs;
        for (size_t j = 0; j < e.attrs().size(); ++j) {
          if (j != 0) attrs += ",";
          attrs += esc(e.attrs()[j].name) + "=" + esc(e.attrs()[j].value);
        }
        out += "n=" + std::to_string(e.attrs().size()) + "|" + attrs;
      } else if (what == "type") {
        out += render_opt_str(e.type());
      } else if (what == "action") {
        out += esc(e.action_str());
      } else if (what == "strop") {
        out += esc(e.stringify());
      } else if (what == "hasattr") {
        out += e.has_attr(key_of(t[i++])) ? "true" : "false";
      } else if (what == "attr" || what == "str_attr") {
        out += render_opt_str(e.attr(key_of(t[i++])));
      } else if (what == "num_attr") {
        out += render_opt_num(e.num_attr(key_of(t[i++])));
      } else if (what == "bool_attr") {
        out += render_opt_bool(e.bool_attr(key_of(t[i++])));
      } else if (what == "strs") {
        const std::optional<std::vector<std::u16string>> v = e.strs_attr(key_of(t[i++]));
        out += v ? to_ascii(render_value(strs_to_value(*v))) : "u";
      } else if (what == "nums") {
        const std::optional<std::vector<double>> v = e.nums_attr(key_of(t[i++]));
        out += v ? to_ascii(render_value(nums_to_value(*v))) : "u";
      } else if (what == "strssoft") {
        const std::optional<std::vector<std::optional<std::u16string>>> v =
            e.strs_attr_soft(key_of(t[i++]));
        out += v ? to_ascii(render_value(strs_soft_to_value(*v))) : "u";
      } else if (what == "numssoft") {
        const std::optional<std::vector<std::optional<double>>> v =
            e.nums_attr_soft(key_of(t[i++]));
        out += v ? to_ascii(render_value(nums_soft_to_value(*v))) : "u";
      } else if (what == "asstr") {
        out += render_opt_str(e.as_string());
      } else if (what == "asnum") {
        out += render_opt_num(e.as_number());
      } else if (what == "asbool") {
        out += render_opt_bool(e.as_boolean());
      } else if (what == "asval") {
        out += to_ascii(render_value(e.as_value()));
      } else if (what == "asarr") {
        out += to_ascii(render_value(e.as_array()));
      } else if (what == "asobj") {
        out += to_ascii(render_value(e.as_object()));
      } else if (what == "asobj_or") {
        out += to_ascii(render_value(e.as_object(parse_value(t, i))));
      } else if (what == "asarr_or") {
        out += to_ascii(render_value(e.as_array(parse_value(t, i))));
      } else if (what == "asstr_or") {
        out += esc(e.as_string(key_of(t[i++])));
      } else if (what == "asnum_or") {
        out += to_ascii(render_value(lfw::Value(e.as_number(to_double(t[i++])))));
      } else if (what == "asbool_or") {
        out += to_ascii(render_value(lfw::Value(e.as_boolean(t[i++] == "1"))));
      } else if (what == "getstr") {
        out += render_opt_str(e.get_str(key_of(t[i++])));
      } else if (what == "getnum") {
        out += render_opt_num(e.get_num(key_of(t[i++])));
      } else if (what == "getbool") {
        out += render_opt_bool(e.get_bool(key_of(t[i++])));
      } else if (what == "getstr_or") {
        const std::u16string name = key_of(t[i++]);
        out += esc(e.get_str(name, key_of(t[i++])));
      } else if (what == "getnum_or") {
        const std::u16string name = key_of(t[i++]);
        out += to_ascii(render_value(lfw::Value(e.get_num(name, to_double(t[i++])))));
      } else if (what == "getbool_or") {
        const std::u16string name = key_of(t[i++]);
        out += to_ascii(render_value(lfw::Value(e.get_bool(name, t[i++] == "1"))));
      } else if (what == "gstrarr") {
        const std::optional<std::vector<std::u16string>> v = e.get_str_arr(key_of(t[i++]));
        out += v ? to_ascii(render_value(strs_to_value(*v))) : "u";
      } else if (what == "gnumarr") {
        const std::optional<std::vector<double>> v = e.get_num_arr(key_of(t[i++]));
        out += v ? to_ascii(render_value(nums_to_value(*v))) : "u";
      } else if (what == "getobj") {
        out += to_ascii(render_value(e.get_obj(key_of(t[i++]))));
      } else if (what == "cbt") {
        lfw::IXMLElement* c = e.child_by_tag(key_of(t[i++]));
        out += c == nullptr ? "u" : esc(c->tag());
      } else if (what == "cbtall") {
        const std::vector<lfw::IXMLElement*> kids = e.children_by_tag(key_of(t[i++]));
        out += "n=" + std::to_string(kids.size()) + "|" + render_child_tags(kids);
      } else {
        std::fprintf(stderr, "unknown rd kind '%s' at line %d\n", what.c_str(), lineno);
        return 2;
      }
      push(out);
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
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
