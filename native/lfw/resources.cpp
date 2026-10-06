#include "lfw/resources.h"

#include <string>
#include <vector>

#include "lfw/base/dedup.h"
#include "lfw/core/value.h"
#include "lfw/loader/get_import_fallbacks.h"

namespace lfw {

namespace {

// TS：`const paths = exact ? [path] : get_import_fallbacks(path)[0];`
//
// `get_import_fallbacks` 在 TS 里对非字符串 `path` 会抛（`path.endsWith`）；端口的 `path` 已经是
// `std::u16string` ⇒ 走不到那条失败面。
std::vector<std::u16string> import_paths(const std::u16string& path, bool exact) {
  if (exact) return std::vector<std::u16string>{path};
  std::vector<std::u16string> more;
  std::u16string suffix;
  loader::get_import_fallbacks(Value(path), more, suffix);
  return more;
}

// TS `file?.name || paths[0]`：`file` 缺席或 `name` 是假值（空串）时退回 `paths[0]`；
// `paths` 为空时 TS 给 `undefined`。
Value file_name_or_first_path(IZipObject* file, const std::vector<std::u16string>& paths) {
  if (file != nullptr && truthy(Value(file->name()))) return Value(file->name());
  if (paths.empty()) return Value();
  return Value(paths[0]);
}

// TS 模板字面量 `` `${Resources.TAG}.json.${path}.${exact}` ``（`${true}` 就是 `"true"`）。
// 端口只用它当 `deduped` 的 key，而同步端口不参与去重逻辑（见 `base/dedup.h`）。
std::u16string dedup_key(const char16_t* kind, const std::u16string& path, bool exact) {
  return std::u16string(Resources::kTag) + u"." + kind + u"." + path + u"." +
         (exact ? u"true" : u"false");
}

}

Resources::Resources(ZipMgr* zip_mgr, IResourcesHost* host) : _zip_mgr(zip_mgr), _host(host) {}

void Resources::find_first(const std::vector<std::u16string>& paths, IZipObject** file,
                           Value& origin) {
  // `this.find(paths, true).at(0) || {}` —— `exact` 恒为 **true**：备选名扩展在上一步的
  // `import_paths` 里已经做过了。
  const std::vector<IZipResult> hits = _zip_mgr->find(paths, true);
  if (hits.empty()) {
    *file = nullptr;
    origin = Value();
    return;
  }
  *file = hits[0].file;
  origin = Value(hits[0].origin);
}

bool Resources::import_json(const std::u16string& path, bool exact, ImportResult& out,
                            std::u16string& error) {
  return deduped(dedup_key(u"json", path, exact), [&]() -> bool {
    const std::vector<std::u16string> paths = import_paths(path, exact);
    IZipObject* file = nullptr;
    Value tag;
    find_first(paths, &file, tag);
    if (file != nullptr && truthy(tag)) {
      Value data;
      if (!file->json(data, error)) return false;
      out.data = data;
      out.file = Value(file->name());
      out.origin = tag;
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_json(paths, data, hit, error)) return false;
    out.data = data;
    out.file = hit;
    out.origin = Value();
    return true;
  });
}

bool Resources::import_resource(const std::u16string& path, bool exact, ImportResult& out,
                                std::u16string& error) {
  return deduped(dedup_key(u"resource", path, exact), [&]() -> bool {
    const std::vector<std::u16string> paths = import_paths(path, exact);
    IZipObject* file = nullptr;
    Value tag;
    find_first(paths, &file, tag);
    if (file != nullptr && truthy(tag)) {
      Value data;
      if (!file->blob_url(data, error)) return false;
      out.data = data;
      out.file = Value(file->name());
      out.origin = tag;
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_blob_url(paths, data, hit, error)) return false;
    out.data = data;
    out.file = hit;
    out.origin = Value();
    return true;
  });
}

bool Resources::import_image_bitmap(const std::u16string& path, bool exact, ImportResult& out,
                                    std::u16string& error) {
  return deduped(dedup_key(u"image_bitmap", path, exact), [&]() -> bool {
    const std::vector<std::u16string> paths = import_paths(path, exact);
    IZipObject* file = nullptr;
    Value tag;
    find_first(paths, &file, tag);
    if (file != nullptr && truthy(tag)) {
      Value data;
      if (!file->image_bitmap(data, error)) return false;
      out.data = data;
      out.file = Value(file->name());
      out.origin = tag;
      return true;
    }
    // TS：`const [data] = await Importer.import_as_image_bitmap(paths)`，然后
    // `return { data, file: paths[0] }` —— 注意这里用的是 `paths[0]`，不是宿主给的命中 URL。
    Value data;
    Value hit;
    if (!_host->import_as_image_bitmap(paths, data, hit, error)) return false;
    out.data = data;
    out.file = paths.empty() ? Value() : Value(paths[0]);
    out.origin = Value();
    return true;
  });
}

bool Resources::import_array_buffer(const std::u16string& path, bool exact, ImportResult& out,
                                    std::u16string& error) {
  return deduped(dedup_key(u"array_buffer", path, exact), [&]() -> bool {
    const std::vector<std::u16string> paths = import_paths(path, exact);
    IZipObject* file = nullptr;
    Value tag;
    find_first(paths, &file, tag);
    if (file != nullptr && truthy(tag)) {
      Value data;
      if (!file->array_buffer(data, error)) return false;
      out.data = data;
      out.file = Value(file->name());
      out.origin = tag;
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_array_buffer(paths, data, hit, error)) return false;
    out.data = data;
    out.file = hit;
    out.origin = Value();
    return true;
  });
}

bool Resources::import_xml(const std::u16string& path, bool exact, ImportResult& out,
                           std::u16string& error) {
  return deduped(dedup_key(u"xml", path, exact), [&]() -> bool {
    const std::vector<std::u16string> paths = import_paths(path, exact);
    IZipObject* file = nullptr;
    Value tag;
    find_first(paths, &file, tag);
    Value text;
    if (file != nullptr && truthy(tag)) {
      if (!file->text(text, error)) return false;
    } else {
      Value hit;
      if (!_host->import_as_text(paths, text, hit, error)) return false;
    }
    Value root;
    if (!_host->xml_parse(text, root, error)) return false;
    if (!truthy(root)) {
      error = u"[" + std::u16string(kTag) + u"::import_xml] failed to parse: " + path;
      return false;
    }
    out.data = root;
    out.file = file_name_or_first_path(file, paths);
    out.origin = tag;
    return true;
  });
}

}
