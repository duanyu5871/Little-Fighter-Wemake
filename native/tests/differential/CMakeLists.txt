file(GLOB SUBJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/subjects/*.cpp")

foreach(src ${SUBJECT_SOURCES})
  get_filename_component(subject "${src}" NAME_WE)
  set(target "lfw_trace_${subject}")
  add_executable(${target} "${src}")
  target_link_libraries(${target} PRIVATE lfw_core)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endforeach()
