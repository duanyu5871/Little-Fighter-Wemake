find_program(EMCC_EXECUTABLE emcc REQUIRED)
find_program(EMXX_EXECUTABLE em++ REQUIRED)
find_program(EMMM_EXECUTABLE emar REQUIRED)
find_program(EMRANLIB_EXECUTABLE emranlib REQUIRED)

# emscripten: `CMAKE_SYSTEM_NAME Emscripten` 只有 CMake 自己 `Modules/Platform/` 下有
# `Emscripten.cmake` 时才被认识；VS 捆绑的 CMake 把这个文件裁掉了，只留下一句
# "System is unknown to cmake"，随后按通用规则链接（wasm 产物后缀/链接参数全丢）。
# emsdk 自带完整实现 ⇒ 能定位到就整个交给它，否则退回原来的手写设置。
get_filename_component(_lfw_emsdk_emscripten_dir "${EMCC_EXECUTABLE}" DIRECTORY)
set(_lfw_emsdk_platform "${_lfw_emsdk_emscripten_dir}/cmake/Modules/Platform/Emscripten.cmake")
if(EXISTS "${_lfw_emsdk_platform}")
  include("${_lfw_emsdk_platform}")
else()
  set(CMAKE_SYSTEM_NAME Emscripten)
  set(CMAKE_C_COMPILER "${EMCC_EXECUTABLE}")
  set(CMAKE_CXX_COMPILER "${EMXX_EXECUTABLE}")
  set(CMAKE_AR "${EMMM_EXECUTABLE}")
  set(CMAKE_RANLIB "${EMRANLIB_EXECUTABLE}")
endif()

add_compile_options(-ffp-contract=off -fno-fast-math)
add_compile_options(-fno-exceptions -fno-rtti)
# emscripten: clang 不吃 `-msimd128=off`（unknown argument），否定形式是 `-mno-simd128`。
add_compile_options(-mno-simd128)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
