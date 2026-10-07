#include "lfw/ditto/xml/tool_xml.h"

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"

namespace lfw {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

std::shared_ptr<ToolXMLElement> make(const std::u16string& tag) {
  return std::make_shared<ToolXMLElement>(tag);
}

void set_attr_str(ToolXMLElement& el, const std::u16string& name, const std::u16string& value) {
  el.set_attr(name, Value(value), u',');
}

}  // namespace

std::shared_ptr<IXMLElement> ToolXML::parse(const std::u16string& text) {
  (void)text;
  return nullptr;
}

std::shared_ptr<IXMLElement> ToolXML::create(const std::u16string& tag) {
  return make(tag);
}

std::shared_ptr<IXMLElement> ToolXML::from_string(const std::u16string& str,
                                                  const std::u16string& tag) {
  std::shared_ptr<ToolXMLElement> el = make(tag);
  set_attr_str(*el, u"type", u"string");
  set_attr_str(*el, u"value", str);
  return el;
}

std::shared_ptr<IXMLElement> ToolXML::from_number(double num, const std::u16string& tag) {
  std::shared_ptr<ToolXMLElement> el = make(tag);
  set_attr_str(*el, u"type", u"number");
  el->set_text(number_to_string(num));
  return el;
}

std::shared_ptr<IXMLElement> ToolXML::from_boolean(bool v, const std::u16string& tag) {
  std::shared_ptr<ToolXMLElement> el = make(tag);
  set_attr_str(*el, u"type", u"boolean");
  el->set_text(v ? u"true" : u"false");
  return el;
}

std::shared_ptr<IXMLElement> ToolXML::from_array(const Value& arr, const std::u16string& tag) {
  std::shared_ptr<ToolXMLElement> el = make(tag);
  set_attr_str(*el, u"type", u"array");
  const Array* a = as_array(arr);
  if (a != nullptr) {
    for (size_t i = 0; i < a->size(); ++i) el->add_child(from_value(a->at(i)));
  }
  return el;
}

std::shared_ptr<IXMLElement> ToolXML::from_object(const Value& obj, const std::u16string& tag) {
  std::shared_ptr<ToolXMLElement> el = make(tag);
  set_attr_str(*el, u"type", u"object");
  const Object* o = as_object(obj);
  if (o != nullptr) {
    for (const std::u16string& key : object_keys(obj)) {
      const Value* v = o->get(key);
      std::shared_ptr<IXMLElement> child = from_value(*v);
      child->set_attr(u"name", Value(key));
      el->add_child(child);
    }
  }
  return el;
}

std::shared_ptr<IXMLElement> ToolXML::from_value(const Value& value) {
  if (is_nullish(value)) {
    std::shared_ptr<ToolXMLElement> el = make(u"string");
    set_attr_str(*el, u"type", u"string");
    return el;
  }
  if (const std::u16string* s = std::get_if<std::u16string>(&value)) {
    return from_string(*s, u"string");
  }
  if (const double* d = std::get_if<double>(&value)) return from_number(*d, u"number");
  if (const bool* b = std::get_if<bool>(&value)) return from_boolean(*b, u"boolean");
  if (as_array(value) != nullptr) return from_array(value, u"array");
  return from_object(value, u"object");
}

}
