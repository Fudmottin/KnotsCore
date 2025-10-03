add_executable(KnotesCore knotscore/main.cpp)
target_include_directories(KnotesCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/knotescore)
# No external deps required; rely on system git
if(UNIX)
  find_program(GIT_EXECUTABLE git REQUIRED)
  if(NOT GIT_EXECUTABLE)
    message(FATAL_ERROR "git not found")
  endif()
  target_compile_definitions(KnotesCore PRIVATE GIT_EXECUTABLE="${GIT_EXECUTABLE}")
endif()

