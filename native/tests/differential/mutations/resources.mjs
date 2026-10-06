// `Resources`（`src/LFW/Resources.ts`）+ `base/dedup.h`（`src/LFW/base/dedup.ts`）。
//
// 用例：`cases/resources/all.txt`（`zip` / `zfile` / `zval` / `zfail` / `add` / `netval` / `netfail` /
// `xmlparse` / `rjson`·`rres`·`rimg`·`rabuf`·`rxml`）。
//
// 有意不覆盖（不可观察、按构造等价或无法在台面上造出来）：
//   * `base/dedup.h` 的整个实现：TS 的 `deduped` 是**并发**去重（模块级 `Map<string, Promise>`），
//     同步端口没有交错 ⇒ 「同 key 共享」观察不到 ⇒ 端口是直通。连带 `dedup_key` 拼出来的键
//     字符串也在端口里不参与任何逻辑（台面只做顺序调用）；
//   * `if (file != nullptr && truthy(tag))` 里 `truthy(tag)` 那一半：`IZipResult.origin` 的格式是
//     `[zip.name]file.name`，**恒带方括号** ⇒ 只要有命中就一定真值 ⇒ 与只看 `file != nullptr` 等价；
//   * `find_first` 里 `hits.empty()` 之外的分支：`hits` 非空时端口只取第一个；
//   * 回退分支里 `out.origin = Value();`（把 `tag` 写回去）：回退时 `tag` 本来就是 `undefined`；
//   * `get_import_fallbacks` 对非字符串 `path` 抛出的失败面：端口的 `path` 是 `std::u16string`；
//   * `import_json` / `import_xml` 的 `exact` 默认值、`import_resource` 等三个必填版本：那是端口
//     的重载形状，台面每个 op 都显式给 `exact`；
//   * `IZipObject` 里 `uint8_array` / `blob`：本刀没有调用点；
//   * harness 层的脚本 op 与回显行：那是台面自己的输出。
export default {
  subject: "resources",
  cases: ["all"],
  mutations: [
    // ---------------------------------------------------------------- import_paths / find_first
    {
      note: "Resources: 非 exact 也不做回退扩展（原样把 path 当候选表）",
      file: "native/lfw/resources.cpp",
      from: `  if (exact) return std::vector<std::u16string>{path};`,
      to: `  if (true) return std::vector<std::u16string>{path};`,
    },
    {
      note: "Resources: 非 exact 的回退扩展拿不到东西（more 被清空）",
      file: "native/lfw/resources.cpp",
      from: `  loader::get_import_fallbacks(Value(path), more, suffix);
  return more;`,
      to: `  loader::get_import_fallbacks(Value(path), more, suffix);
  more.clear();
  return more;`,
    },
    {
      note: "Resources: find_first 用 exact=false 查（自己又扩展一次）",
      file: "native/lfw/resources.cpp",
      from: `  const std::vector<IZipResult> hits = _zip_mgr->find(paths, true);`,
      to: `  const std::vector<IZipResult> hits = _zip_mgr->find(paths, false);`,
    },
    {
      note: "Resources: find_first 取最后一个命中",
      file: "native/lfw/resources.cpp",
      from: `  *file = hits[0].file;
  origin = Value(hits[0].origin);`,
      to: `  *file = hits.back().file;
  origin = Value(hits.back().origin);`,
    },
    {
      note: "Resources: find_first 不判空",
      file: "native/lfw/resources.cpp",
      from: `  if (hits.empty()) {
    *file = nullptr;
    origin = Value();
    return;
  }`,
      to: `  if (false) {
    *file = nullptr;
    origin = Value();
    return;
  }`,
    },
    {
      note: "Resources: find_first 不给出 origin",
      file: "native/lfw/resources.cpp",
      from: `  *file = hits[0].file;
  origin = Value(hits[0].origin);`,
      to: `  *file = hits[0].file;
  origin = Value();`,
    },

    // ---------------------------------------------------------------- import_json
    {
      note: "Resources(import_json): 命中判断恒假（总是走宿主 Importer）",
      file: "native/lfw/resources.cpp",
      from: `    if (file != nullptr && truthy(tag)) {
      Value data;
      if (!file->json(data, error)) return false;`,
      to: `    if (false) {
      Value data;
      if (!file->json(data, error)) return false;`,
    },
    {
      note: "Resources(import_json): 命中判断恒真（没命中也要读文件）",
      file: "native/lfw/resources.cpp",
      from: `    if (file != nullptr && truthy(tag)) {
      Value data;
      if (!file->json(data, error)) return false;`,
      to: `    if (true) {
      Value data;
      if (!file->json(data, error)) return false;`,
    },
    {
      note: "Resources(import_json): 忽略 file.json() 的失败",
      file: "native/lfw/resources.cpp",
      from: `      if (!file->json(data, error)) return false;`,
      to: `      file->json(data, error);`,
    },
    {
      note: "Resources(import_json): 读的是文件的 text 而不是 json",
      file: "native/lfw/resources.cpp",
      from: `      if (!file->json(data, error)) return false;`,
      to: `      if (!file->text(data, error)) return false;`,
    },
    {
      note: "Resources(import_json): file 用常量",
      file: "native/lfw/resources.cpp",
      from: `      out.data = data;
      out.file = Value(file->name());
      out.origin = tag;
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
      to: `      out.data = data;
      out.file = Value(u"x");
      out.origin = tag;
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
    },
    {
      note: "Resources(import_json): file 用 paths[0] 而不是 file.name()",
      file: "native/lfw/resources.cpp",
      from: `      out.data = data;
      out.file = Value(file->name());
      out.origin = tag;
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
      to: `      out.data = data;
      out.file = paths.empty() ? Value() : Value(paths[0]);
      out.origin = tag;
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
    },
    {
      note: "Resources(import_json): 不写 origin",
      file: "native/lfw/resources.cpp",
      from: `      out.origin = tag;
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
      to: `      out.origin = Value();
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
    },
    {
      note: "Resources(import_json): 丢掉 data",
      file: "native/lfw/resources.cpp",
      from: `      out.data = data;
      out.file = Value(file->name());
      out.origin = tag;
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
      to: `      out.data = Value();
      out.file = Value(file->name());
      out.origin = tag;
      return true;
    }
    Value data;
    Value hit;
    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
    },
    {
      note: "Resources(import_json): 回退时忽略宿主失败",
      file: "native/lfw/resources.cpp",
      from: `    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
      to: `    _host->import_as_json(paths, data, hit, error);`,
    },
    {
      note: "Resources(import_json): 回退时调 import_as_text",
      file: "native/lfw/resources.cpp",
      from: `    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
      to: `    if (!_host->import_as_text(paths, data, hit, error)) return false;`,
    },
    {
      note: "Resources(import_json): 回退时 file 写死 undefined",
      file: "native/lfw/resources.cpp",
      from: `    out.data = data;
    out.file = hit;
    out.origin = Value();
    return true;
  });
}

bool Resources::import_resource`,
      to: `    out.data = data;
    out.file = Value();
    out.origin = Value();
    return true;
  });
}

bool Resources::import_resource`,
    },

    // ---------------------------------------------------------------- import_resource
    {
      note: "Resources(import_resource): 读的是 array_buffer 而不是 blob_url",
      file: "native/lfw/resources.cpp",
      from: `      if (!file->blob_url(data, error)) return false;`,
      to: `      if (!file->array_buffer(data, error)) return false;`,
    },
    {
      note: "Resources(import_resource): 回退时调 import_as_array_buffer",
      file: "native/lfw/resources.cpp",
      from: `    if (!_host->import_as_blob_url(paths, data, hit, error)) return false;`,
      to: `    if (!_host->import_as_array_buffer(paths, data, hit, error)) return false;`,
    },
    {
      note: "Resources(import_resource): 忽略 file.blob_url() 的失败",
      file: "native/lfw/resources.cpp",
      from: `      if (!file->blob_url(data, error)) return false;`,
      to: `      file->blob_url(data, error);`,
    },
    {
      note: "Resources(import_resource): 命中判断恒假",
      file: "native/lfw/resources.cpp",
      from: `    if (file != nullptr && truthy(tag)) {
      Value data;
      if (!file->blob_url(data, error)) return false;`,
      to: `    if (false) {
      Value data;
      if (!file->blob_url(data, error)) return false;`,
    },

    // ---------------------------------------------------------------- import_image_bitmap
    {
      note: "Resources(import_image_bitmap): 读的是 json 而不是 image_bitmap",
      file: "native/lfw/resources.cpp",
      from: `      if (!file->image_bitmap(data, error)) return false;`,
      to: `      if (!file->json(data, error)) return false;`,
    },
    {
      note: "Resources(import_image_bitmap): 回退时调 import_as_json",
      file: "native/lfw/resources.cpp",
      from: `    if (!_host->import_as_image_bitmap(paths, data, hit, error)) return false;`,
      to: `    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
    },
    {
      note: "Resources(import_image_bitmap): 回退时 file 用宿主命中 URL（TS 用的是 paths[0]）",
      file: "native/lfw/resources.cpp",
      from: `    out.data = data;
    out.file = paths.empty() ? Value() : Value(paths[0]);
    out.origin = Value();`,
      to: `    out.data = data;
    out.file = hit;
    out.origin = Value();`,
    },
    {
      note: "Resources(import_image_bitmap): 回退时 file 写死 paths[0] 之外（用 file.name）",
      file: "native/lfw/resources.cpp",
      from: `    out.data = data;
    out.file = paths.empty() ? Value() : Value(paths[0]);
    out.origin = Value();`,
      to: `    out.data = data;
    out.file = Value();
    out.origin = Value();`,
    },
    {
      note: "Resources(import_image_bitmap): 忽略 file.image_bitmap() 的失败",
      file: "native/lfw/resources.cpp",
      from: `      if (!file->image_bitmap(data, error)) return false;`,
      to: `      file->image_bitmap(data, error);`,
    },

    // ---------------------------------------------------------------- import_array_buffer
    {
      note: "Resources(import_array_buffer): 读的是 blob_url 而不是 array_buffer",
      file: "native/lfw/resources.cpp",
      from: `      if (!file->array_buffer(data, error)) return false;`,
      to: `      if (!file->blob_url(data, error)) return false;`,
    },
    {
      note: "Resources(import_array_buffer): 回退时调 import_as_blob_url",
      file: "native/lfw/resources.cpp",
      from: `    if (!_host->import_as_array_buffer(paths, data, hit, error)) return false;`,
      to: `    if (!_host->import_as_blob_url(paths, data, hit, error)) return false;`,
    },
    {
      note: "Resources(import_array_buffer): 忽略 file.array_buffer() 的失败",
      file: "native/lfw/resources.cpp",
      from: `      if (!file->array_buffer(data, error)) return false;`,
      to: `      file->array_buffer(data, error);`,
    },

    // ---------------------------------------------------------------- import_xml
    {
      note: "Resources(import_xml): 命中判断恒假（总是走宿主文本）",
      file: "native/lfw/resources.cpp",
      from: `    if (file != nullptr && truthy(tag)) {
      if (!file->text(text, error)) return false;`,
      to: `    if (false) {
      if (!file->text(text, error)) return false;`,
    },
    {
      note: "Resources(import_xml): 读的是 file.json 而不是 text",
      file: "native/lfw/resources.cpp",
      from: `      if (!file->text(text, error)) return false;`,
      to: `      if (!file->json(text, error)) return false;`,
    },
    {
      note: "Resources(import_xml): 回退时调 import_as_json",
      file: "native/lfw/resources.cpp",
      from: `      if (!_host->import_as_text(paths, text, hit, error)) return false;`,
      to: `      if (!_host->import_as_json(paths, text, hit, error)) return false;`,
    },
    {
      note: "Resources(import_xml): 忽略宿主文本的失败",
      file: "native/lfw/resources.cpp",
      from: `      if (!_host->import_as_text(paths, text, hit, error)) return false;`,
      to: `      _host->import_as_text(paths, text, hit, error);`,
    },
    {
      note: "Resources(import_xml): 忽略 XML.parse 的失败",
      file: "native/lfw/resources.cpp",
      from: `    if (!_host->xml_parse(text, root, error)) return false;`,
      to: `    _host->xml_parse(text, root, error);`,
    },
    {
      note: "Resources(import_xml): 不判解析结果假值（不再抛「failed to parse」）",
      file: "native/lfw/resources.cpp",
      from: `    if (!truthy(root)) {`,
      to: `    if (false) {`,
    },
    {
      note: "Resources(import_xml): 解析结果恒判失败",
      file: "native/lfw/resources.cpp",
      from: `    if (!truthy(root)) {`,
      to: `    if (true) {`,
    },
    {
      note: "Resources(import_xml): 失败文案多了个字符",
      file: "native/lfw/resources.cpp",
      from: `      error = u"[" + std::u16string(kTag) + u"::import_xml] failed to parse: " + path;`,
      to: `      error = u"[" + std::u16string(kTag) + u"::import_xml] failed to parse! " + path;`,
    },
    {
      note: "Resources(import_xml): 失败文案不带路径",
      file: "native/lfw/resources.cpp",
      from: `      error = u"[" + std::u16string(kTag) + u"::import_xml] failed to parse: " + path;`,
      to: `      error = u"[" + std::u16string(kTag) + u"::import_xml] failed to parse: ";`,
    },
    {
      note: "Resources(import_xml): 失败文案的 tag 写错",
      file: "native/lfw/resources.cpp",
      from: `      error = u"[" + std::u16string(kTag) + u"::import_xml] failed to parse: " + path;`,
      to: `      error = u"[" + std::u16string(u"Resource") + u"::import_xml] failed to parse: " + path;`,
    },
    {
      note: "Resources(import_xml): file 直接用 paths[0]",
      file: "native/lfw/resources.cpp",
      from: `    out.file = file_name_or_first_path(file, paths);`,
      to: `    out.file = paths.empty() ? Value() : Value(paths[0]);`,
    },
    {
      note: "Resources(import_xml): 命中时也不写 origin",
      file: "native/lfw/resources.cpp",
      from: `    out.data = root;
    out.file = file_name_or_first_path(file, paths);
    out.origin = tag;`,
      to: `    out.data = root;
    out.file = file_name_or_first_path(file, paths);
    out.origin = Value();`,
    },
    {
      note: "Resources(file_name_or_first_path): 不看 name 的假值（空名也照用）",
      file: "native/lfw/resources.cpp",
      from: `  if (file != nullptr && truthy(Value(file->name()))) return Value(file->name());`,
      to: `  if (file != nullptr) return Value(file->name());`,
    },
    {
      note: "Resources(file_name_or_first_path): 恒用 paths[0]",
      file: "native/lfw/resources.cpp",
      from: `  if (file != nullptr && truthy(Value(file->name()))) return Value(file->name());`,
      to: `  if (false) return Value(file->name());`,
    },

    // ---------------------------------------------------------------- 五个方法的宿主分发
    {
      note: "Resources(import_resource): 回退分支混进 import_json 的写法",
      file: "native/lfw/resources.cpp",
      from: `    Value data;
    Value hit;
    if (!_host->import_as_blob_url(paths, data, hit, error)) return false;`,
      to: `    Value data;
    Value hit;
    if (!_host->import_as_json(paths, data, hit, error)) return false;`,
    },
  ],
};
