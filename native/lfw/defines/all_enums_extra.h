#pragma once

#include <vector>

#include "lfw/defines/bin_op.h"
#include "lfw/defines/cmd.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/enum_entries.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/hit_flag.h"

namespace lfw {

inline void append_extra_number_enum_tables(std::vector<EnumNumberTableRef>& out) {
  out.push_back({u"EntityEnum", &entity_enum_entries(), &entity_enum_name_of});
  out.push_back({u"FacingFlag", &facing_flag_entries(), &facing_flag_name_of});
  out.push_back({u"HitFlag", &hit_flag_entries(), &hit_flag_name_of});
}

inline void append_extra_text_enum_tables(std::vector<EnumTextTableRef>& out) {
  out.push_back({u"BinOp", &bin_op_entries()});
  out.push_back({u"CMD", &cmd_entries()});
}

}
