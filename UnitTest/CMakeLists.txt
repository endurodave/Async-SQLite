# Collect all .c files in this subdirectory
file(GLOB SUBDIR_SOURCES "*.c" "*.cpp")

# Collect all .h files in this subdirectory
file(GLOB SUBDIR_HEADERS "*.h")

# Create a library target 
add_library(UnitTestLib STATIC ${SUBDIR_SOURCES} ${SUBDIR_HEADERS})

# Include directories for the library
target_include_directories(UnitTestLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Add /bigobj flag for MSVC 
if (MSVC)
    target_compile_options(UnitTestLib PRIVATE /bigobj)
endif()