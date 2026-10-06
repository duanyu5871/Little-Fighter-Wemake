#include "lfw/loader/get_import_fallbacks.h"

#include <string>
#include <variant>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

namespace {

// `path.endsWith(suffix)` ⇒ `[dir_part, name_part, suffix]`（`/` 之后是名字）。
bool split_path(const std::u16string& path, const std::u16string& suffix,
                std::u16string& dir_part, std::u16string& name_part) {
  if (path.size() < suffix.size() ||
      path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
    return false;
  const size_t slash = path.rfind(u'/');
  const long long name_index = slash == std::u16string::npos ? -1 : static_cast<long long>(slash);
  // `substring(0, name_index + 1)`：无 `/` 时是 `-1 + 1 == 0` ⇔ 空串。
  dir_part = path.substr(0, static_cast<size_t>(name_index + 1));
  long long begin = name_index + 1;
  long long end = static_cast<long long>(path.size() - suffix.size());
  // JS 的 `substring` 在两个端点反了时会**交换**（例如 `a/.png` 会取到 `/`）。
  if (begin > end) std::swap(begin, end);
  name_part = path.substr(static_cast<size_t>(begin), static_cast<size_t>(end - begin));
  return true;
}

// `fallbacks.unshift(...xs.filter(v => v !== name))`：与原名相同的那条不进列表。
void push_if_new(std::vector<std::u16string>& fallbacks, const std::u16string& name,
                 const std::u16string& v) {
  if (v != name) fallbacks.push_back(v);
}

}

bool get_import_fallbacks(const Value& name, std::vector<std::u16string>& fallbacks,
                          std::u16string& suffix) {
  fallbacks.clear();
  suffix.clear();
  const std::u16string* const text = std::get_if<std::u16string>(&name);
  if (text == nullptr) return false;   // `path.endsWith` 抛

  std::u16string dir_part;
  std::u16string name_part;
  const char16_t* const image_suffixes[] = {u".png", u".bmp", u".webp"};
  for (const char16_t* const s : image_suffixes) {
    if (!split_path(*text, std::u16string(s), dir_part, name_part)) continue;
    suffix = std::u16string(s);
    // TS：`fallbacks.unshift(...[...].filter(v => v !== name))` ⇒ 新名在前、原名殿后。
    static const char16_t* const kinds[] = {u"@4x.webp", u"@4x.png", u"@3x.webp", u"@3x.png",
                                            u"@2x.webp", u"@2x.png"};
    for (const char16_t* const k : kinds) push_if_new(fallbacks, *text, dir_part + name_part + k);
    for (const char16_t* const fold : {u"@4x", u"@3x", u"@2x"}) {
      push_if_new(fallbacks, *text, dir_part + fold + u"/" + name_part + u".webp");
      push_if_new(fallbacks, *text, dir_part + fold + u"/" + name_part + u".png");
    }
    push_if_new(fallbacks, *text, dir_part + name_part + u".webp");
    push_if_new(fallbacks, *text, dir_part + name_part + u".png");
    fallbacks.push_back(*text);
    return true;
  }

  const char16_t* const sound_suffixes[] = {u".wav", u".wma"};
  for (const char16_t* const s : sound_suffixes) {
    if (!split_path(*text, std::u16string(s), dir_part, name_part)) continue;
    suffix = std::u16string(s);
    fallbacks.push_back(dir_part + name_part + u".mp3");
    fallbacks.push_back(dir_part + name_part + suffix + u".mp3");
    fallbacks.push_back(*text);
    return true;
  }

  fallbacks.push_back(*text);
  return true;
}

}
}
