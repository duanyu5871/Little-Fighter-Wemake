#pragma once

#include <vector>

namespace lfw {

struct EnumNumberEntry {
  const char16_t* name;
  double value;
};

struct EnumTextEntry {
  const char16_t* name;
  const char16_t* text;
};

struct EnumNumberTableRef {
  const char16_t* name;
  const std::vector<EnumNumberEntry>* entries;
  const char16_t* (*name_of)(int);
};

struct EnumTextTableRef {
  const char16_t* name;
  const std::vector<EnumTextEntry>* entries;
};

}
