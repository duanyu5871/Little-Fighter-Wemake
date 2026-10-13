#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lfw/base/clock.h"
#include "lfw/ditto/xml/tool_xml.h"
#include "lfw/lfw.h"
#include "lfw/world.h"

namespace lfw::host {

// 手动时钟：时间由 app 推进（`advance`）。`add` 注册的回调**不实现**（返回 0 = 没有句柄）
// ⇒ `Ticker` 自己不会走，帧由 app 显式 `World::update_once(dt)` 推 —— 可复现，也不依赖
// 平台的 requestAnimationFrame / setInterval。
class ManualClock : public IClock {
 public:
  double now() const override { return _t; }
  int add(std::function<void()>) override { return 0; }
  void del(int) override {}
  bool hidden() const override { return false; }
  void advance(double dt) { _t += dt; }
  void set(double t) { _t = t; }

 private:
  double _t = 0;
};

class NoopTimeout : public ITimeout {
 public:
  int add(std::function<void()>, double) override { return 0; }
  void del(int) override {}
};

// `Ditto.WorldRender` 那一面：headless 不做任何绘制，但实体进出世界时要记账（便于统计）。
class NullWorldRenderer : public IWorldRenderer {
 public:
  void add_entity(Entity&) override { ++added; }
  void del_entity(Entity&) override { ++removed; }
  void render(double) override { ++renders; }
  void dispose() override {}

  long long added = 0;
  long long removed = 0;
  long long renders = 0;
};

// headless 宿主：`ILfwHost` 的最小可用实现。
//   * 资源读盘：`--root` 下的目录树（`builtin_data/...`）与 `--zip` 给的「已解包包」目录；
//   * 渲染 / 音频 / 指针 / UI 图片：空实现（会计数，结束后打印一行统计）；
//   * 存档（`Ditto.Cache`）：内存态，进程退出即丢；
//   * `md5`：**不是真 MD5**，只保证「同输入同输出、不同输入基本不同」（给 `ui_load_img` 拼
//     key 用；真 MD5 与 zip 那一刀一起做）。
class HeadlessHost : public ILfwHost {
 public:
  HeadlessHost(std::vector<std::filesystem::path> roots, bool dev);

  ManualClock& clock() { return _clock; }
  NoopTimeout& timeout() { return _timeout; }
  NullWorldRenderer& renderer() { return _renderer; }

  // 未实现/被忽略的宿主面计数（结束时的统计行）。
  long long ignored_images() const { return _ignored_images; }
  int warn_count() const { return _warns; }
  int error_count() const { return _errors; }
  const std::vector<std::u16string>& last_errors() const { return _last_errors; }

  // `--root` 里第一个真的存在的文件（给 import_as_* 与宿主自己的诊断用）。
  std::filesystem::path resolve(const std::u16string& path) const;

  // ---- ILfwHost ----
  void sounds_init(LFW&) override {}
  void images_init(LFW&) override {}
  void keyboard_init(LFW&) override {}
  void pointings_init(LFW&) override {}
  void keyboard_add_callback(LFW&) override {}
  void pointings_add_ui_input(LFW&) override {}
  void keyboard_callback_add(LFW&, LfwKeyboardCallback* cb) override;
  void keyboard_callback_del(LFW&, LfwKeyboardCallback* cb) override;
  void pointings_callback_add(LFW&, LfwPointingsCallback* cb) override;
  void pointings_callback_del(LFW&, LfwPointingsCallback* cb) override;
  void cache_forget(const std::u16string& type, double version) override;
  void zip_forget_stored(const std::u16string& type, double version) override;
  IWorldRenderer* create_world_renderer(LFW&) override { return &_renderer; }
  void load_img(const std::u16string& path) override;
  bool ui_image_load(const std::u16string& img_key, const Value& path, const Value& ops, Value& out,
                     std::u16string& error) override;
  void ui_image_pin(const std::u16string& img_key) override;
  std::u16string md5(const std::u16string& text) override;
  bool dev() const override { return _dev; }
  double now() override { return _clock.now(); }
  void warn(const std::vector<Value>& args) override;
  void error(const std::vector<Value>& args) override;
  void log(const std::vector<Value>& args) override;
  void debug_msg(const std::vector<Value>& args) override;
  std::function<void()> sounds_play_bgm(const Value& music) override;
  void sounds_stop_bgm() override {}
  void sounds_play(const Value& path, const Value& x, const Value& y, const Value& z) override;
  void sounds_play_preset(const Value& name, const Value& x, const Value& y,
                          const Value& z) override;
  void sounds_play_with_load(const Value& path) override;
  void sounds_load(const Value& name, const Value& src) override;
  std::u16string sounds_bgm() override { return _bgm; }
  bool sounds_is_random() override { return _bgm_random; }
  double sounds_volume() override { return _volume; }
  void sounds_set_volume(double v) override { _volume = v; }
  double sounds_bgm_volume() override { return _bgm_volume; }
  void sounds_set_bgm_volume(double v) override { _bgm_volume = v; }
  double sounds_sound_volume() override { return _sound_volume; }
  void sounds_set_sound_volume(double v) override { _sound_volume = v; }
  bool sounds_bgm_muted() override { return _bgm_muted; }
  void sounds_set_bgm_muted(bool v) override { _bgm_muted = v; }
  bool sounds_sound_muted() override { return _sound_muted; }
  void sounds_set_sound_muted(bool v) override { _sound_muted = v; }
  void sounds_callback_add(LFW&, LfwSoundsCallback* cb) override;
  void sounds_callback_del(LFW&, LfwSoundsCallback* cb) override;
  void sounds_dispose() override {}
  void keyboard_dispose() override {}
  void pointings_dispose() override {}
  Value measure_text(const Value& text, const Value& style) override;
  void player_cache_get(const std::u16string& name, PlayerInfoCacheEntry& out) override;
  bool player_cache_del(const std::u16string& name, std::u16string& error) override;
  void player_cache_put(const PlayerInfoCachePut& data) override;
  bool import_as_json(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                      std::u16string& error) override;
  bool import_as_blob_url(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                          std::u16string& error) override;
  bool import_as_array_buffer(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                              std::u16string& error) override;
  bool import_as_image_bitmap(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                              std::u16string& error) override;
  bool import_as_text(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                      std::u16string& error) override;
  bool xml_parse(const Value& text, Value& marker, std::shared_ptr<IXMLElement>& root,
                 std::u16string& error) override;
  void lang_apply(LFW& lfw, const std::u16string& lang, const std::u16string& prev) override;
  Value zip_get_stored(const std::u16string& zip_url, const std::u16string& md5) override;
  bool zip_read_blob(const std::u16string& name, const Value& blob, const Value& md5, IZip*& out,
                     std::u16string& error) override;
  bool zip_read_buf(const std::u16string& name, const Value& data, IZip*& out,
                    std::u16string& error) override;
  bool zip_download(const std::u16string& zip_url, const Value& md5, LFW& lfw, DownloadedZip& out,
                    std::u16string& error) override;
  Value zip_cache_get(const std::u16string& name) override;
  void zip_cache_del(const std::u16string& name, const std::u16string& version) override;
  void zip_cache_put(const Value& entry) override;

  // 按键 / 指针的回调列表（app 可以喂事件；M1 只用来保证注册/注销不成漏）。
  void emit_key_down(LfwKeyEvent& e);
  void emit_key_up(LfwKeyEvent& e);

 private:
  bool read_file(const std::filesystem::path& path, std::string& out, std::u16string& error) const;

  std::vector<std::filesystem::path> _roots;
  bool _dev = false;
  ManualClock _clock;
  NoopTimeout _timeout;
  NullWorldRenderer _renderer;
  ToolXML _xml;
  std::vector<LfwKeyboardCallback*> _kb_callbacks;
  std::vector<LfwPointingsCallback*> _pt_callbacks;
  std::vector<LfwSoundsCallback*> _sd_callbacks;
  std::map<std::u16string, PlayerInfoCachePut> _player_cache;
  std::map<std::u16string, Value> _zip_cache;
  std::u16string _bgm;
  bool _bgm_random = false;
  bool _bgm_muted = false;
  bool _sound_muted = false;
  double _volume = 1.0;
  double _bgm_volume = 1.0;
  double _sound_volume = 1.0;
  long long _ignored_images = 0;
  int _warns = 0;
  int _errors = 0;
  int _logs = 0;
  std::vector<std::u16string> _last_errors;
};

}
