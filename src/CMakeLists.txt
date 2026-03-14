# Create a library that holds all our database logic
add_library(DatabaseEngine STATIC
    DiskManager.cpp
    Page.cpp
    BufferPoolManager.cpp
    # BTree.cpp  <-- Uncomment these as you add more files/folders
    # QueryParser.cpp
)

# If you move DiskManager to src/storage/ later, you would do:
# target_sources(DatabaseEngine PRIVATE storage/DiskManager.cpp)

# Export the include path so the App and Tests can find the .h files
target_include_directories(DatabaseEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})