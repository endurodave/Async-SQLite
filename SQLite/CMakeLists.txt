# Collect all .c files in this subdirectory
file(GLOB SUBDIR_SOURCES "*.c" "*.cpp")

# Collect all .h files in this subdirectory
file(GLOB SUBDIR_HEADERS "*.h")

# Create a library target 
add_library(SQLiteLib STATIC ${SUBDIR_SOURCES} ${SUBDIR_HEADERS})

# Include directories for the library
target_include_directories(SQLiteLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(SQLiteLib LINK_PUBLIC 
    PortLib
)