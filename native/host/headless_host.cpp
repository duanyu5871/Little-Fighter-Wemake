#include "host/headless_host.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <system_error>

#include "lfw/core/js_num.h"
#include "lfw/core/json.h"
#include "lfw/core/json5.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/utf8.h"

namespace lfw::host {

namespace {

const std::u16string* as_str(const Value& v) { return std::get_if<std::u16string>(&v); }

std::u16string as_u16(const std::string& s) {
  return decode_utf8(std::vector<uint8_t>(s.begin(), s.end()));
}

std::string as_u8(const std::u16string& s) {
  const std::vector<uint8_t> buf = encode_utf8(s);
  return std::string(buf.begin(), buf.end());
}

std::string to_utf8(const std::u16string& s) { return as_u8(s); }

// `Ditto.warn(...args)` 的打印口径：参数按 `to_string` 拼（TS 里是 `console` 的多参形式）。
std::u16string join_args(const std::vector<Value>& args) {
  std::u16string out;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i != 0) out += u" ";
    out += to_string(args[i]);
  }
  return out;
}

}  // namespace

HeadlessHost::HeadlessHost(std::vector<std::filesystem::path> roots, bool dev)
    : _roots(std::move(roots)), _dev(dev) {}

std::filesystem::path HeadlessHost::resolve(const std::u16string& path) const {
  std::error_code ec;
  for (const std::filesystem::path& root : _roots) {
    const std::filesystem::path candidate = root / std::filesystem::path(to_utf8(path));
    if (std::filesystem::is_regular_file(candidate, ec)) return candidate;
  }
  return {};
}

bool HeadlessHost::read_file(const std::filesystem::path& path, std::string& out,
                             std::u16string& error) const {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    error = u"[host] cannot open: " + as_u16(path.generic_string());
    return false;
  }
  out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  if (!in.good() && !in.eof()) {
    error = u"[host] read failed: " + as_u16(path.generic_string());
    return false;
  }
  return true;
}

void HeadlessHost::keyboard_callback_add(LFW&, LfwKeyboardCallback* cb) {
  if (cb == nullptr) return;
  if (std::find(_kb_callbacks.begin(), _kb_callbacks.end(), cb) == _kb_callbacks.end()) {
    _kb_callbacks.push_back(cb);
  }
}

void HeadlessHost::keyboard_callback_del(LFW&, LfwKeyboardCallback* cb) {
  _kb_callbacks.erase(std::remove(_kb_callbacks.begin(), _kb_callbacks.end(), cb),
                      _kb_callbacks.end());
}

void HeadlessHost::pointings_callback_add(LFW&, LfwPointingsCallback* cb) {
  if (cb == nullptr) return;
  if (std::find(_pt_callbacks.begin(), _pt_callbacks.end(), cb) == _pt_callbacks.end()) {
    _pt_callbacks.push_back(cb);
  }
}

void HeadlessHost::pointings_callback_del(LFW&, LfwPointingsCallback* cb) {
  _pt_callbacks.erase(std::remove(_pt_callbacks.begin(), _pt_callbacks.end(), cb),
                      _pt_callbacks.end());
}

void HeadlessHost::sounds_callback_add(LFW&, LfwSoundsCallback* cb) {
  if (cb == nullptr) return;
  if (std::find(_sd_callbacks.begin(), _sd_callbacks.end(), cb) == _sd_callbacks.end()) {
    _sd_callbacks.push_back(cb);
  }
}

void HeadlessHost::sounds_callback_del(LFW&, LfwSoundsCallback* cb) {
  _sd_callbacks.erase(std::remove(_sd_callbacks.begin(), _sd_callbacks.end(), cb),
                      _sd_callbacks.end());
}

void HeadlessHost::emit_key_down(LfwKeyEvent& e) {
  for (LfwKeyboardCallback* cb : _kb_callbacks) {
    if (cb != nullptr) cb->on_key_down(e);
  }
}

void HeadlessHost::emit_key_up(LfwKeyEvent& e) {
  for (LfwKeyboardCallback* cb : _kb_callbacks) {
    if (cb != nullptr) cb->on_key_up(e);
  }
}

void HeadlessHost::cache_forget(const std::u16string&, double) {}
void HeadlessHost::zip_forget_stored(const std::u16string&, double) {}

void HeadlessHost::load_img(const std::u16string& path) {
  ++_ignored_images;
  if (_dev) std::printf("[host:img] %s\n", to_utf8(path).c_str());
}

bool HeadlessHost::ui_image_load(const std::u16string& img_key, const Value& path, const Value& ops,
                                 Value& out, std::u16string& error) {
  (void)ops;
  (void)error;
  // 渲染 / 位图在平台侧，headless 只把「这张图存在」这件事记下来：给一个零尺寸的描述，
  // 让 UI 树能照 TS 的形状建起来（组件按 `txt_info` 取宽高，画不画由渲染宿主决定）。
  ++_ignored_images;
  Object info;
  info.set(u"key", Value(img_key));
  info.set(u"path", path);
  info.set(u"width", Value(0.0));
  info.set(u"height", Value(0.0));
  out = Value(std::make_shared<Object>(std::move(info)));
  return true;
}

void HeadlessHost::ui_image_pin(const std::u16string&) {}

std::u16string HeadlessHost::md5(const std::u16string& text) {
  // ⚠ 不是真 MD5：只保证「同输入同输出」。真 MD5（与 `Ditto.MD5` 逐位一致）留到宿主 zip 那一刀。
  const std::string bytes = to_utf8(text);
  const auto fnv = [](const std::string& s, uint64_t seed) {
    uint64_t h = seed;
    for (const unsigned char c : s) {
      h ^= static_cast<uint64_t>(c);
      h *= 0x100000001b3ull;
    }
    return h;
  };
  const uint64_t a = fnv(bytes, 0xcbf29ce484222325ull);
  const uint64_t b = fnv(bytes, 0x84222325cbf29ce4ull);
  char buf[40];
  std::snprintf(buf, sizeof(buf), "%016llx%016llx", static_cast<unsigned long long>(a),
                static_cast<unsigned long long>(b));
  return as_u16(std::string(buf));
}

void HeadlessHost::warn(const std::vector<Value>& args) {
  ++_warns;
  std::printf("[host:warn] %s\n", to_utf8(join_args(args)).c_str());
}

void HeadlessHost::error(const std::vector<Value>& args) {
  ++_errors;
  const std::u16string msg = join_args(args);
  if (_last_errors.size() < 20) _last_errors.push_back(msg);
  std::printf("[host:error] %s\n", to_utf8(msg).c_str());
}

void HeadlessHost::log(const std::vector<Value>& args) {
  ++_logs;
  if (_dev) std::printf("[host:log] %s\n", to_utf8(join_args(args)).c_str());
}

void HeadlessHost::debug_msg(const std::vector<Value>& args) {
  if (_dev) std::printf("[host:debug] %s\n", to_utf8(join_args(args)).c_str());
}

std::function<void()> HeadlessHost::sounds_play_bgm(const Value& music) {
  _bgm = to_string(music);
  return []() {};
}

void HeadlessHost::sounds_play(const Value& path, const Value&, const Value&, const Value&) {
  if (_dev) std::printf("[host:sound] %s\n", to_utf8(to_string(path)).c_str());
}

void HeadlessHost::sounds_play_preset(const Value& name, const Value&, const Value&,
                                      const Value&) {
  if (_dev) std::printf("[host:sound:preset] %s\n", to_utf8(to_string(name)).c_str());
}

void HeadlessHost::sounds_play_with_load(const Value& path) {
  if (_dev) std::printf("[host:sound:load] %s\n", to_utf8(to_string(path)).c_str());
}

void HeadlessHost::sounds_load(const Value& name, const Value& src) {
  if (_dev) {
    std::printf("[host:sound:preload] %s <= %s\n", to_utf8(to_string(name)).c_str(),
                to_utf8(to_string(src)).c_str());
  }
}

Value HeadlessHost::measure_text(const Value& text, const Value& style) {
  // 字体度量属平台（TS 里走 canvas / 字体表）。headless 给零尺寸：布局照算，只是都挤在一起。
  Object info;
  info.set(u"text", Value(to_string(text)));
  info.set(u"style", style);
  info.set(u"width", Value(0.0));
  info.set(u"height", Value(0.0));
  return Value(std::make_shared<Object>(std::move(info)));
}

void HeadlessHost::player_cache_get(const std::u16string& name, PlayerInfoCacheEntry& out) {
  const auto it = _player_cache.find(name);
  if (it == _player_cache.end()) {
    out = PlayerInfoCacheEntry{};
    out.missing = true;  // `await Ditto.Cache.get(name)` 回 `undefined`
    return;
  }
  out = PlayerInfoCacheEntry{};
  out.data_kind = PlayerInfoCacheEntry::DataKind::kBytes;
  out.data = it->second.data;
}

bool HeadlessHost::player_cache_del(const std::u16string& name, std::u16string& error) {
  (void)error;
  _player_cache.erase(name);
  return true;
}

void HeadlessHost::player_cache_put(const PlayerInfoCachePut& data) { _player_cache[data.name] = data; }

bool HeadlessHost::import_as_json(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                                  std::u16string& error) {
  for (const std::u16string& url : urls) {
    const std::filesystem::path path = resolve(url);
    if (path.empty()) continue;
    std::string raw;
    if (!read_file(path, raw, error)) return false;
    const std::u16string text = as_u16(raw);
    const Json5Result r = json5_parse(text);
    if (r.ok) {
      data = r.value;
      hit = Value(url);
      return true;
    }
    const std::optional<Value> strict = json_parse(text);
    if (strict.has_value()) {
      data = *strict;
      hit = Value(url);
      return true;
    }
    error = u"[host] import_as_json parse failed: " + url + u" (" + r.error + u")";
    return false;
  }
  error = u"[host] import_as_json not found: " +
          (urls.empty() ? std::u16string(u"(no url)") : urls.front());
  return false;
}

bool HeadlessHost::import_as_text(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                                  std::u16string& error) {
  for (const std::u16string& url : urls) {
    const std::filesystem::path path = resolve(url);
    if (path.empty()) continue;
    std::string raw;
    if (!read_file(path, raw, error)) return false;
    data = Value(as_u16(raw));
    hit = Value(url);
    return true;
  }
  error = u"[host] import_as_text not found: " +
          (urls.empty() ? std::u16string(u"(no url)") : urls.front());
  return false;
}

bool HeadlessHost::import_as_blob_url(const std::vector<std::u16string>& urls, Value& data,
                                      Value& hit, std::u16string& error) {
  for (const std::u16string& url : urls) {
    const std::filesystem::path path = resolve(url);
    if (path.empty()) continue;
    // TS 给的是 blob URL 字符串；宿主没有 blob 层 ⇒ 给相对条目名（消费者只把它当标识）。
    data = Value(url);
    hit = Value(url);
    return true;
  }
  error = u"[host] import_as_blob_url not found: " +
          (urls.empty() ? std::u16string(u"(no url)") : urls.front());
  return false;
}

bool HeadlessHost::import_as_array_buffer(const std::vector<std::u16string>& urls, Value& data,
                                          Value& hit, std::u16string& error) {
  for (const std::u16string& url : urls) {
    const std::filesystem::path path = resolve(url);
    if (path.empty()) continue;
    std::string raw;
    if (!read_file(path, raw, error)) return false;
    auto arr = std::make_shared<Array>();
    for (const unsigned char c : raw) arr->push_back(Value(static_cast<double>(c)));
    data = Value(std::move(arr));
    hit = Value(url);
    return true;
  }
  error = u"[host] import_as_array_buffer not found: " +
          (urls.empty() ? std::u16string(u"(no url)") : urls.front());
  return false;
}

bool HeadlessHost::import_as_image_bitmap(const std::vector<std::u16string>& urls, Value& data,
                                          Value& hit, std::u16string& error) {
  (void)data;
  (void)hit;
  error = u"[host] import_as_image_bitmap: no bitmap decoder in the headless host: " +
          (urls.empty() ? std::u16string(u"(no url)") : urls.front());
  return false;
}

bool HeadlessHost::xml_parse(const Value& text, Value& marker, std::shared_ptr<IXMLElement>& root,
                             std::u16string& error) {
  const std::u16string* const s = as_str(text);
  if (s == nullptr) {
    error = u"[host] xml_parse: text should be string, but got " + to_string(text);
    return false;
  }
  root = _xml.parse(*s);
  if (root == nullptr) {
    // TS：`const root = I.Ditto.XML.parse(text); if (!root) throw ...`。
    error = u"failed to parse xml text (no root element)";
    return false;
  }
  // `marker` 顶 TS 的 `root` 真值：`Resources::import_xml` 判它假值就报「failed to parse」。
  marker = Value(true);
  return true;
}

void HeadlessHost::lang_apply(LFW& lfw, const std::u16string& lang, const std::u16string&) {
  // TS 在这一步还会遍历 UI 节点把 `text` 重新度量。headless 不做渲染 ⇒ 只切 i18n 的语言表，
  // 节点文本停在旧语言（记 README 偏差表）。
  std::u16string ignored;
  lfw.i18n().set_lang(Value(lang), ignored);
}

Value HeadlessHost::zip_get_stored(const std::u16string&, const std::u16string&) { return Value(); }

bool HeadlessHost::zip_read_blob(const std::u16string& name, const Value&, const Value&, IZip*& out,
                                 std::u16string& error) {
  (void)out;
  error = u"[host] zip_read_blob: not supported (load unpacked packages via --zip): " + name;
  return false;
}

bool HeadlessHost::zip_read_buf(const std::u16string& name, const Value&, IZip*& out,
                                std::u16string& error) {
  (void)out;
  error = u"[host] zip_read_buf: not supported (load unpacked packages via --zip): " + name;
  return false;
}

bool HeadlessHost::zip_download(const std::u16string& zip_url, const Value&, LFW&, DownloadedZip& out,
                                std::u16string& error) {
  (void)out;
  error = u"[host] zip_download: not supported (load unpacked packages via --zip): " + zip_url;
  return false;
}

Value HeadlessHost::zip_cache_get(const std::u16string& name) {
  const auto it = _zip_cache.find(name);
  return it == _zip_cache.end() ? Value() : it->second;
}

void HeadlessHost::zip_cache_del(const std::u16string& name, const std::u16string&) {
  _zip_cache.erase(name);
}

void HeadlessHost::zip_cache_put(const Value& entry) {
  const std::u16string* const name = as_str(field_or(entry, u"name"));
  if (name == nullptr) return;
  _zip_cache[*name] = entry;
}

}
