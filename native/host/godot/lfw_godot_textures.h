// 资源路径 → `ImageTexture` 的缓存（Godot 宿主侧）。
//
// 这一层是 DESIGN「宿主负责图片解码」那一条的落地：核心只给**素材键 + 裁剪矩形**（契约 D2），
// 把键解析成资源路径、把路径读成字节、把字节解码成像素，全在宿主。字节来自
// `GodotWorldRenderer::read_resource`（已加载的数据包 → `--root` 根目录），解码交给引擎自己的
// PNG 解码器（`Image::load_png_from_buffer`）—— 这两条都不需要核心加缝。
//
// 失败的键**也缓存**（空引用）：一张取不到的图不该每帧重试一次磁盘。
#pragma once

#include <functional>
#include <map>
#include <string>

#include <godot_cpp/classes/image_texture.hpp>

namespace lfw::host {

class GodotTextureCache {
 public:
  // `load`：条目名（`/` 分隔）→ 字节。宿主注入（真源是数据包 / 根目录）。
  explicit GodotTextureCache(std::function<bool(const std::string&, std::string&)> load)
      : _load(std::move(load)) {}

  // 命中缓存或加载成功 ⇒ 非空；失败 ⇒ 空引用。
  godot::Ref<godot::ImageTexture> get(const std::string& path);

  long long loaded() const { return _loaded; }
  long long failed() const { return _failed; }
  const std::string& last_error() const { return _last_error; }

 private:
  std::function<bool(const std::string&, std::string&)> _load;
  std::map<std::string, godot::Ref<godot::ImageTexture>> _cache;
  long long _loaded = 0;
  long long _failed = 0;
  std::string _last_error;
};

}  // namespace lfw::host
