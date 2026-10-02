#pragma once

#include <string>
#include <utility>
#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace gk {

inline constexpr const char16_t* kL = u"L";
inline constexpr const char16_t* kLeft = u"L";
inline constexpr const char16_t* kR = u"R";
inline constexpr const char16_t* kRight = u"R";
inline constexpr const char16_t* kU = u"U";
inline constexpr const char16_t* kUp = u"U";
inline constexpr const char16_t* kD = u"D";
inline constexpr const char16_t* kDown = u"D";
inline constexpr const char16_t* ka = u"a";
inline constexpr const char16_t* kAttack = u"a";
inline constexpr const char16_t* kj = u"j";
inline constexpr const char16_t* kJump = u"j";
inline constexpr const char16_t* kd = u"d";
inline constexpr const char16_t* kDefend = u"d";

}

inline const std::vector<EnumTextEntry>& gk_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"L", gk::kL},
    {u"Left", gk::kLeft},
    {u"R", gk::kR},
    {u"Right", gk::kRight},
    {u"U", gk::kU},
    {u"Up", gk::kUp},
    {u"D", gk::kD},
    {u"Down", gk::kDown},
    {u"a", gk::ka},
    {u"Attack", gk::kAttack},
    {u"j", gk::kj},
    {u"Jump", gk::kJump},
    {u"d", gk::kd},
    {u"Defend", gk::kDefend},
  };
  return e;
}

inline const std::vector<std::pair<const char16_t*, const char16_t*>>& gk_label_table() {
  static const std::vector<std::pair<const char16_t*, const char16_t*>> t = {
    {gk::kL, u"<"},
    {gk::kR, u">"},
    {gk::kU, u"^"},
    {gk::kD, u"v"},
    {gk::ka, u"A"},
    {gk::kj, u"J"},
    {gk::kd, u"D"},
  };
  return t;
}

inline const std::vector<const char16_t*>& all_game_keys() {
  static const std::vector<const char16_t*> t = {gk::kd, gk::kL, gk::kR,
                                                 gk::kU, gk::kD, gk::kj, gk::ka};
  return t;
}

inline const std::vector<std::pair<const char16_t*, const char16_t*>>& conflicts_key_table() {
  static const std::vector<std::pair<const char16_t*, const char16_t*>> t = {
    {gk::ka, nullptr},
    {gk::kj, nullptr},
    {gk::kd, nullptr},
    {gk::kL, gk::kR},
    {gk::kR, gk::kL},
    {gk::kU, gk::kD},
    {gk::kD, gk::kU},
  };
  return t;
}

inline std::u16string gk_label_of(const std::u16string& key) {
  for (const auto& e : gk_label_table()) {
    if (std::u16string(e.first) == key) return std::u16string(e.second);
  }
  return std::u16string();
}

inline const char16_t* conflicts_key_of(const std::u16string& key) {
  for (const auto& e : conflicts_key_table()) {
    if (std::u16string(e.first) == key) return e.second;
  }
  return nullptr;
}

}
