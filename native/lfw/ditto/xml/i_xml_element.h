#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

// Mirrors `src/LFW/ditto/xml/IXMLElement.ts`（**依赖注入缝**，见 DESIGN 4.66：`ditto/` 是
// 宿主装进来的服务接口，不是要整块搬的代码）。LFW 的 `dat_translator/xml/*` 只透过这个接口
// 读写 XML；实现由宿主提供 —— TS 有两份（`DittoImpl/xml/XMLElement.ts` 的浏览器 DOM 版、
// `tool/src/xml/ToolXMLElement.ts` 的 Node 版），端口默认把 tool 那份搬成
// `lfw/ditto/xml/tool_xml_element.h`（纯逻辑、无平台依赖），两份 TS 实现之间的差异记在
// README 偏差表。
//
// 形状约定：
//   * `Voidable<T>` / `T | undefined` ⇒ `std::optional<T>`；可空成员（`strs_attr_soft` 的项）
//     ⇒ `std::optional<T>` 嵌一层；
//   * 无参 `as_string()` 这类「缺省给 `undefined`」的返回 ⇒ `std::optional<T>`；TS 的
//     `as_string(or)` 重载 ⇒ 同名重载（`or` 由调用方给）；
//   * `set_attr` / `as_value` / `as_object` 这类装 JS 值的口子 ⇒ `Value`（`undefined` 是
//     `monostate`、`null` 是 `NullTag`、数组是 `Array`）。
//
// 所有权：工厂（`IXML`）交出 `std::shared_ptr<IXMLElement>`，`insert` 收 `shared_ptr`；读向
// （`children` / `child_by_tag` / `parent`）给裸指针。TS 的 `insert` 允许同一个元素挂在两个
// 父下（tool 实现不先摘旧的）⇒ 端口用 `shared_ptr` 才装得下这种「重复挂」。
struct XmlAttr {
  std::u16string name;
  std::u16string value;
};

class IXMLElement {
 public:
  virtual ~IXMLElement() = default;

  // ---- `get tag` / `get children` / `get attrs` / `get text` / `get parent` ----
  virtual const std::u16string& tag() const = 0;
  virtual const std::vector<IXMLElement*>& children() const = 0;
  virtual const std::vector<XmlAttr>& attrs() const = 0;
  virtual const std::u16string& text() const = 0;
  // `nullptr` = TS 的 `undefined`。
  virtual IXMLElement* parent() const = 0;

  // ---- 属性读取 ----
  virtual std::optional<std::u16string> attr(const std::u16string& name) const = 0;
  virtual std::optional<std::u16string> str_attr(const std::u16string& name) const = 0;
  virtual std::optional<double> num_attr(const std::u16string& name) const = 0;
  virtual std::optional<bool> bool_attr(const std::u16string& name) const = 0;
  virtual std::optional<std::vector<std::u16string>> strs_attr(const std::u16string& name,
                                                               char16_t sep = u',') const = 0;
  virtual std::optional<std::vector<double>> nums_attr(const std::u16string& name,
                                                       char16_t sep = u',') const = 0;
  virtual std::optional<std::vector<std::optional<std::u16string>>> strs_attr_soft(
      const std::u16string& name, char16_t sep = u',') const = 0;
  virtual std::optional<std::vector<std::optional<double>>> nums_attr_soft(
      const std::u16string& name, char16_t sep = u',') const = 0;

  // ---- 属性写入 ----
  // `value` 为 `undefined` / `null` 时删属性；数组按 `sep` 连接；其余 `String(value)`。
  virtual void set_attr(const std::u16string& name, const Value& value, char16_t sep = u',') = 0;
  virtual void del_attr(const std::u16string& name) = 0;
  // `set_arr_attr_soft`：数组尾部的 nullish 丢、中间的 nullish 变空串；数组为空 ⇒ 删属性。
  virtual void set_arr_attr_soft(const std::u16string& name, const Value& value,
                                 char16_t sep = u',') = 0;
  virtual bool has_attr(const std::u16string& name) const = 0;
  virtual void set_text(const std::u16string& text) = 0;

  // ---- 取值 ----
  // `type` 是 impl 内部概念（接口里没有）：按 `type` 属性 / 标签名推。
  virtual Value as_value() const = 0;
  virtual std::optional<std::u16string> as_string() const = 0;
  virtual std::u16string as_string(const std::u16string& or_value) const = 0;
  virtual std::optional<double> as_number() const = 0;
  virtual double as_number(double or_value) const = 0;
  virtual std::optional<bool> as_boolean() const = 0;
  virtual bool as_boolean(bool or_value) const = 0;
  virtual Value as_array() const = 0;
  virtual Value as_array(const Value& or_value) const = 0;
  virtual Value as_object() const = 0;
  virtual Value as_object(const Value& or_value) const = 0;

  // `action_str()`：`action(args)` / `name(args)` 这类动作文本。
  virtual std::u16string action_str() const = 0;
  // `stringify()`：fast-xml-parser 的 `XMLBuilder` 输出（见 impl 顶部说明）。
  virtual std::u16string stringify() const = 0;

  // ---- 结构操作 ----
  virtual void insert(const std::shared_ptr<IXMLElement>& child,
                      std::optional<std::size_t> index = std::nullopt) = 0;
  virtual bool remove(IXMLElement* child) = 0;
  virtual bool remove_self() = 0;
  virtual void remove_all() = 0;

  virtual std::vector<IXMLElement*> children_by_tag(const std::u16string& tag) const = 0;
  virtual IXMLElement* child_by_tag(const std::u16string& tag) const = 0;

  // ---- `get_*`：先找同名子元素的 `as_*()`，再回落到同名属性，最后给 `or` ----
  // ⚠ `get_str_arr` / `get_num_arr` 是**例外**：tool 实现里那个 `or` 被忽略了（直接
  // `return ret`，缺省就是 `undefined`）⇒ 端口只给 optional 版（浏览器实现是 `?? or`，
  // 这份差异记在 README 偏差表）。
  virtual std::optional<std::u16string> get_str(const std::u16string& name) const = 0;
  virtual std::u16string get_str(const std::u16string& name,
                                 const std::u16string& or_value) const = 0;
  virtual std::optional<double> get_num(const std::u16string& name) const = 0;
  virtual double get_num(const std::u16string& name, double or_value) const = 0;
  virtual std::optional<bool> get_bool(const std::u16string& name) const = 0;
  virtual bool get_bool(const std::u16string& name, bool or_value) const = 0;
  virtual std::optional<std::vector<std::u16string>> get_str_arr(
      const std::u16string& name) const = 0;
  virtual std::optional<std::vector<double>> get_num_arr(const std::u16string& name) const = 0;
  virtual Value get_obj(const std::u16string& tag) const = 0;
};

}
