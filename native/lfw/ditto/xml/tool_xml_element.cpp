#include "lfw/ditto/xml/tool_xml_element.h"

#include <cmath>
#include <utility>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"

namespace lfw {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// JS `String.prototype.trim`：两端 `is_str_white_space`。
std::u16string trim(const std::u16string& s) {
  size_t b = 0;
  size_t e = s.size();
  while (b < e && is_str_white_space(s[b])) ++b;
  while (e > b && is_str_white_space(s[e - 1])) --e;
  return s.substr(b, e - b);
}

// JS `String.prototype.split(sep)`（sep 是单个码元；LFW 的调用点都走默认的 `,`）。
std::vector<std::u16string> split_char(const std::u16string& s, char16_t sep) {
  std::vector<std::u16string> out;
  size_t start = 0;
  for (size_t i = 0; i <= s.size(); ++i) {
    if (i == s.size() || s[i] == sep) {
      out.push_back(s.substr(start, i - start));
      start = i + 1;
    }
  }
  return out;
}

// JS `Array.prototype.join(sep)`：`undefined` / `null` 成员变空串，其余 `String(v)`。
std::u16string join_values(const Array& a, char16_t sep) {
  std::u16string out;
  for (size_t i = 0; i < a.size(); ++i) {
    if (i != 0) out.push_back(sep);
    const Value& item = a.at(i);
    if (is_nullish(item)) continue;
    out += to_string(item);
  }
  return out;
}

// fast-xml-parser 的 XML 转义：`& < > " '` 五个。
void escape_into(const std::u16string& s, std::u16string& out) {
  for (char16_t c : s) {
    switch (c) {
      case u'&': out += u"&amp;"; break;
      case u'<': out += u"&lt;"; break;
      case u'>': out += u"&gt;"; break;
      case u'"': out += u"&quot;"; break;
      case u'\'': out += u"&apos;"; break;
      default: out.push_back(c); break;
    }
  }
}

// fast-xml-parser FXP 对象的等价物（只给 `stringify` 用）。
// `FxpChild::undef` 对应 `_toFXPObject` 里 `{[tag]: undefined}` 的空元素（整块会被丢掉）。
// `FxpGroup::items` 对应 JS `childObj[key]`：1 项=单值（可能 undef），多项=数组。
struct FxpChild {
  bool undef = true;
  std::u16string tag;
  std::vector<XmlAttr> attrs;
  bool has_text = false;
  std::u16string text;
  std::vector<struct FxpGroup> groups;
};

struct FxpGroup {
  std::u16string key;
  std::vector<FxpChild> items;
};

// 对应 `_toFXPObject`：有子元素走子元素分支（文本被丢掉），否则文本非空走文本分支，
// 否则只剩属性；属性也是空的话就是 undef。
FxpChild make_fxp(const IXMLElement& el) {
  FxpChild node;
  node.tag = el.tag();
  node.attrs = el.attrs();
  const std::vector<IXMLElement*>& kids = el.children();
  if (!kids.empty()) {
    node.undef = false;
    for (IXMLElement* child : kids) {
      FxpChild c = make_fxp(*child);
      std::u16string key = c.tag;
      FxpGroup* group = nullptr;
      for (FxpGroup& g : node.groups) {
        if (g.key == key) {
          group = &g;
          break;
        }
      }
      if (group == nullptr) {
        FxpGroup g;
        g.key = key;
        g.items.push_back(std::move(c));
        node.groups.push_back(std::move(g));
      } else if (group->items.size() == 1 && group->items[0].undef) {
        // JS `if (childObj[key])`：单值是 undefined 时是假值 ⇒ 直接覆盖，不转数组。
        group->items[0] = std::move(c);
      } else {
        group->items.push_back(std::move(c));
      }
    }
    return node;
  }
  if (!el.text().empty()) {
    node.undef = false;
    node.has_text = true;
    node.text = el.text();
    return node;
  }
  if (!node.attrs.empty()) node.undef = false;
  return node;
}

// 对应 `XMLBuilder({format:true, suppressEmptyNode:false})`：每层缩进 2 空格、行尾 `\n`；
// 文本与属性内联；键值 undefined 的孩子不出现。
void build_xml(const FxpChild& node, size_t depth, std::u16string& out) {
  std::u16string indent(depth * 2, u' ');
  out += indent;
  out.push_back(u'<');
  out += node.tag;
  for (const XmlAttr& a : node.attrs) {
    out.push_back(u' ');
    out += a.name;
    out += u"=\"";
    escape_into(a.value, out);
    out.push_back(u'"');
  }
  bool any_child = false;
  for (const FxpGroup& g : node.groups) {
    for (const FxpChild& c : g.items) {
      if (!c.undef) any_child = true;
    }
  }
  if (any_child) {
    out += u">\n";
    for (const FxpGroup& g : node.groups) {
      for (const FxpChild& c : g.items) {
        if (!c.undef) build_xml(c, depth + 1, out);
      }
    }
    out += indent;
  } else if (node.has_text) {
    out.push_back(u'>');
    escape_into(node.text, out);
    out += u"</";
    out += node.tag;
    out.push_back(u'>');
    out.push_back(u'\n');
    return;
  } else {
    out += u"></";
    out += node.tag;
    out.push_back(u'>');
    out.push_back(u'\n');
    return;
  }
  out += u"</";
  out += node.tag;
  out.push_back(u'>');
  out.push_back(u'\n');
}

}  // namespace

ToolXMLElement::ToolXMLElement(std::u16string tag) : tag_(std::move(tag)) {}

std::optional<std::u16string> ToolXMLElement::type() const {
  std::u16string t = to_lower_case(tag_);
  if (t == u"value") {
    std::optional<std::u16string> a = attr(u"type");
    if (!a) return std::nullopt;
    return to_lower_case(*a);
  }
  return t;
}

std::optional<std::u16string> ToolXMLElement::attr(const std::u16string& name) const {
  for (const XmlAttr& a : attrs_) {
    if (a.name == name) return a.value;
  }
  return std::nullopt;
}

std::optional<std::u16string> ToolXMLElement::str_attr(const std::u16string& name) const {
  return attr(name);
}

std::optional<double> ToolXMLElement::num_attr(const std::u16string& name) const {
  std::optional<std::u16string> v = attr(name);
  if (!v) return std::nullopt;
  return string_to_number(*v);
}

std::optional<bool> ToolXMLElement::bool_attr(const std::u16string& name) const {
  std::optional<std::u16string> v = attr(name);
  if (!v) return std::nullopt;
  return *v == u"true" || *v == u"1";
}

std::optional<std::vector<std::u16string>> ToolXMLElement::strs_attr(const std::u16string& name,
                                                                     char16_t sep) const {
  std::optional<std::u16string> v = attr(name);
  if (!v) return std::nullopt;
  std::vector<std::u16string> ret;
  for (const std::u16string& part : split_char(*v, sep)) ret.push_back(trim(part));
  return ret;
}

std::optional<std::vector<double>> ToolXMLElement::nums_attr(const std::u16string& name,
                                                             char16_t sep) const {
  std::optional<std::u16string> v = attr(name);
  if (!v) return std::nullopt;
  std::vector<double> ret;
  for (const std::u16string& part : split_char(*v, sep)) ret.push_back(string_to_number(trim(part)));
  return ret;
}

std::optional<std::vector<std::optional<std::u16string>>> ToolXMLElement::strs_attr_soft(
    const std::u16string& name, char16_t sep) const {
  std::optional<std::u16string> v = attr(name);
  if (!v) return std::nullopt;
  std::vector<std::optional<std::u16string>> ret;
  for (const std::u16string& part : split_char(*v, sep)) {
    std::u16string t = trim(part);
    if (t.empty()) ret.push_back(std::nullopt);
    else ret.push_back(t);
  }
  return ret;
}

std::optional<std::vector<std::optional<double>>> ToolXMLElement::nums_attr_soft(
    const std::u16string& name, char16_t sep) const {
  std::optional<std::u16string> v = attr(name);
  if (!v) return std::nullopt;
  std::vector<std::optional<double>> ret;
  for (const std::u16string& part : split_char(*v, sep)) {
    std::u16string t = trim(part);
    if (t.empty()) ret.push_back(std::nullopt);
    else ret.push_back(string_to_number(t));
  }
  return ret;
}

void ToolXMLElement::set_attr(const std::u16string& name, const Value& value, char16_t sep) {
  if (is_nullish(value)) {
    del_attr(name);
    return;
  }
  std::u16string sv;
  if (const Array* arr = lfw::as_array(value)) sv = join_values(*arr, sep);
  else sv = to_string(value);
  for (XmlAttr& a : attrs_) {
    if (a.name == name) {
      a.value = sv;
      return;
    }
  }
  attrs_.push_back(XmlAttr{name, sv});
}

void ToolXMLElement::del_attr(const std::u16string& name) {
  for (size_t i = 0; i < attrs_.size(); ++i) {
    if (attrs_[i].name == name) {
      attrs_.erase(attrs_.begin() + static_cast<std::ptrdiff_t>(i));
      return;
    }
  }
}

void ToolXMLElement::set_arr_attr_soft(const std::u16string& name, const Value& value,
                                       char16_t sep) {
  if (is_nullish(value)) {
    del_attr(name);
    return;
  }
  const Array* arr = lfw::as_array(value);
  if (arr == nullptr) {
    set_attr(name, value, sep);
    return;
  }
  std::vector<Value> items = arr->items();
  while (!items.empty() && is_nullish(items.back())) items.pop_back();
  if (items.empty()) {
    del_attr(name);
    return;
  }
  std::u16string sv;
  for (size_t i = 0; i < items.size(); ++i) {
    if (i != 0) sv.push_back(sep);
    if (!is_nullish(items[i])) sv += to_string(items[i]);
  }
  set_attr(name, Value(std::move(sv)), u',');
}

bool ToolXMLElement::has_attr(const std::u16string& name) const {
  for (const XmlAttr& a : attrs_) {
    if (a.name == name) return true;
  }
  return false;
}

Value ToolXMLElement::as_value() const {
  std::optional<std::u16string> t = type();
  if (t) {
    if (*t == u"string") {
      std::optional<std::u16string> v = as_string();
      if (v) return Value(*v);
      return Value();
    }
    if (*t == u"number") {
      std::optional<double> v = as_number();
      if (v) return Value(*v);
      return Value();
    }
    if (*t == u"boolean") {
      std::optional<bool> v = as_boolean();
      if (v) return Value(*v);
      return Value();
    }
    if (*t == u"array") return as_array();
  }
  return as_object();
}

std::optional<std::u16string> ToolXMLElement::as_string() const {
  if (type() != u"string") return std::nullopt;
  std::optional<std::u16string> v = attr(u"value");
  if (v) return v;
  return text_;
}

std::u16string ToolXMLElement::as_string(const std::u16string& or_value) const {
  std::optional<std::u16string> v = as_string();
  return v ? *v : or_value;
}

std::optional<double> ToolXMLElement::as_number() const {
  if (type() != u"number") return std::nullopt;
  std::optional<std::u16string> txt = as_string();
  if (!txt) return std::nullopt;
  double ret = string_to_number(*txt);
  if (std::isnan(ret)) return std::nullopt;
  return ret;
}

double ToolXMLElement::as_number(double or_value) const {
  std::optional<double> v = as_number();
  return v ? *v : or_value;
}

std::optional<bool> ToolXMLElement::as_boolean() const {
  if (type() != u"boolean") return std::nullopt;
  std::optional<std::u16string> txt = as_string();
  if (txt) {
    std::u16string lower = to_lower_case(*txt);
    if (lower == u"1" || lower == u"true") return true;
    if (lower == u"0" || lower == u"false") return false;
  }
  return std::nullopt;
}

bool ToolXMLElement::as_boolean(bool or_value) const {
  std::optional<bool> v = as_boolean();
  return v ? *v : or_value;
}

Value ToolXMLElement::as_array() const {
  if (type() != u"array") return Value();
  std::shared_ptr<Array> ret = std::make_shared<Array>();
  for (IXMLElement* child : child_views_) ret->push_back(child->as_value());
  return Value(std::move(ret));
}

Value ToolXMLElement::as_array(const Value& or_value) const {
  if (type() != u"array") return or_value;
  std::shared_ptr<Array> ret = std::make_shared<Array>();
  for (IXMLElement* child : child_views_) ret->push_back(child->as_value());
  return Value(std::move(ret));
}

Value ToolXMLElement::as_object() const {
  std::shared_ptr<Object> ret = std::make_shared<Object>();
  for (const XmlAttr& a : attrs_) ret->set(a.name, Value(a.value));
  for (IXMLElement* child : child_views_) {
    std::optional<std::u16string> name = child->attr(u"name");
    std::u16string key = (name && !name->empty()) ? *name : child->tag();
    if (!key.empty()) ret->set(key, child->as_value());
  }
  if (ret->size() != 0) return Value(std::move(ret));
  return Value();
}

Value ToolXMLElement::as_object(const Value& or_value) const {
  std::shared_ptr<Object> ret = std::make_shared<Object>();
  for (const XmlAttr& a : attrs_) ret->set(a.name, Value(a.value));
  for (IXMLElement* child : child_views_) {
    std::optional<std::u16string> name = child->attr(u"name");
    std::u16string key = (name && !name->empty()) ? *name : child->tag();
    if (!key.empty()) ret->set(key, child->as_value());
  }
  if (ret->size() != 0) return Value(std::move(ret));
  return or_value;
}

std::u16string ToolXMLElement::action_str() const {
  std::optional<std::u16string> name = attr(u"action");
  if (!name || name->empty()) name = attr(u"name");
  if (name && !name->empty()) {
    std::optional<std::u16string> args = attr(u"args");
    if (args && !args->empty()) return *name + u"(" + *args + u")";
    return *name + u"()";
  }
  return text_;
}

std::u16string ToolXMLElement::stringify() const {
  FxpChild root = make_fxp(*this);
  if (root.undef) return std::u16string();
  std::u16string out;
  build_xml(root, 0, out);
  return out;
}

void ToolXMLElement::insert(const std::shared_ptr<IXMLElement>& child,
                            std::optional<std::size_t> index) {
  if (!child) return;
  ToolXMLElement* elem = static_cast<ToolXMLElement*>(child.get());
  elem->parent_ = this;
  if (!index || *index >= child_views_.size()) {
    child_owners_.push_back(child);
    child_views_.push_back(elem);
    return;
  }
  child_owners_.insert(child_owners_.begin() + static_cast<std::ptrdiff_t>(*index), child);
  child_views_.insert(child_views_.begin() + static_cast<std::ptrdiff_t>(*index), elem);
}

void ToolXMLElement::add_child(const std::shared_ptr<IXMLElement>& child) {
  insert(child, std::nullopt);
}

bool ToolXMLElement::remove(IXMLElement* child) {
  for (size_t i = 0; i < child_views_.size(); ++i) {
    if (child_views_[i] == child) {
      static_cast<ToolXMLElement*>(child)->parent_ = nullptr;
      child_owners_.erase(child_owners_.begin() + static_cast<std::ptrdiff_t>(i));
      child_views_.erase(child_views_.begin() + static_cast<std::ptrdiff_t>(i));
      return true;
    }
  }
  return false;
}

bool ToolXMLElement::remove_self() {
  if (parent_ == nullptr) return false;
  return parent_->remove(this);
}

void ToolXMLElement::remove_all() {
  for (IXMLElement* child : child_views_) static_cast<ToolXMLElement*>(child)->parent_ = nullptr;
  child_owners_.clear();
  child_views_.clear();
}

std::vector<IXMLElement*> ToolXMLElement::children_by_tag(const std::u16string& tag) const {
  std::vector<IXMLElement*> ret;
  for (IXMLElement* child : child_views_) {
    if (child->tag() == tag) ret.push_back(child);
  }
  return ret;
}

IXMLElement* ToolXMLElement::child_by_tag(const std::u16string& tag) const {
  for (IXMLElement* child : child_views_) {
    if (child->tag() == tag) return child;
  }
  return nullptr;
}

std::optional<std::u16string> ToolXMLElement::get_str(const std::u16string& name) const {
  if (IXMLElement* child = child_by_tag(name)) {
    std::optional<std::u16string> v = child->as_string();
    if (v) return v;
  }
  return attr(name);
}

std::u16string ToolXMLElement::get_str(const std::u16string& name,
                                       const std::u16string& or_value) const {
  std::optional<std::u16string> v = get_str(name);
  return v ? *v : or_value;
}

std::optional<double> ToolXMLElement::get_num(const std::u16string& name) const {
  if (IXMLElement* child = child_by_tag(name)) {
    std::optional<double> v = child->as_number();
    if (v) return v;
  }
  return num_attr(name);
}

double ToolXMLElement::get_num(const std::u16string& name, double or_value) const {
  std::optional<double> v = get_num(name);
  return v ? *v : or_value;
}

std::optional<bool> ToolXMLElement::get_bool(const std::u16string& name) const {
  if (IXMLElement* child = child_by_tag(name)) {
    std::optional<bool> v = child->as_boolean();
    if (v) return v;
  }
  return bool_attr(name);
}

bool ToolXMLElement::get_bool(const std::u16string& name, bool or_value) const {
  std::optional<bool> v = get_bool(name);
  return v ? *v : or_value;
}

std::optional<std::vector<std::u16string>> ToolXMLElement::get_str_arr(
    const std::u16string& name) const {
  std::optional<std::vector<std::u16string>> ret;
  for (IXMLElement* child : child_views_) {
    if (child->tag() != name) continue;
    std::optional<std::u16string> v = child->as_string();
    if (!v) continue;
    if (!ret) ret = std::vector<std::u16string>();
    ret->push_back(*v);
  }
  std::optional<std::vector<std::u16string>> b = strs_attr(name);
  if (b) {
    if (!ret) ret = std::vector<std::u16string>();
    ret->insert(ret->end(), b->begin(), b->end());
  }
  return ret;
}

std::optional<std::vector<double>> ToolXMLElement::get_num_arr(
    const std::u16string& name) const {
  std::optional<std::vector<double>> ret;
  for (IXMLElement* child : child_views_) {
    if (child->tag() != name) continue;
    std::optional<double> v = child->as_number();
    if (!v) continue;
    if (!ret) ret = std::vector<double>();
    ret->push_back(*v);
  }
  std::optional<std::vector<double>> b = nums_attr(name);
  if (b) {
    if (!ret) ret = std::vector<double>();
    ret->insert(ret->end(), b->begin(), b->end());
  }
  return ret;
}

Value ToolXMLElement::get_obj(const std::u16string& tag) const {
  IXMLElement* child = child_by_tag(tag);
  if (child == nullptr) return Value();
  return child->as_object();
}

}
