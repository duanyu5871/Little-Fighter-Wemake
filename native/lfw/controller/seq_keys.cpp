#include "lfw/controller/seq_keys.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace controller {

SeqKeys::SeqKeys(const std::u16string& keys, const Value& data)
    : _keys(keys), _data(data) {}

void SeqKeys::press(const std::u16string& keys) {
  const size_t len = _keys.size();
  std::vector<char16_t> arr(keys.begin(), keys.end());
  while (_idx < static_cast<double>(len) && !arr.empty()) {
    const char16_t expected = _keys[static_cast<size_t>(_idx)];
    size_t j = arr.size();
    for (size_t i = 0; i < arr.size(); ++i) {
      if (arr[i] == expected) {
        j = i;
        break;
      }
    }
    if (j == arr.size()) {
      _idx = 0;
      _hit = 0;
      return;
    }
    arr.erase(arr.begin() + static_cast<std::ptrdiff_t>(j));
    if (_idx == static_cast<double>(len) - 1.0) {
      _idx = 0;
      _hit = 1;
      return;
    }
    _idx += 1;
  }
}

void SeqKeys::reset() {
  _idx = 0;
  _hit = 0;
}

Value SeqKeys::to_snapshot() const {
  Object o;
  o.set(u"idx", Value(_idx));
  o.set(u"hit", Value(_hit));
  o.set(u"keys", Value(_keys));
  o.set(u"data", _data);
  return Value(std::make_shared<Object>(o));
}

void SeqKeys::from_snapshot(const Value& s) {
  _idx = to_number(field_or(s, u"idx"));
  _hit = to_number(field_or(s, u"hit"));
  _keys = to_string(field_or(s, u"keys"));
  _data = field_or(s, u"data");
}

}
}
