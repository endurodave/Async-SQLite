# Example CMake command line to create project build files:
#
# *** Windows ***
# cmake -G "Visual Studio 17 2022" -A Win32 -B ../Async-SQLiteBuild -S .
#
# *** Linux ***
# cmake -G "Unix Makefiles" -B ../Async-SQLiteBuild -S .

# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language (C or C++)
project(Async-SQLite VERSION 1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Collect all .cpp and *.h source files in the current directory
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/*.cpp" "${CMAKE_SOURCE_DIR}/*.h")

# Add subdirectories to include path
include_directories( 
    ${CMAKE_SOURCE_DIR}/Delegate
    ${CMAKE_SOURCE_DIR}/SQLite
    ${CMAKE_SOURCE_DIR}/Port
)

if (ENABLE_ALLOCATOR)
include_directories( 
    ${CMAKE_SOURCE_DIR}/Allocator
)
endif()

# Add an executable target
add_executable(Async-SQLiteApp ${SOURCES})

if (ENABLE_ALLOCATOR)
    add_compile_definitions(USE_ALLOCATOR)
endif()

# Add subdirectories to build
add_subdirectory(Delegate)
add_subdirectory(SQLite)
add_subdirectory(Port)

if (ENABLE_ALLOCATOR)
    add_subdirectory(Allocator)
endif()

target_link_libraries(Async-SQLiteApp PRIVATE 
    DelegateLib
    SQLiteLib
    PortLib
)

if (ENABLE_ALLOCATOR)
    target_link_libraries(Async-SQLiteApp PRIVATE 
        AllocatorLib
)
endif()

