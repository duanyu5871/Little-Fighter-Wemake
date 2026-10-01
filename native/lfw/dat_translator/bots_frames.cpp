#include "lfw/dat_translator/bots_frames.h"

#include <memory>
#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

Value bots_frames_defends() {
  static const Value v = [] {
    Array a;
    a.push_back(Value(std::u16string(u"110")));
    a.push_back(Value(std::u16string(u"111")));
    return Value(std::make_shared<Array>(a));
  }();
  return v;
}

}
}
