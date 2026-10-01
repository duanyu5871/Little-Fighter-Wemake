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
  dat_translator/cookers.cpp
  dat_translator/frame_behavior.cpp
  dat_translator/make_frame_state.cpp
  dat_translator/string_matchers.cpp
  dat_translator/helpers.cpp
  dat_translator/next_frame.cpp
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
