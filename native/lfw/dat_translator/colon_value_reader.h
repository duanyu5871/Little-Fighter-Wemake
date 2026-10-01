#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

class Object;

namespace dat_translator {

enum class ColonCellType { kStr, kInt, kIntInt };

struct ColonCell {
  std::u16string name;
  ColonCellType type = ColonCellType::kStr;
};

class ColonValueReader {
 public:
  ColonValueReader& str(std::u16string name);
  ColonValueReader& int_(std::u16string name);
  ColonValueReader& int_2(std::u16string name);
  std::u16string read(const std::u16string& text, Object& output) const;

  const std::vector<ColonCell>& cells() const { return _cells; }

 private:
  std::vector<ColonCell> _cells;
};

}

}
