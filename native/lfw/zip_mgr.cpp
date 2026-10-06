#include "lfw/zip_mgr.h"

#include <set>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/loader/get_import_fallbacks.h"

namespace lfw {

namespace {

// `temp.add(path)`（JS `Set`）：只记首次出现，保持插入序。
void push_unique(std::vector<std::u16string>& out, std::set<std::u16string>& seen,
                 const std::u16string& path) {
  if (seen.insert(path).second) out.push_back(path);
}

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

}

std::vector<IZip*> ZipMgr::zips() const {
  std::vector<IZip*> out;
  out.reserve(_list.size());
  for (const ILoadedZip& v : _list) out.push_back(v.zip);
  return out;
}

std::vector<Value> ZipMgr::md5s() const {
  std::vector<Value> out;
  out.reserve(_list.size());
  for (const ILoadedZip& v : _list) {
    // TS `v.info.md5 ?? ''`：只有 `null` / `undefined` 落回空串。
    out.push_back(is_nullish(v.info->md5) ? Value(std::u16string()) : v.info->md5);
  }
  return out;
}

std::vector<IDataInfo*> ZipMgr::data_infos() const {
  std::vector<IDataInfo*> out;
  out.reserve(_list.size());
  for (const ILoadedZip& v : _list) out.push_back(v.info);
  return out;
}

void ZipMgr::add(const ILoadedZip& zip) { _list.insert(_list.begin(), zip); }

void ZipMgr::clear() { _list.clear(); }

std::vector<IZipResult> ZipMgr::find(const std::vector<std::u16string>& paths, bool exact) {
  std::vector<std::u16string> candidates;
  if (!exact) {
    std::set<std::u16string> seen;
    for (const std::u16string& path : paths) push_unique(candidates, seen, path);
    for (const std::u16string& path : paths) {
      std::vector<std::u16string> more;
      std::u16string suffix;
      // `paths` 在端口里已经是字符串 ⇒ 不会走 `get_import_fallbacks` 的「名字不是字符串」失败面。
      loader::get_import_fallbacks(Value(path), more, suffix);
      for (const std::u16string& fallback : more) push_unique(candidates, seen, fallback);
    }
  } else {
    candidates = paths;
  }

  std::vector<IZipResult> ret;
  for (const ILoadedZip& loaded : _list) {
    for (const std::u16string& path : candidates) {
      IZipObject* file = loaded.zip->file(path);
      if (file == nullptr) continue;
      ret.push_back(
          IZipResult{u"[" + loaded.zip->name() + u"]" + file->name(), file, loaded.zip});
    }
  }
  return ret;
}

}
