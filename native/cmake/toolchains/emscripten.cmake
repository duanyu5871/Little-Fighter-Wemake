set(CMAKE_SYSTEM_NAME Emscripten)

find_program(EMCC_EXECUTABLE emcc REQUIRED)
find_program(EMXX_EXECUTABLE em++ REQUIRED)
find_program(EMMM_EXECUTABLE emar REQUIRED)
find_program(EMRANLIB_EXECUTABLE emranlib REQUIRED)

set(CMAKE_C_COMPILER "${EMCC_EXECUTABLE}")
set(CMAKE_CXX_COMPILER "${EMXX_EXECUTABLE}")
set(CMAKE_AR "${EMMM_EXECUTABLE}")
set(CMAKE_RANLIB "${EMRANLIB_EXECUTABLE}")

add_compile_options(-ffp-contract=off -fno-fast-math)
add_compile_options(-fno-exceptions -fno-rtti)
add_compile_options(-msimd128=off)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
