#include "host/fs_zip.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>

#include "lfw/core/json.h"
#include "lfw/core/json5.h"
#include "lfw/core/js_num.h"
#include "lfw/utils/utf8.h"

namespace lfw::host {

namespace {

std::u16string as_u16(const std::string& s) { return decode_utf8(std::vector<uint8_t>(s.begin(), s.end())); }

std::string as_u8(const std::u16string& s) {
  const std::vector<uint8_t> buf = encode_utf8(s);
  return std::string(buf.begin(), buf.end());
}

// 条目名与磁盘路径都按「`/` 分隔」写；Windows 的 `std::filesystem::path` 也认 `/`。
std::string normalize_entry(const std::string& raw) {
  std::string out;
  out.reserve(raw.size());
  for (const char c : raw) out.push_back(c == '\\' ? '/' : c);
  while (!out.empty() && out.front() == '/') out.erase(out.begin());
  return out;
}

}  // namespace

FsZipObject::FsZipObject(std::string entry, std::filesystem::path path)
    : entry_(std::move(entry)), name_(as_u16(entry_)), path_(std::move(path)) {}

bool FsZipObject::read(std::string& out, std::u16string& error) const {
  std::ifstream in(path_, std::ios::binary);
  if (!in) {
    error = u"[FsZipObject::read] cannot open: " + name_;
    return false;
  }
  out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  if (!in.good() && !in.eof()) {
    error = u"[FsZipObject::read] read failed: " + name_;
    return false;
  }
  return true;
}

bool FsZipObject::json(Value& out, std::u16string& error) {
  std::string raw;
  if (!read(raw, error)) return false;
  const std::u16string text = as_u16(raw);
  // 包里的 `*.json5` 与 `*.json` 走同一条路：JSON5 是 JSON 的超集，失败再退回严格 JSON。
  const Json5Result r = json5_parse(text);
  if (r.ok) {
    out = r.value;
    return true;
  }
  const std::optional<Value> strict = json_parse(text);
  if (strict.has_value()) {
    out = *strict;
    return true;
  }
  error = u"[FsZipObject::json] parse failed: " + name_ + u" (" + r.error + u")";
  return false;
}

bool FsZipObject::text(Value& out, std::u16string& error) {
  std::string raw;
  if (!read(raw, error)) return false;
  out = Value(as_u16(raw));
  return true;
}

bool FsZipObject::blob_url(Value& out, std::u16string& error) {
  // TS 给的是 `BlobUrl`（平台对象 URL）。宿主没有 blob 层 ⇒ 直接给磁盘路径（记 README 偏差表）。
  (void)error;
  out = Value(as_u16(entry_));
  return true;
}

bool FsZipObject::array_buffer(Value& out, std::u16string& error) {
  std::string raw;
  if (!read(raw, error)) return false;
  auto arr = std::make_shared<Array>();
  for (const unsigned char c : raw) arr->push_back(Value(static_cast<double>(c)));
  out = Value(std::move(arr));
  return true;
}

bool FsZipObject::image_bitmap(Value& out, std::u16string& error) {
  (void)out;
  // 宿主没有位图解码（渲染层在平台侧）⇒ 明确拒绝，调用方按 TS 的 reject 分支走。
  error = u"[FsZipObject::image_bitmap] not supported by the headless host: " + name_;
  return false;
}

FsZip::FsZip(std::u16string name, std::filesystem::path root)
    : name_(std::move(name)), root_(std::move(root)) {
  index();
}

void FsZip::index() {
  std::error_code ec;
  if (!std::filesystem::is_directory(root_, ec)) return;
  for (std::filesystem::recursive_directory_iterator it(root_, ec), end; it != end; it.increment(ec)) {
    if (ec) break;
    if (!it->is_regular_file(ec)) continue;
    const std::filesystem::path& p = it->path();
    std::error_code rel_ec;
    const std::filesystem::path rel = std::filesystem::relative(p, root_, rel_ec);
    if (rel_ec) continue;
    entries_.emplace_back(normalize_entry(rel.generic_string()), p);
  }
  std::sort(entries_.begin(), entries_.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
}

IZipObject* FsZip::object_for(const std::string& entry, const std::filesystem::path& path) {
  const auto found = by_entry_.find(entry);
  if (found != by_entry_.end()) return found->second;
  objects_.push_back(std::make_unique<FsZipObject>(entry, path));
  FsZipObject* const obj = objects_.back().get();
  by_entry_.emplace(entry, obj);
  return obj;
}

IZipObject* FsZip::file(const std::u16string& path) {
  const std::string entry = normalize_entry(as_u8(path));
  const auto found = by_entry_.find(entry);
  if (found != by_entry_.end()) return found->second;
  // 不在索引里也允许直查：条目名大小写/斜杠差异时按磁盘再说一次。
  std::error_code ec;
  const std::filesystem::path full = root_ / std::filesystem::path(entry);
  if (!std::filesystem::is_regular_file(full, ec)) return nullptr;
  std::error_code rel_ec;
  const std::filesystem::path rel = std::filesystem::relative(full, root_, rel_ec);
  return object_for(rel_ec ? entry : normalize_entry(rel.generic_string()), full);
}

std::vector<IZipObject*> FsZip::file_regex(const std::u16string& pattern) {
  std::vector<IZipObject*> out;
  for (const auto& kv : entries_) {
    if (!zip_name_matches(as_u16(kv.first), pattern)) continue;
    out.push_back(object_for(kv.first, kv.second));
  }
  return out;
}

}
