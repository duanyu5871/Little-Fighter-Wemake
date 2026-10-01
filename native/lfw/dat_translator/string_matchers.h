#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace lfw {
namespace dat_translator {

std::vector<std::pair<std::u16string, std::u16string>> match_colon_value(const std::u16string& text);

std::optional<std::u16string> match_block_once(const std::u16string& text,
                                               const std::u16string& start,
                                               const std::u16string& end);

struct TakeBlocksResult {
  std::vector<std::u16string> blocks;
  std::u16string remains;
};

TakeBlocksResult take_blocks(const std::u16string& text, const std::u16string& start,
                             const std::u16string& end);

std::optional<std::u16string> match_hash_end(const std::u16string& text);

}

}
