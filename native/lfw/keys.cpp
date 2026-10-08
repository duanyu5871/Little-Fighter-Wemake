#include "lfw/keys.h"

#include <utility>

namespace lfw {

Keys::Keys(IKeysLfw& lfw) : _lfw(&lfw) {
  // TS 的字段初始化顺序：`[GK.L] [GK.R] [GK.U] [GK.D] [GK.a] [GK.j] [GK.d]`。
  for (const char16_t* const k : {u"L", u"R", u"U", u"D", u"a", u"j", u"d"}) {
    const std::u16string key(k);
    _keys.emplace_back(key, controller::KeyStatus(Value(key)));
  }
}

controller::KeyStatus* Keys::get(const std::u16string& key) {
  for (auto& kv : _keys) {
    if (kv.first == key) return &kv.second;
  }
  return nullptr;
}

void Keys::mount() { _lfw->regist_keys(*this); }

void Keys::unmount() { _lfw->recycle_keys(*this); }

}  // namespace lfw
