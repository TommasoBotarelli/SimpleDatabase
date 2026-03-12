include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(unit_tests 
    DiskManagerTest.cpp
    PageTest.cpp
    # BTreeTest.cpp <-- Add more test files here as you go
)

target_link_libraries(unit_tests 
    PRIVATE 
        GTest::gtest_main 
        DatabaseEngine
)

include(GoogleTest)
gtest_discover_tests(unit_tests)

# This tells CMake: "After building the 'unit_tests' target, execute it"
add_custom_command(TARGET unit_tests POST_BUILD
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Running unit tests..."
)