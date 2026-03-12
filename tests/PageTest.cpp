#include <gtest/gtest.h>
#include "Page.h"
#include <vector>
#include <cstddef>

// ---------------------------------------------------------
// Test Fixture (Optional, but good for shared setup)
// ---------------------------------------------------------
class PageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset or initialize anything before each test if needed
    }

    // Helper function to create dummy row data
    std::vector<std::byte> CreateDummyRow(size_t size, std::byte fill_value = std::byte{0xAA}) {
        return std::vector<std::byte>(size, fill_value);
    }
};

// ---------------------------------------------------------
// Test Cases
// ---------------------------------------------------------

TEST_F(PageTest, InitializationSetsCorrectDefaults) {
    Page page;
    PageHeader* header = page.get_header();

    EXPECT_EQ(header->page_id, INVALID_PAGE_ID);
    EXPECT_EQ(header->tuple_count, 0);
    EXPECT_EQ(header->free_space_pointer, PAGE_SIZE);
}

TEST_F(PageTest, AddSingleRowSucceeds) {
    Page page;
    size_t row_size = 50;
    auto row_data = CreateDummyRow(row_size);
    
    auto slot_id = page.add_row(row_data);

    // Verify insertion succeeded and returned slot 0
    ASSERT_TRUE(slot_id.has_value());
    EXPECT_EQ(slot_id.value(), 0);

    // Verify header was updated correctly
    PageHeader* header = page.get_header();
    EXPECT_EQ(header->tuple_count, 1);
    EXPECT_EQ(header->free_space_pointer, PAGE_SIZE - row_size);
}

TEST_F(PageTest, AddMultipleRowsSucceeds) {
    Page page;
    size_t row1_size = 30;
    size_t row2_size = 40;
    
    auto slot1 = page.add_row(CreateDummyRow(row1_size));
    auto slot2 = page.add_row(CreateDummyRow(row2_size));

    // Verify both insertions returned sequential slots
    ASSERT_TRUE(slot1.has_value());
    ASSERT_TRUE(slot2.has_value());
    EXPECT_EQ(slot1.value(), 0);
    EXPECT_EQ(slot2.value(), 1);

    // Verify header state
    PageHeader* header = page.get_header();
    EXPECT_EQ(header->tuple_count, 2);
    EXPECT_EQ(header->free_space_pointer, PAGE_SIZE - (row1_size + row2_size));
}

TEST_F(PageTest, AddRowRejectsWhenInsufficientSpace) {
    Page page;
    PageHeader* header = page.get_header();
    
    // Calculate exact max data size for a single row
    // Space = PAGE_SIZE - PageHeader size - Slot size
    size_t max_data_size = PAGE_SIZE - sizeof(PageHeader) - sizeof(Slot);
    
    // Attempt to add a row that is exactly 1 byte too large
    auto row_too_big = CreateDummyRow(max_data_size + 1);
    auto slot_id = page.add_row(row_too_big);

    // Verify the insertion failed
    EXPECT_FALSE(slot_id.has_value());
    
    // Verify the page state remained completely unchanged
    EXPECT_EQ(header->tuple_count, 0);
    EXPECT_EQ(header->free_space_pointer, PAGE_SIZE);
}

TEST_F(PageTest, AddRowAcceptsExactFit) {
    Page page;
    PageHeader* header = page.get_header();
    
    size_t max_data_size = PAGE_SIZE - sizeof(PageHeader) - sizeof(Slot);
    
    // Attempt to add a row that perfectly fills the page
    auto exact_fit_row = CreateDummyRow(max_data_size);
    auto slot_id = page.add_row(exact_fit_row);

    // Verify it succeeds
    EXPECT_TRUE(slot_id.has_value());
    EXPECT_EQ(header->tuple_count, 1);
    EXPECT_EQ(header->free_space_pointer, PAGE_SIZE - max_data_size);
}