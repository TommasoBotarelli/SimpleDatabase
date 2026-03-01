cmake_minimum_required(VERSION 3.23.0)
project(DatabaseFromScratch VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Global Setup ---
# This ensures that our internal library headers can be found easily
include_directories(src)

# --- 2. Build the Core Library ---
# We go into 'src' to build the actual logic
add_subdirectory(src)

# --- 3. Build the Main Application ---
# This is just the entry point (main.cpp)
add_executable(DatabaseFromScratch src/main.cpp)
target_link_libraries(DatabaseFromScratch PRIVATE DatabaseEngine)

# --- 4. Testing Infrastructure ---
enable_testing()
add_subdirectory(tests)

