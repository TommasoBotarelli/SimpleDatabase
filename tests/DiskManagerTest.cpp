#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
#include "DiskManager.h"

namespace fs = std::filesystem;

class DiskManagerReadTest : public testing::Test {
protected:
    const std::string test_db = "read_test.bin";

    void SetUp() override {
        if (fs::exists(test_db)) fs::remove(test_db);
    }

    void TearDown() override {
        if (fs::exists(test_db)) fs::remove(test_db);
    }

    // Helper to create a file with specific content for testing
    void PrepareFile(const std::vector<char>& data) {
        std::ofstream ofs(test_db, std::ios::binary | std::ios::out);
        ofs.write(data.data(), data.size());
        ofs.close();
    }
};

// --- TEST CASES ---

TEST_F(DiskManagerReadTest, ReadFullPageSuccessfully) {
    // 1. Create a file with exactly 1 page of 'A' (0x41)
    std::vector<char> mock_data(PAGE_SIZE, 'A');
    PrepareFile(mock_data);

    DiskManager dm(test_db);
    std::array<std::byte, PAGE_SIZE> buffer;
    
    // 2. Read page 0
    dm.read_page(0, buffer);

    // 3. Verify
    for (auto b : buffer) {
        EXPECT_EQ(b, std::byte{'A'});
    }
}

TEST_F(DiskManagerReadTest, ReadPartialPageZerosRemainingBuffer) {
    // 1. Create a file with only 10 bytes (half-page or less)
    std::vector<char> small_data(10, 'B');
    PrepareFile(small_data);

    DiskManager dm(test_db);
    std::array<std::byte, PAGE_SIZE> buffer;
    
    dm.read_page(0, buffer);

    // 2. First 10 bytes should be 'B'
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(buffer[i], std::byte{'B'});
    }
    // 3. The rest must be zeroed by your function's logic
    for (size_t i = 10; i < PAGE_SIZE; ++i) {
        EXPECT_EQ(buffer[i], std::byte{0});
    }
}

TEST_F(DiskManagerReadTest, ReadNonExistentPageReturnsZeros) {
    // 1. Create an empty file
    PrepareFile({});

    DiskManager dm(test_db);
    std::array<std::byte, PAGE_SIZE> buffer;
    
    // Fill buffer with garbage first to ensure the function actually clears it
    std::fill(buffer.begin(), buffer.end(), std::byte{0xFF});

    // 2. Read page 5 (well beyond EOF)
    dm.read_page(5, buffer);

    // 3. Verify entire buffer is zeroed
    for (auto b : buffer) {
        EXPECT_EQ(b, std::byte{0});
    }
}

TEST_F(DiskManagerReadTest, WriteSinglePage) {
    // 1. Create an empty file
    PrepareFile({});

    DiskManager dm(test_db);
    std::array<std::byte, PAGE_SIZE> buffer;
    std::fill(buffer.begin(), buffer.end(), std::byte{'A'});
    
    dm.write_page(0, buffer);

    std::array<std::byte, PAGE_SIZE> buffer_expected;
    std::fill(buffer_expected.begin(), buffer_expected.end(), std::byte{'A'});

    dm.read_page(0, buffer);

    std::cout << buffer_expected.size() << std::endl;

    for (int i = 0; i < buffer_expected.size(); i++) {
        EXPECT_EQ(buffer_expected[i], buffer[i]);
    }
}

TEST_F(DiskManagerReadTest, WritePageIdGreaterThanSaved) {
    // 1. Create an empty file
    PrepareFile({});

    DiskManager dm(test_db);
    std::array<std::byte, PAGE_SIZE> buffer;
    std::fill(buffer.begin(), buffer.end(), std::byte{'A'});
    
    dm.write_page(2, buffer);

    for (int page = 0; page < 2; page++) {
        dm.read_page(page, buffer);
        for (int i = 0; i < PAGE_SIZE; i++) {
            EXPECT_EQ(buffer[i], std::byte{0});
        }
    }

    dm.read_page(2, buffer);
    std::array<std::byte, PAGE_SIZE> buffer_expected;
    std::fill(buffer_expected.begin(), buffer_expected.end(), std::byte{'A'});

    std::cout << buffer_expected.size() << std::endl;

    for (int i = 0; i < buffer_expected.size(); i++) {
        EXPECT_EQ(buffer_expected[i], buffer[i]);
    }
}