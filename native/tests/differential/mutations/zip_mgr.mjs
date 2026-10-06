// `ZipMgr`（`src/LFW/ZipMgr.ts`）。
//
// 用例：`cases/zip_mgr/all.txt`（`zip` / `zfile` / `info` / `imd5` / `add` / `clear` / `dump` / `find`）。
//
// 有意不覆盖（不可观察、按构造等价或无法在台面上造出来）：
//   * `zips()` / `data_infos()` / `md5s()` 里 `out.reserve(_list.size())`：只影响扩容，不改结果；
//   * `find` 里 `loader::get_import_fallbacks` 的**失败面**：端口的 `paths` 已经是
//     `std::u16string` ⇒ 永远走不到「名字不是字符串」那条（TS `paths: string[]` 同理）；
//   * `IZip::file` 的抛错面与 `IZipObject` 除 `name` 以外的成员：真实 zip 是纯查表，
//     台面也没脚本化（记在 README 偏差表）；
//   * `ILoadedZip` 按值存与 TS 存引用的差别：宿主在 `add` 之后改结构体才会显现，台面不造；
//   * `md5s()` 里 `Value(std::u16string())` 的「空串」写成 `Value(u"")`：同一个值；
//   * harness 层的脚本 op 与 `dump`/`find` 回显行：那是台面自己的输出。
export default {
  subject: "zip_mgr",
  cases: ["all"],
  mutations: [
    // ---------------------------------------------------------------- 三个 getter
    {
      note: "ZipMgr: zips() 取错元素（一律取第一个）",
      file: "native/lfw/zip_mgr.cpp",
      from: `  for (const ILoadedZip& v : _list) out.push_back(v.zip);`,
      to: `  for (const ILoadedZip& v : _list) out.push_back(_list.front().zip);`,
    },
    {
      note: "ZipMgr: zips() 顺序反了",
      file: "native/lfw/zip_mgr.cpp",
      from: `  for (const ILoadedZip& v : _list) out.push_back(v.zip);`,
      to: `  for (const ILoadedZip& v : _list) out.insert(out.begin(), v.zip);`,
    },
    {
      note: "ZipMgr: data_infos() 取错元素（一律取第一个）",
      file: "native/lfw/zip_mgr.cpp",
      from: `  for (const ILoadedZip& v : _list) out.push_back(v.info);`,
      to: `  for (const ILoadedZip& v : _list) out.push_back(_list.front().info);`,
    },
    {
      note: "ZipMgr: data_infos() 顺序反了",
      file: "native/lfw/zip_mgr.cpp",
      from: `  for (const ILoadedZip& v : _list) out.push_back(v.info);`,
      to: `  for (const ILoadedZip& v : _list) out.insert(out.begin(), v.info);`,
    },
    {
      note: "ZipMgr: md5s() 顺序反了",
      file: "native/lfw/zip_mgr.cpp",
      from: `    out.push_back(is_nullish(v.info->md5) ? Value(std::u16string()) : v.info->md5);`,
      to: `    out.insert(out.begin(),
                    is_nullish(v.info->md5) ? Value(std::u16string()) : v.info->md5);`,
    },
    {
      note: "ZipMgr: md5s() 不管 nullish，原样返回（null 不落回空串）",
      file: "native/lfw/zip_mgr.cpp",
      from: `  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);`,
      to: `  return std::holds_alternative<std::monostate>(v);`,
    },
    {
      note: "ZipMgr: md5s() 的默认值写成 'x'",
      file: "native/lfw/zip_mgr.cpp",
      from: `    out.push_back(is_nullish(v.info->md5) ? Value(std::u16string()) : v.info->md5);`,
      to: `    out.push_back(is_nullish(v.info->md5) ? Value(u"x") : v.info->md5);`,
    },
    {
      note: "ZipMgr: md5s() 读错字段（version）",
      file: "native/lfw/zip_mgr.cpp",
      from: `    out.push_back(is_nullish(v.info->md5) ? Value(std::u16string()) : v.info->md5);`,
      to: `    out.push_back(is_nullish(v.info->version) ? Value(std::u16string()) : v.info->version);`,
    },
    {
      note: "ZipMgr: length 恒为 0",
      file: "native/lfw/zip_mgr.h",
      from: `  size_t length() const { return _list.size(); }`,
      to: `  size_t length() const { return 0; }`,
    },
    {
      note: "ZipMgr: length 多算一个",
      file: "native/lfw/zip_mgr.h",
      from: `  size_t length() const { return _list.size(); }`,
      to: `  size_t length() const { return _list.size() + 1; }`,
    },

    // ---------------------------------------------------------------- add / clear
    {
      note: "ZipMgr: add 用 push_back（后加载不再优先）",
      file: "native/lfw/zip_mgr.cpp",
      from: `void ZipMgr::add(const ILoadedZip& zip) { _list.insert(_list.begin(), zip); }`,
      to: `void ZipMgr::add(const ILoadedZip& zip) { _list.push_back(zip); }`,
    },
    {
      note: "ZipMgr: add 空实现",
      file: "native/lfw/zip_mgr.cpp",
      from: `void ZipMgr::add(const ILoadedZip& zip) { _list.insert(_list.begin(), zip); }`,
      to: `void ZipMgr::add(const ILoadedZip&) {}`,
    },
    {
      note: "ZipMgr: clear 空实现",
      file: "native/lfw/zip_mgr.cpp",
      from: `void ZipMgr::clear() { _list.clear(); }`,
      to: `void ZipMgr::clear() {}`,
    },

    // ---------------------------------------------------------------- find：候选名表
    {
      note: "ZipMgr: find 的 exact 判断反了",
      file: "native/lfw/zip_mgr.cpp",
      from: `  if (!exact) {`,
      to: `  if (exact) {`,
    },
    {
      note: "ZipMgr: find 的 exact 分支返回空候选表",
      file: "native/lfw/zip_mgr.cpp",
      from: `    candidates = paths;`,
      to: `    candidates.clear();`,
    },
    {
      note: "ZipMgr: find 的候选名不去重",
      file: "native/lfw/zip_mgr.cpp",
      from: `  if (seen.insert(path).second) out.push_back(path);`,
      to: `  out.push_back(path);`,
    },
    {
      note: "ZipMgr: find 不先把原名放进候选表（只有回退名）",
      file: "native/lfw/zip_mgr.cpp",
      from: `    for (const std::u16string& path : paths) push_unique(candidates, seen, path);`,
      to: `    for (const std::u16string& path : paths) { (void)path; }`,
    },
    {
      note: "ZipMgr: find 的原名候选表反序",
      file: "native/lfw/zip_mgr.cpp",
      from: `    for (const std::u16string& path : paths) push_unique(candidates, seen, path);`,
      to: `    for (auto it = paths.rbegin(); it != paths.rend(); ++it)
      push_unique(candidates, seen, *it);`,
    },
    {
      note: "ZipMgr: find 的原名候选表不去重",
      file: "native/lfw/zip_mgr.cpp",
      from: `    for (const std::u16string& path : paths) push_unique(candidates, seen, path);`,
      to: `    for (const std::u16string& path : paths) candidates.push_back(path);`,
    },
    {
      note: "ZipMgr: find 的回退名候选表反序",
      file: "native/lfw/zip_mgr.cpp",
      from: `      for (const std::u16string& fallback : more) push_unique(candidates, seen, fallback);`,
      to: `      for (auto it = more.rbegin(); it != more.rend(); ++it)
        push_unique(candidates, seen, *it);`,
    },
    {
      note: "ZipMgr: find 的回退名候选表不去重",
      file: "native/lfw/zip_mgr.cpp",
      from: `      for (const std::u16string& fallback : more) push_unique(candidates, seen, fallback);`,
      to: `      for (const std::u16string& fallback : more) candidates.push_back(fallback);`,
    },
    {
      note: "ZipMgr: find 的 exact 分支也去重",
      file: "native/lfw/zip_mgr.cpp",
      from: `    candidates = paths;`,
      to: `    candidates = paths;
    std::set<std::u16string> uniq{candidates.begin(), candidates.end()};
    candidates.assign(uniq.begin(), uniq.end());`,
    },
    {
      note: "ZipMgr: find 不扩展回退名",
      file: "native/lfw/zip_mgr.cpp",
      from: `      loader::get_import_fallbacks(Value(path), more, suffix);`,
      to: `      loader::get_import_fallbacks(Value(path), more, suffix);
      more.clear();`,
    },
    {
      note: "ZipMgr: find 把回退名写成原名",
      file: "native/lfw/zip_mgr.cpp",
      from: `      for (const std::u16string& fallback : more) push_unique(candidates, seen, fallback);`,
      to: `      for (const std::u16string& fallback : more) push_unique(candidates, seen, path);`,
    },

    // ---------------------------------------------------------------- find：收集结果
    {
      note: "ZipMgr: find 只查第一个数据包",
      file: "native/lfw/zip_mgr.cpp",
      from: `  for (const ILoadedZip& loaded : _list) {`,
      to: `  for (const ILoadedZip& loaded : std::vector<ILoadedZip>{_list.front()}) {`,
    },
    {
      note: "ZipMgr: find 的外层数据包与内层候选名调换",
      file: "native/lfw/zip_mgr.cpp",
      from: `  for (const ILoadedZip& loaded : _list) {
    for (const std::u16string& path : candidates) {`,
      to: `  for (const std::u16string& path : candidates) {
    for (const ILoadedZip& loaded : _list) {`,
    },
    {
      note: "ZipMgr: find 用数据包名当查询路径",
      file: "native/lfw/zip_mgr.cpp",
      from: `      IZipObject* file = loaded.zip->file(path);`,
      to: `      IZipObject* file = loaded.zip->file(loaded.zip->name());`,
    },
    {
      note: "ZipMgr: find 不过滤未命中",
      file: "native/lfw/zip_mgr.cpp",
      from: `      if (file == nullptr) continue;`,
      to: `      if (file != nullptr) continue;`,
    },
    {
      note: "ZipMgr: find 的结果顺序反了",
      file: "native/lfw/zip_mgr.cpp",
      from: `      ret.push_back(
          IZipResult{u"[" + loaded.zip->name() + u"]" + file->name(), file, loaded.zip});`,
      to: `      ret.insert(ret.begin(),
                 IZipResult{u"[" + loaded.zip->name() + u"]" + file->name(), file, loaded.zip});`,
    },
    {
      note: "ZipMgr: find 的 origin 去掉方括号",
      file: "native/lfw/zip_mgr.cpp",
      from: `          IZipResult{u"[" + loaded.zip->name() + u"]" + file->name(), file, loaded.zip});`,
      to: `          IZipResult{loaded.zip->name() + file->name(), file, loaded.zip});`,
    },
    {
      note: "ZipMgr: find 的 origin 用候选名而不是文件名",
      file: "native/lfw/zip_mgr.cpp",
      from: `          IZipResult{u"[" + loaded.zip->name() + u"]" + file->name(), file, loaded.zip});`,
      to: `          IZipResult{u"[" + loaded.zip->name() + u"]" + path, file, loaded.zip});`,
    },
    {
      note: "ZipMgr: find 的 origin 把 zip 名与文件名调换",
      file: "native/lfw/zip_mgr.cpp",
      from: `          IZipResult{u"[" + loaded.zip->name() + u"]" + file->name(), file, loaded.zip});`,
      to: `          IZipResult{u"[" + file->name() + u"]" + loaded.zip->name(), file, loaded.zip});`,
    },
    {
      note: "ZipMgr: find 的结果里 zip/file 调换",
      file: "native/lfw/zip_mgr.cpp",
      from: `          IZipResult{u"[" + loaded.zip->name() + u"]" + file->name(), file, loaded.zip});`,
      to: `          IZipResult{u"[" + loaded.zip->name() + u"]" + file->name(), nullptr, loaded.zip});`,
    },
  ],
};
