#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

struct DefinesRuntimeEntry {
  std::u16string name;
  std::u16string value_json5;
  bool is_top_level;
};

const std::vector<DefinesRuntimeEntry>& defines_runtime_entries();

Value bg_info_new();
Value bg_layer_info_new();
Value bg_data_new();
Value armor_Info_new();
Value bdy_info_new();
Value bpoint_info_new();
Value chase_info_new();
Value cpoint_new();
Value dat_index_new();
Value dialog_info_new();
Value drink_info_new();
Value entity_data_new();
Value entity_info_new();
Value frame_indexes_new();
Value frame_info_new();
Value frame_model_new();
Value frame_pic_new();
Value itr_info_new();
Value model_info_new();
Value next_frame_new();
Value opoint_info_new();
Value opoint_multi_new();
Value picture_info_new();
Value sound_play_info_new();
Value stage_info_new();
Value stage_object_info_new();
Value stage_phase_info_new();
Value terrain_info_new();
Value wpoint_info_new();

}
