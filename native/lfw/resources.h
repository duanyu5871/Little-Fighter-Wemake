#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/zip_mgr.h"

namespace lfw {

// TS `ImportResult<T>`：`data` 的实际类型随方法而定（json 对象 / `BlobUrl`（字符串）/
// `ArrayBuffer` / `ImageBitmap` / `IXMLElement`）⇒ 端口统一用 `Value` 装。
// `file` / `origin` 都是可选的（网络回退那条路**不写** `origin`；`import_image_bitmap` 回退时
// `file` 是 `paths[0]`）⇒ 端口也用 `Value`（没写就是 `undefined`）。
struct ImportResult {
  Value data;
  Value file;
  Value origin;
};

// `I.Ditto.Importer` + `I.Ditto.XML`（TS 里是宿主平台包 `Ditto` 的两个成员）。
// TS 的方法全是 `async`：抛 ⇒ 端口 `false` + `error`（`Resources` 不 catch，直接往外传）。
class IResourcesHost {
 public:
  // `await Importer.import_as_*(urls)` 解出 `[data, hitUrl]`。`image_bitmap` 的调用方只取第一个，
  // 但宿主本来就给两个 ⇒ 端口统一给两个（与 TS 的返回值形状一致）。
  virtual bool import_as_json(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                              std::u16string& error) = 0;
  virtual bool import_as_blob_url(const std::vector<std::u16string>& urls, Value& data,
                                  Value& hit, std::u16string& error) = 0;
  virtual bool import_as_array_buffer(const std::vector<std::u16string>& urls, Value& data,
                                      Value& hit, std::u16string& error) = 0;
  virtual bool import_as_image_bitmap(const std::vector<std::u16string>& urls, Value& data,
                                      Value& hit, std::u16string& error) = 0;
  virtual bool import_as_text(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                              std::u16string& error) = 0;
  // `Ditto.XML.parse(text)`：结果是宿主 XML 元素（`IXMLElement`），端口给个 `Value` 标记。
  virtual bool xml_parse(const Value& text, Value& out, std::u16string& error) = 0;
};

// TS `Resources`。
class Resources {
 public:
  static constexpr const char16_t* kTag = u"Resources";

  Resources(ZipMgr* zip_mgr, IResourcesHost* host);

  ZipMgr* zip_mgr() const { return _zip_mgr; }

  bool import_json(const std::u16string& path, bool exact, ImportResult& out,
                   std::u16string& error);
  // TS 里 `import_resource` / `import_image_bitmap` / `import_array_buffer` 的 `exact` 是必填，
  // 只有 `import_json` / `import_xml` 有默认值 `true` ⇒ 端口照抄（这里给重载）。
  bool import_resource(const std::u16string& path, bool exact, ImportResult& out,
                       std::u16string& error);
  bool import_image_bitmap(const std::u16string& path, bool exact, ImportResult& out,
                           std::u16string& error);
  bool import_array_buffer(const std::u16string& path, bool exact, ImportResult& out,
                           std::u16string& error);
  bool import_xml(const std::u16string& path, bool exact, ImportResult& out,
                  std::u16string& error);

 private:
  // `this.find(paths, true).at(0) || {}`：给出命中的文件与来源标签（都没命中就是两个 `nullptr`
  // / `undefined`）。
  void find_first(const std::vector<std::u16string>& paths, IZipObject** file, Value& origin);

  ZipMgr* _zip_mgr = nullptr;
  IResourcesHost* _host = nullptr;
};

}
