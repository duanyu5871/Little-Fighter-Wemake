#include "lfw/dat_translator/xml/xml_from_json.h"

#include <memory>
#include <variant>

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// JS `typeof v === "object"`（数组也算；null 单独判）。
bool is_object_like(const Value& v) {
  return as_object(v) != nullptr || as_array(v) != nullptr;
}

bool is_empty_string(const Value& v) {
  const std::u16string* const s = std::get_if<std::u16string>(&v);
  return s != nullptr && s->empty();
}

// `esc`：只换四个，`String(s)` 语义。
std::u16string esc(const Value& v) {
  const std::u16string s = to_string(v);
  std::u16string out;
  for (char16_t c : s) {
    switch (c) {
      case u'<': out += u"&lt;"; break;
      case u'>': out += u"&gt;"; break;
      case u'&': out += u"&amp;"; break;
      case u'"': out += u"&quot;"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

// `attrs(obj)`：undefined/null/空串跳过；数组分支在 TS 里接不到（attrsOf 早滤了），
// 端口保留同样的形状以防万一。
std::u16string attrs(const Object& obj) {
  std::u16string out;
  for (const std::u16string& key : obj.keys()) {
    const Value* const v = obj.get(key);
    if (v == nullptr || is_nullish(*v) || is_empty_string(*v)) continue;
    out += u" " + key + u"=\"";
    if (const Array* const a = as_array(*v)) {
      std::u16string joined;
      for (size_t i = 0; i < a->size(); ++i) {
        if (i != 0) joined.push_back(u',');
        if (is_nullish(a->at(i))) continue;
        joined += to_string(a->at(i));
      }
      out += esc(Value(std::move(joined)));
    } else {
      out += esc(*v);
    }
    out.push_back(u'"');
  }
  return out;
}

// `attrsOf`：`typeof v === "object"`（数组也算）直接跳过 ⇒ 数组进不了属性表。
std::u16string attrs_of(const Object& obj,
                        const std::optional<std::vector<std::u16string>>& key_order) {
  const std::vector<std::u16string> keys = key_order ? *key_order : obj.keys();
  auto attr_obj = std::make_shared<Object>();
  for (const std::u16string& k : keys) {
    const Value* const v = obj.get(k);
    if (v == nullptr) continue;
    if (is_nullish(*v) || is_object_like(*v)) continue;
    attr_obj->set(k, *v);
  }
  return attrs(*attr_obj);
}

bool has_object_value(const Object& obj) {
  for (const std::u16string& k : obj.keys()) {
    const Value* const v = obj.get(k);
    if (v == nullptr || is_nullish(*v)) continue;
    if (is_object_like(*v)) return true;
  }
  return false;
}

std::u16string children_xml(const Object& obj, const std::u16string& indent);

void emit_object_child(std::u16string& xml, const std::u16string& indent,
                       const std::u16string& child_tag, const Object& item) {
  xml += indent + u"  <" + child_tag + attrs_of(item, std::nullopt);
  if (has_object_value(item)) {
    xml += u">\n";
    xml += children_xml(item, indent + u"  ");
    xml += indent + u"  </" + child_tag + u">\n";
  } else {
    xml += u" />\n";
  }
}

std::u16string children_xml(const Object& obj, const std::u16string& indent) {
  std::u16string xml;
  for (const std::u16string& key : obj.keys()) {
    const Value* const kv = obj.get(key);
    if (kv == nullptr) continue;
    const Value& v = *kv;
    if (is_nullish(v)) continue;

    if (const Array* const a = as_array(v)) {
      bool all_primitive = true;
      for (size_t i = 0; i < a->size(); ++i) {
        if (is_object_like(a->at(i))) {
          all_primitive = false;
          break;
        }
      }
      if (all_primitive) continue;
      for (size_t i = 0; i < a->size(); ++i) {
        const Value& item = a->at(i);
        const Object* const item_o = as_object(item);
        if (item_o != nullptr && !std::holds_alternative<NullTag>(item)) {
          std::u16string child_tag = key;
          const Value* const tn = item_o->get(u"tagName");
          if (tn != nullptr && !is_nullish(*tn)) child_tag = to_string(*tn);
          emit_object_child(xml, indent, child_tag, *item_o);
        } else {
          xml += indent + u"  <" + key + u">" + esc(item) + u"</" + key + u">\n";
        }
      }
    } else if (is_object_like(v) && !std::holds_alternative<NullTag>(v)) {
      emit_object_child(xml, indent, key, *as_object(v));
    }
  }
  return xml;
}

}  // namespace

std::u16string xml_from_json(const Value& data, const std::u16string& tag_name,
                             const std::optional<std::vector<std::u16string>>& key_order) {
  const Object* const d = as_object(data);
  if (d == nullptr) return u"<" + tag_name + u">\n</" + tag_name + u">\n";
  std::u16string xml = u"<" + tag_name + attrs_of(*d, key_order) + u">\n";
  xml += children_xml(*d, u"");
  xml += u"</" + tag_name + u">\n";
  return xml;
}

}
}
}
