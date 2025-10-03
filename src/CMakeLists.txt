set(SOURCES
    knotscore/main.cpp
    knotscore/GitUtils.cpp
    knotscore/DiffPrinter.cpp
    knotscore/FileWalker.cpp
)

add_executable(KnotesCore ${SOURCES})

target_include_directories(KnotesCore PRIVATE knotscore)
target_link_libraries(KnotesCore PRIVATE ${LIBGIT2_LIBRARIES})

# Optional: make a custom target to update submodules at build time
find_package(Git REQUIRED)
add_custom_target(update_submodules
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --remote
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Updating git submodules (Core and Knots)"
)

