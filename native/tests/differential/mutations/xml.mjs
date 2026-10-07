// `lfw/ditto/xml` 的 tool 实现（`native/lfw/ditto/xml/tool_xml_element.cpp` + `tool_xml.cpp`）。
//
// 用例：`cases/xml/{factory,quirks,reads,tree}.txt`。
//
// 有意不覆盖（不可观察 / 走不到 / 台面造不出）：
//   * `as_number()` / `as_boolean()` 的**函数体尾段**（`string_to_number` / isNaN / `'1'`、`'true'`
//     判定）：tool 实现里它先调 `as_string()`，而 `as_string` 又按 `type == 'string'` 挡了一道
//     ⇒ 对 `type` 是 number / boolean 的元素恒走缺省，尾段不可达（浏览器实现不走 `as_string`，
//     那份差异记 README 偏差表）。用例只验证「恒缺省」这个事实。
//   * `get_num_arr` 的子元素循环：同上（子元素走 `as_number` 恒 undefined）⇒ 只有属性那半边可观察。
//   * `parse(text)`：fast-xml-parser 的解析等价物没搬（`i_xml.h` 注释）⇒ 没有用例。
//   * `attr` 名大小写 / 多字符 `sep`：LFW 调用点没有，台面也不脚本化。
//   * `del_attr` 的「只删第一个同名」：`set_attr` 保唯一，造不出重复属性名。
//   * `insert` / `remove` 里的 `static_cast<ToolXMLElement*>`：台面上只有这一种实现。
export default {
  subject: "xml",
  cases: ["factory", "quirks", "reads", "tree"],
  mutations: [
    // ---------------------------------------------------------------- type() / attr 读取
    {
      note: "type(): 标签名不小写",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  std::u16string t = to_lower_case(tag_);`,
      to: `  std::u16string t = tag_;`,
    },
    {
      note: "type(): `value` 的 type 属性不小写",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `    if (!a) return std::nullopt;
    return to_lower_case(*a);`,
      to: `    if (!a) return std::nullopt;
    return a;`,
    },
    {
      note: "type(): `value` 特殊分支关掉",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  if (t == u"value") {`,
      to: `  if (t == u"value2") {`,
    },
    {
      note: "attr(): 不看名字、一律取第一个",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `    if (a.name == name) return a.value;`,
      to: `    if (true) return a.value;`,
    },
    {
      note: "num_attr(): 一律 0",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  return string_to_number(*v);
}

std::optional<bool> ToolXMLElement::bool_attr`,
      to: `  return 0.0;
}

std::optional<bool> ToolXMLElement::bool_attr`,
    },
    {
      note: "bool_attr(): 一律 true",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  return *v == u"true" || *v == u"1";`,
      to: `  return true;`,
    },
    {
      note: "strs_attr(): 项不 trim",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  for (const std::u16string& part : split_char(*v, sep)) ret.push_back(trim(part));`,
      to: `  for (const std::u16string& part : split_char(*v, sep)) ret.push_back(part);`,
    },
    {
      note: "strs_attr_soft(): 空项给空串而不是 undefined",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `    if (t.empty()) ret.push_back(std::nullopt);
    else ret.push_back(t);`,
      to: `    if (false) ret.push_back(std::nullopt);
    else ret.push_back(t);`,
    },
    {
      note: "nums_attr_soft(): 空项给 0 而不是 undefined",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `    if (t.empty()) ret.push_back(std::nullopt);
    else ret.push_back(string_to_number(t));`,
      to: `    if (false) ret.push_back(std::nullopt);
    else ret.push_back(string_to_number(t));`,
    },
    {
      note: "has_attr(): 找不到也 true",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `    if (a.name == name) return true;
  }
  return false;`,
      to: `    if (a.name == name) return true;
  }
  return true;`,
    },

    // ---------------------------------------------------------------- 属性写入
    {
      note: "set_attr(): nullish 不删属性（写成 \"undefined\"）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  if (is_nullish(value)) {
    del_attr(name);
    return;
  }
  std::u16string sv;`,
      to: `  if (is_nullish(value) && false) {
    del_attr(name);
    return;
  }
  std::u16string sv;`,
    },
    {
      note: "set_attr(): 已存在同名属性也追加（不原地改）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  for (XmlAttr& a : attrs_) {
    if (a.name == name) {
      a.value = sv;
      return;
    }
  }
  attrs_.push_back(XmlAttr{name, sv});`,
      to: `  attrs_.push_back(XmlAttr{name, sv});`,
    },
    {
      note: "set_arr_attr_soft(): 不丢尾部 nullish",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  while (!items.empty() && is_nullish(items.back())) items.pop_back();`,
      to: `  ;`,
    },

    // ---------------------------------------------------------------- as_* / action
    {
      note: "as_string(): 去掉 type 门（错类型的也回文本）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  if (type() != u"string") return std::nullopt;
  std::optional<std::u16string> v = attr(u"value");`,
      to: `  std::optional<std::u16string> v = attr(u"value");`,
    },
    {
      note: "as_array(): 不收子元素（恒空）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  for (IXMLElement* child : child_views_) ret->push_back(child->as_value());
  return Value(std::move(ret));
}

Value ToolXMLElement::as_array(const Value& or_value) const {`,
      to: `  return Value(std::move(ret));
}

Value ToolXMLElement::as_array(const Value& or_value) const {`,
    },
    {
      note: "as_array(or): 错类型不给 or（给 undefined）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  if (type() != u"array") return or_value;
  std::shared_ptr<Array> ret = std::make_shared<Array>();`,
      to: `  if (type() != u"array") return Value();
  std::shared_ptr<Array> ret = std::make_shared<Array>();`,
    },
    {
      note: "as_object(): 非空也回 undefined",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  if (ret->size() != 0) return Value(std::move(ret));
  return Value();
}

Value ToolXMLElement::as_object(const Value& or_value) const {`,
      to: `  return Value();
}

Value ToolXMLElement::as_object(const Value& or_value) const {`,
    },
    {
      note: "as_object(or): 非空也回 or",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  if (ret->size() != 0) return Value(std::move(ret));
  return or_value;
}`,
      to: `  return or_value;
}`,
    },
    {
      note: "action_str(): name 回落的空串语义关掉",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  if (!name || name->empty()) name = attr(u"name");`,
      to: `  if (!name) name = attr(u"name");`,
    },
    {
      note: "action_str(): 忽略 args",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `    if (args && !args->empty()) return *name + u"(" + *args + u")";
    return *name + u"()";`,
      to: `    return *name + u"()";`,
    },

    // ---------------------------------------------------------------- stringify
    {
      note: "stringify(): 空根不重定向（回 `<tag></tag>`）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  if (root.undef) return std::u16string();`,
      to: `  if (false) return std::u16string();`,
    },
    {
      note: "stringify(): 同名子元素不分组（按插入序）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `        if (g.key == key) {`,
      to: `        if (false) {`,
    },
    {
      note: "stringify(): 空子元素不再丢（any_child 恒真）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `      if (!c.undef) any_child = true;`,
      to: `      any_child = true;`,
    },
    {
      note: "stringify(): 空子元素也渲染出来",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `        if (!c.undef) build_xml(c, depth + 1, out);`,
      to: `        build_xml(c, depth + 1, out);`,
    },
    {
      note: "stringify(): 文本分支关掉",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  } else if (node.has_text) {`,
      to: `  } else if (false) {`,
    },
    {
      note: "stringify(): 缩进回到 0 之外（2 → 3 空格）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  std::u16string indent(depth * 2, u' ');`,
      to: `  std::u16string indent(depth * 3, u' ');`,
    },
    {
      note: "stringify(): 不转义 `<`",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `      case u'<': out += u"&lt;"; break;`,
      to: `      case u'<': out.push_back(c); break;`,
    },
    {
      note: "stringify(): 不转义 `&`",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `      case u'&': out += u"&amp;"; break;`,
      to: `      case u'&': out.push_back(c); break;`,
    },

    // ---------------------------------------------------------------- 结构操作
    {
      note: "insert(): 下标落在中间也当追加",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  if (!index || *index >= child_views_.size()) {`,
      to: `  if (!index || *index < child_views_.size()) {`,
    },
    {
      note: "remove(): 删成功也回 false",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `      return true;
    }
  }
  return false;
}

bool ToolXMLElement::remove_self() {`,
      to: `      return false;
    }
  }
  return false;
}

bool ToolXMLElement::remove_self() {`,
    },
    {
      note: "remove_self(): 没有父也回 true",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  if (parent_ == nullptr) return false;
  return parent_->remove(this);`,
      to: `  if (parent_ == nullptr) return true;
  return parent_->remove(this);`,
    },
    {
      note: "remove_all(): 不摘孩子的 parent",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  for (IXMLElement* child : child_views_) static_cast<ToolXMLElement*>(child)->parent_ = nullptr;
  child_owners_.clear();`,
      to: `  child_owners_.clear();`,
    },
    {
      note: "children_by_tag(): 取反",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `    if (child->tag() == tag) ret.push_back(child);`,
      to: `    if (child->tag() != tag) ret.push_back(child);`,
    },
    {
      note: "child_by_tag(): 取最后一个同名",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `IXMLElement* ToolXMLElement::child_by_tag(const std::u16string& tag) const {
  for (IXMLElement* child : child_views_) {
    if (child->tag() == tag) return child;
  }
  return nullptr;
}`,
      to: `IXMLElement* ToolXMLElement::child_by_tag(const std::u16string& tag) const {
  IXMLElement* found = nullptr;
  for (IXMLElement* child : child_views_) {
    if (child->tag() == tag) found = child;
  }
  return found;
}`,
    },

    // ---------------------------------------------------------------- get_*
    {
      note: "get_str(): 属性优先（回落链反了）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `std::optional<std::u16string> ToolXMLElement::get_str(const std::u16string& name) const {
  if (IXMLElement* child = child_by_tag(name)) {
    std::optional<std::u16string> v = child->as_string();
    if (v) return v;
  }
  return attr(name);
}`,
      to: `std::optional<std::u16string> ToolXMLElement::get_str(const std::u16string& name) const {
  if (std::optional<std::u16string> a = attr(name)) return a;
  if (IXMLElement* child = child_by_tag(name)) {
    std::optional<std::u16string> v = child->as_string();
    if (v) return v;
  }
  return std::nullopt;
}`,
    },
    {
      note: "get_num(): 去掉属性回落",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  return num_attr(name);
}`,
      to: `  return std::nullopt;
}`,
    },
    {
      note: "get_bool(): 去掉属性回落",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  return bool_attr(name);
}`,
      to: `  return std::nullopt;
}`,
    },
    {
      note: "get_str_arr(): 不合并子元素",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  std::optional<std::vector<std::u16string>> ret;
  for (IXMLElement* child : child_views_) {
    if (child->tag() != name) continue;
    std::optional<std::u16string> v = child->as_string();
    if (!v) continue;
    if (!ret) ret = std::vector<std::u16string>();
    ret->push_back(*v);
  }
  std::optional<std::vector<std::u16string>> b = strs_attr(name);`,
      to: `  std::optional<std::vector<std::u16string>> ret;
  std::optional<std::vector<std::u16string>> b = strs_attr(name);`,
    },
    {
      note: "get_str_arr(): 属性插到前面（合并顺序反了）",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  std::optional<std::vector<std::u16string>> b = strs_attr(name);
  if (b) {
    if (!ret) ret = std::vector<std::u16string>();
    ret->insert(ret->end(), b->begin(), b->end());
  }
  return ret;
}`,
      to: `  std::optional<std::vector<std::u16string>> b = strs_attr(name);
  if (b) {
    if (!ret) ret = std::vector<std::u16string>();
    ret->insert(ret->begin(), b->begin(), b->end());
  }
  return ret;
}`,
    },
    {
      note: "get_obj(): 回 as_value 而不是 as_object",
      file: "native/lfw/ditto/xml/tool_xml_element.cpp",
      from: `  return child->as_object();`,
      to: `  return child->as_value();`,
    },

    // ---------------------------------------------------------------- 工厂（tool_xml.cpp）
    {
      note: "from_string(): 不写 value 属性",
      file: "native/lfw/ditto/xml/tool_xml.cpp",
      from: `  set_attr_str(*el, u"type", u"string");
  set_attr_str(*el, u"value", str);`,
      to: `  set_attr_str(*el, u"type", u"string");`,
    },
    {
      note: "from_number(): 文本改成 value 属性",
      file: "native/lfw/ditto/xml/tool_xml.cpp",
      from: `  el->set_text(number_to_string(num));`,
      to: `  set_attr_str(*el, u"value", number_to_string(num));`,
    },
    {
      note: "from_boolean(): 文本改成 value 属性",
      file: "native/lfw/ditto/xml/tool_xml.cpp",
      from: `  el->set_text(v ? u"true" : u"false");`,
      to: `  set_attr_str(*el, u"value", v ? u"true" : u"false");`,
    },
    {
      note: "from_array(): 丢最后一项",
      file: "native/lfw/ditto/xml/tool_xml.cpp",
      from: `    for (size_t i = 0; i < a->size(); ++i) el->add_child(from_value(a->at(i)));`,
      to: `    for (size_t i = 0; i + 1 < a->size(); ++i) el->add_child(from_value(a->at(i)));`,
    },
    {
      note: "from_object(): 不写 name 属性",
      file: "native/lfw/ditto/xml/tool_xml.cpp",
      from: `      child->set_attr(u"name", Value(key));`,
      to: `      (void)key;`,
    },
    {
      note: "from_value(): nullish 的子元素标签换成 `number`",
      file: "native/lfw/ditto/xml/tool_xml.cpp",
      from: `    std::shared_ptr<ToolXMLElement> el = make(u"string");`,
      to: `    std::shared_ptr<ToolXMLElement> el = make(u"number");`,
    },
    {
      note: "from_value(): 数组分支关掉（当对象走）",
      file: "native/lfw/ditto/xml/tool_xml.cpp",
      from: `  if (as_array(value) != nullptr) return from_array(value, u"array");`,
      to: `  if (as_array(value) != nullptr && false) return from_array(value, u"array");`,
    },
    {
      note: "create(): 标签换成固定值",
      file: "native/lfw/ditto/xml/tool_xml.cpp",
      from: `std::shared_ptr<IXMLElement> ToolXML::create(const std::u16string& tag) {
  return make(tag);
}`,
      to: `std::shared_ptr<IXMLElement> ToolXML::create(const std::u16string& tag) {
  return make(u"item");
}`,
    },
  ],
};
