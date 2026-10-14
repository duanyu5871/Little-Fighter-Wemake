#include "host/godot/lfw_godot_textures.h"

#include <cstring>
#include <utility>

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace lfw::host {

namespace {

godot::PackedByteArray to_bytes(const std::string& raw) {
  godot::PackedByteArray out;
  if (raw.empty()) return out;
  out.resize(static_cast<int64_t>(raw.size()));
  std::memcpy(out.ptrw(), raw.data(), raw.size());
  return out;
}

}  // namespace

godot::Ref<godot::ImageTexture> GodotTextureCache::get(const std::string& path) {
  if (path.empty()) return {};
  const auto found = _cache.find(path);
  if (found != _cache.end()) return found->second;

  godot::Ref<godot::ImageTexture> tex;
  std::string raw;
  if (!_load(path, raw)) {
    ++_failed;
    _last_error = "read failed: " + path;
  } else {
    godot::Ref<godot::Image> image;
    image.instantiate();
    // 引擎自己的解码器：只吃 PNG（包里的图片全是 PNG，webp 一张都没有）。
    const godot::Error err = image->load_png_from_buffer(to_bytes(raw));
    if (err != godot::OK) {
      ++_failed;
      _last_error = "png decode failed (" + std::to_string(static_cast<int>(err)) + "): " + path;
    } else {
      tex = godot::ImageTexture::create_from_image(image);
      if (tex.is_null()) {
        ++_failed;
        _last_error = "texture create failed: " + path;
      } else {
        ++_loaded;
      }
    }
  }
  _cache.emplace(path, tex);
  return tex;
}

}  // namespace lfw::host
