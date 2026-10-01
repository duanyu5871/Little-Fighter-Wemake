add_library(lfw_core STATIC
  core/js_num.cpp
  core/state_hash.cpp
  core/mersenne_twister.cpp
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
