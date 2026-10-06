#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "lfw/defines/i_data_info.h"
#include "lfw/ditto/zip/i_zip.h"
#include "lfw/ditto/zip/i_zip_object.h"

namespace lfw {

// TS `IZipResult`：一次命中的（文件, 数据包, 来源标签）。
struct IZipResult {
  std::u16string origin;
  IZipObject* file = nullptr;
  IZip* zip = nullptr;
};

// TS `ILoadedZip`：一个已加载数据包 + 它的信息表。
// 端口按**值**存进列表（TS 存的是对象引用）⇒ 宿主在 `add` 之后改自己那份结构体，端口里看不出来；
// 台面不造这种情形（真实调用点 `LFW.ts` 也是现造现加）。
struct ILoadedZip {
  IZip* zip = nullptr;
  IDataInfo* info = nullptr;
};

// TS `ZipMgr`。
class ZipMgr {
 public:
  size_t length() const { return _list.size(); }
  // TS `get all()`：返回内部数组本身（宿主改它等于改列表）。
  const std::vector<ILoadedZip>& all() const { return _list; }
  std::vector<IZip*> zips() const;
  // TS `get md5s()` 声明是 `string[]`，但实际会原样给出 `info.md5`（只把 `null` / `undefined`
  // 落回空串）⇒ 端口同样给 `Value`，不丢信息。
  std::vector<Value> md5s() const;
  std::vector<IDataInfo*> data_infos() const;
  // TS `add`：`list.unshift(zip)` ⇒ 后加载的排在前面。
  void add(const ILoadedZip& zip);
  // TS `clear`：`list.length = 0`。
  void clear();
  // TS `find(paths, exact)`：`exact` 为 false 时先对每个路径做回退扩展（`get_import_fallbacks`）
  // 并按「先原名后回退名、各自去重保序」拼成候选表，再按「数据包（后加载优先）× 候选名」
  // 两重循环收集命中。
  std::vector<IZipResult> find(const std::vector<std::u16string>& paths, bool exact);

 private:
  std::vector<ILoadedZip> _list;
};

}
