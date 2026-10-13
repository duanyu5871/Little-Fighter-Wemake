#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/ditto/zip/i_zip.h"

namespace lfw::host {

// 目录当 zip 用：条目名 = 相对 `root` 的路径（`/` 分隔）。
// 只支持「已解包的包」——压缩包本体（中央目录 / DEFLATE）留到宿主 zip 那一刀。
class FsZipObject : public IZipObject {
 public:
  FsZipObject(std::string entry, std::filesystem::path path);

  const std::u16string& name() const override { return name_; }
  bool json(Value& out, std::u16string& error) override;
  bool text(Value& out, std::u16string& error) override;
  bool blob_url(Value& out, std::u16string& error) override;
  bool array_buffer(Value& out, std::u16string& error) override;
  bool image_bitmap(Value& out, std::u16string& error) override;

  const std::filesystem::path& path() const { return path_; }
  bool read(std::string& out, std::u16string& error) const;

 private:
  std::string entry_;
  std::u16string name_;
  std::filesystem::path path_;
};

class FsZip : public IZip {
 public:
  FsZip(std::u16string name, std::filesystem::path root);

  const std::u16string& name() const override { return name_; }
  IZipObject* file(const std::u16string& path) override;
  std::vector<IZipObject*> file_regex(const std::u16string& pattern) override;

  const std::filesystem::path& root() const { return root_; }
  size_t entry_count() const { return entries_.size(); }

 private:
  void index();
  IZipObject* object_for(const std::string& entry, const std::filesystem::path& path);

  std::u16string name_;
  std::filesystem::path root_;
  // 条目名（`/` 分隔、相对 root）按字典序 —— `file_regex` 的包内顺序就是它。
  std::vector<std::pair<std::string, std::filesystem::path>> entries_;
  std::map<std::string, FsZipObject*> by_entry_;
  std::vector<std::unique_ptr<FsZipObject>> objects_;
};

}
