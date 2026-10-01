add_executable(lfw_mt_trace mt_trace_main.cpp)
target_link_libraries(lfw_mt_trace PRIVATE lfw_core)

set_target_properties(lfw_mt_trace PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
