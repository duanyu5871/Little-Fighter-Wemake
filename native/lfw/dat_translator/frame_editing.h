#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

struct FrameEditing {
  Value frame;
  const Object* costs = nullptr;

  FrameEditing(const Value& f, const Object* c);
  FrameEditing& init(const Value& f);
  FrameEditing& keydown(const Value& key, const std::vector<Value>& nexts);
  FrameEditing& hit(const Value& key, const std::vector<Value>& nexts);
  FrameEditing& seq(const std::u16string& key, const std::vector<Value>& nexts);
};

}
}
