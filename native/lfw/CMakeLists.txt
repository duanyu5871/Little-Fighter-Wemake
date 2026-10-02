add_library(lfw_core STATIC
  core/js_num.cpp
  core/js_string.cpp
  core/json.cpp
  core/json5.cpp
  core/json5_unicode.cpp
  core/state_hash.cpp
  core/value.cpp
  fields.cpp
  defines/defines.cpp
  defines/labels.cpp
  defines/runtime_gen.cpp
  dat_translator/colon_value_reader.cpp
  dat_translator/cond_maker.cpp
  dat_translator/hit_next_frame.cpp
  dat_translator/ball_bdy.cpp
  dat_translator/ball_frame_state.cpp
  dat_translator/bg_data.cpp
  dat_translator/cookers.cpp
  dat_translator/cook_frames.cpp
  dat_translator/entity_data.cpp
  dat_translator/entity_kinds.cpp
  dat_translator/frame_behavior.cpp
  dat_translator/itr_prefabs.cpp
  dat_translator/make_frame_state.cpp
  dat_translator/make_ball_special.cpp
  dat_translator/make_weapon_special.cpp
  dat_translator/make_stage_info_list.cpp
  dat_translator/cook_file_variants.cpp
  dat_translator/frame_editing.cpp
  dat_translator/broken_piece_frames.cpp
  dat_translator/parase_indexes.cpp
  dat_translator/string_matchers.cpp
  dat_translator/helpers.cpp
  dat_translator/next_frame.cpp
  dat_translator/bots_frames.cpp
  dat_translator/make_fighter_data.cpp
  dat_translator/obj_dat_to_json.cpp
  dat_translator/cook_frame_indicator_info.cpp
  dat_translator/make_buring_smoke.cpp
  dat_translator/bots/frames.cpp
  dat_translator/bots/bot_actions.cpp
  loader/preprocess_pic.cpp
  loader/preprocess_stage.cpp
  loader/preprocess_ball_frame.cpp
  loader/preprocess_bg_data.cpp
  loader/resolve_prefab.cpp
  loader/preprocess_action.cpp
  loader/preprocess_bot_data.cpp
  loader/preprocess_next_frame.cpp
  base/predicate.cpp
  utils/math/mersenne_twister.cpp
  utils/math/normalize_plane.cpp
  utils/math/calc_plane.cpp
  utils/math/line_plane_intersection.cpp
  utils/math/range.cpp
  utils/math/probability.cpp
  utils/math/project_to_line.cpp
  utils/utf8.cpp
  utils/times.cpp
)

target_include_directories(lfw_core PUBLIC
  $<BUILD_INTERFACE:${LFW_NATIVE_ROOT}>
)

if(MSVC)
  target_compile_options(lfw_core PRIVATE /GR-)
  target_compile_definitions(lfw_core PRIVATE _HAS_EXCEPTIONS=0)
else()
  target_compile_options(lfw_core PRIVATE -fno-exceptions -fno-rtti)
endif()

target_link_libraries(lfw_core PUBLIC)
