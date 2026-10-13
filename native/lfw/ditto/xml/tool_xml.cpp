#include "lfw/ditto/xml/tool_xml.h"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

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

bool is_ws(char16_t c) {
  return c == u' ' || c == u'\t' || c == u'\n' || c == u'\r' || c == u'\f' || c == u'\v';
}

// --- 先把 XML 文本读成「带文档序」的树，再照 `ToolXML::_convert` 铺成 `ToolXMLElement` ---
//
// 形状口径来自 fast-xml-parser（`tool/src/xml/ToolXML.ts` 的那套选项）：
//   * 元素只有**直接文本**（无属性、无子元素）⇒ 文本（空元素 ⇒ 空串）；
//   * 有属性 ⇒ 属性（`String(value)`；无值属性是 `"true"`），直接文本进 `#text`；
//   * 子元素按文档序；同名重复 ⇒ 数组（端口直接把子元素按序挂上，等价）；
//   * 直接文本**保留空白**（`trimValues: false`）并按序拼接；子元素内部的文本不并进来；
//   * `removeNSPrefix`：标签名/属性名的 `ns:` 前缀去掉；
//   * `ignoreDeclaration`：`<?…?>` 丢掉；注释在进解析器之前已被剥掉；
//   * 实体只解命名的那五个（`&amp; &lt; &gt; &quot; &apos;`）—— 十进制/十六进制引用**照抄**
//     （实测 fast-xml-parser 在这个配置下就是不解它们）；
//   * `<![CDATA[…]]>` 的内容原样当文本。
struct XmlNode {
  std::u16string tag;
  std::vector<XmlAttr> attrs;
  std::u16string text;
  std::vector<std::unique_ptr<XmlNode>> children;
};

void strip_prefix(std::u16string& name) {
  const size_t at = name.find(u':');
  if (at != std::u16string::npos) name.erase(0, at + 1);
}

std::u16string decode_entities(const std::u16string& text) {
  std::u16string out;
  out.reserve(text.size());
  struct Ent {
    const char16_t* from;
    char16_t to;
  };
  static const Ent kEnts[] = {{u"&amp;", u'&'},  {u"&lt;", u'<'},
                              {u"&gt;", u'>'},   {u"&quot;", u'"'},
                              {u"&apos;", u'\''}};
  for (size_t i = 0; i < text.size();) {
    if (text[i] != u'&') {
      out.push_back(text[i++]);
      continue;
    }
    bool hit = false;
    for (const Ent& e : kEnts) {
      const std::u16string from(e.from);
      if (text.compare(i, from.size(), from) == 0) {
        out.push_back(e.to);
        i += from.size();
        hit = true;
        break;
      }
    }
    if (!hit) out.push_back(text[i++]);  // 不认识的引用 / 数字引用：照抄
  }
  return out;
}

// `<!-- … -->` 整段剥掉（非贪婪，逐个找 `-->`）。
std::u16string strip_comments(const std::u16string& text) {
  std::u16string out;
  out.reserve(text.size());
  size_t i = 0;
  while (i < text.size()) {
    if (text.compare(i, 4, u"<!--") == 0) {
      const size_t end = text.find(u"-->", i + 4);
      if (end == std::u16string::npos) break;
      i = end + 3;
      continue;
    }
    out.push_back(text[i++]);
  }
  return out;
}

class XmlReader {
 public:
  explicit XmlReader(const std::u16string& text) : _t(text) {}

  // 文档里若干顶层节点（TS 取 `Object.keys(data)[0]` ⇒ 只要第一个）。
  std::unique_ptr<XmlNode> parse_first() {
    std::vector<std::unique_ptr<XmlNode>> roots;
    std::vector<XmlNode*> stack;
    XmlNode* curr = nullptr;
    while (_i < _t.size()) {
      if (_t[_i] != u'<') {
        const size_t next = _t.find(u'<', _i);
        const std::u16string raw =
            _t.substr(_i, (next == std::u16string::npos ? _t.size() : next) - _i);
        if (curr != nullptr) curr->text += decode_entities(raw);
        _i = next == std::u16string::npos ? _t.size() : next;
        continue;
      }
      if (_t.compare(_i, 4, u"<!--") == 0) {
        const size_t end = _t.find(u"-->", _i + 4);
        _i = end == std::u16string::npos ? _t.size() : end + 3;
        continue;
      }
      if (_t.compare(_i, 9, u"<![CDATA[") == 0) {
        const size_t end = _t.find(u"]]>", _i + 9);
        const size_t stop = end == std::u16string::npos ? _t.size() : end;
        if (curr != nullptr) curr->text += _t.substr(_i + 9, stop - _i - 9);
        _i = end == std::u16string::npos ? _t.size() : end + 3;
        continue;
      }
      if (_t.compare(_i, 2, u"<?") == 0) {  // 声明 / 处理指令
        const size_t end = _t.find(u"?>", _i + 2);
        _i = end == std::u16string::npos ? _t.size() : end + 2;
        continue;
      }
      if (_t.compare(_i, 2, u"<!") == 0) {  // `<!DOCTYPE …>`（数据里没有，稳妥跳过）
        const size_t end = _t.find(u'>', _i + 2);
        _i = end == std::u16string::npos ? _t.size() : end + 1;
        continue;
      }
      if (_t.compare(_i, 2, u"</") == 0) {  // 闭合
        const size_t end = _t.find(u'>', _i + 2);
        std::u16string name =
            _t.substr(_i + 2, (end == std::u16string::npos ? _t.size() : end) - _i - 2);
        strip_prefix(name);
        // 与栈顶同名就弹；不同名时按「忽略这次闭合」处理（fast-xml-parser 是宽容的）。
        if (!stack.empty() && stack.back()->tag == name) {
          stack.pop_back();
        } else if (!stack.empty()) {
          for (size_t k = stack.size(); k > 0; --k) {
            if (stack[k - 1]->tag == name) {
              stack.resize(k - 1);
              break;
            }
          }
        }
        _i = end == std::u16string::npos ? _t.size() : end + 1;
        curr = stack.empty() ? nullptr : stack.back();
        continue;
      }
      // 开标签
      ++_i;  // 跳过 '<'
      std::u16string name;
      while (_i < _t.size() && !is_ws(_t[_i]) && _t[_i] != u'>' && _t[_i] != u'/') {
        name.push_back(_t[_i++]);
      }
      strip_prefix(name);
      auto node = std::make_unique<XmlNode>();
      node->tag = name;
      bool self_closing = false;
      while (_i < _t.size()) {
        while (_i < _t.size() && is_ws(_t[_i])) ++_i;
        if (_i < _t.size() && _t[_i] == u'/') {
          self_closing = true;
          ++_i;
          continue;
        }
        if (_i >= _t.size() || _t[_i] == u'>') {
          ++_i;
          break;
        }
        std::u16string aname;
        while (_i < _t.size() && !is_ws(_t[_i]) && _t[_i] != u'=' && _t[_i] != u'>' &&
               _t[_i] != u'/') {
          aname.push_back(_t[_i++]);
        }
        strip_prefix(aname);
        while (_i < _t.size() && is_ws(_t[_i])) ++_i;
        std::u16string avalue;
        if (_i < _t.size() && _t[_i] == u'=') {
          ++_i;
          while (_i < _t.size() && is_ws(_t[_i])) ++_i;
          if (_i < _t.size() && (_t[_i] == u'"' || _t[_i] == u'\'')) {
            const char16_t quote = _t[_i++];
            while (_i < _t.size() && _t[_i] != quote) avalue.push_back(_t[_i++]);
            if (_i < _t.size()) ++_i;  // 收尾引号
          } else {
            while (_i < _t.size() && !is_ws(_t[_i]) && _t[_i] != u'>' && _t[_i] != u'/') {
              avalue.push_back(_t[_i++]);
            }
          }
          avalue = decode_entities(avalue);
        } else {
          // `allowBooleanAttributes`：无值属性 ⇒ `String(true)` = `"true"`。
          avalue = u"true";
        }
        if (!aname.empty()) node->attrs.push_back(XmlAttr{std::move(aname), std::move(avalue)});
      }
      XmlNode* const raw = node.get();
      if (stack.empty()) roots.push_back(std::move(node));
      else stack.back()->children.push_back(std::move(node));
      if (!self_closing) {
        stack.push_back(raw);
        curr = raw;
      } else {
        curr = stack.empty() ? nullptr : stack.back();
      }
    }
    return roots.empty() ? nullptr : std::move(roots.front());
  }

 private:
  const std::u16string& _t;
  size_t _i = 0;
};

// `ToolXML::_convert`：属性 → 文本 → 子元素（顺序照 TS）。
std::shared_ptr<ToolXMLElement> convert(const XmlNode& node) {
  std::shared_ptr<ToolXMLElement> el = make(node.tag);
  for (const XmlAttr& a : node.attrs) el->set_attr(a.name, Value(a.value), u',');
  if (!node.text.empty()) el->set_text(node.text);
  for (const std::unique_ptr<XmlNode>& child : node.children) el->add_child(convert(*child));
  return el;
}

}  // namespace

std::shared_ptr<IXMLElement> ToolXML::parse(const std::u16string& text) {
  // TS：`parser.parse(text.replace(/<!--[\s\S]*?-->/g, ''))` 后取**第一个**顶层键；
  // 一个顶层元素都没有 ⇒ 抛 `No root element found in XML`（端口给 `nullptr`，宿主转成失败）。
  XmlReader reader(strip_comments(text));
  const std::unique_ptr<XmlNode> root = reader.parse_first();
  if (root == nullptr) return nullptr;
  return convert(*root);
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
