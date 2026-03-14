#include <gtest/gtest.h>
#include <cstdio> // For std::remove
#include "BufferPoolManager.h"

// Test fixture to set up and tear down the environment for each test
class BufferPoolManagerTest : public ::testing::Test {
protected:
    DiskManager* disk_manager_{nullptr};
    BufferPoolManager* bpm_{nullptr};
    const std::string test_db_name = "test_database.db";

    void SetUp() override {
        // Remove any leftover file from previous runs
        std::remove(test_db_name.c_str());
        
        disk_manager_ = new DiskManager(test_db_name);
        // We use a small pool size (3) to easily trigger eviction logic
        bpm_ = new BufferPoolManager(3, disk_manager_);
    }

    void TearDown() override {
        delete bpm_;
        delete disk_manager_;
        // Clean up the test database file
        std::remove(test_db_name.c_str());
    }
};

TEST_F(BufferPoolManagerTest, NewPageTest) {
    page_id_t page_id = 0;
    Page* page = bpm_->new_page(&page_id);

    ASSERT_NE(page, nullptr);
    EXPECT_EQ(page->get_header()->page_id, page_id);
    
    // Unpin the page so it can be evicted later if needed
    EXPECT_TRUE(bpm_->unpin_page(page_id, false));
}

TEST_F(BufferPoolManagerTest, FetchPageTest) {
    page_id_t page_id = 1;
    bpm_->new_page(&page_id);
    bpm_->unpin_page(page_id, false);

    // Fetch the page we just created
    Page* fetched_page = bpm_->fetch_page(page_id);
    
    ASSERT_NE(fetched_page, nullptr);
    EXPECT_EQ(fetched_page->get_header()->page_id, page_id);
    
    bpm_->unpin_page(page_id, false);
}

TEST_F(BufferPoolManagerTest, UnpinAndFlushTest) {
    page_id_t page_id = 2;
    bpm_->new_page(&page_id);

    // Unpin and mark as dirty
    EXPECT_TRUE(bpm_->unpin_page(page_id, true));

    // Flush should succeed because the page was marked dirty
    EXPECT_TRUE(bpm_->flush_page(page_id));

    // A second flush should return false because the dirty flag should be reset
    EXPECT_FALSE(bpm_->flush_page(page_id));
}

TEST_F(BufferPoolManagerTest, AllPinnedNoEvictionTest) {
    page_id_t p0 = 0, p1 = 1, p2 = 2;
    
    // Fill up the pool (Pool size is 3)
    EXPECT_NE(bpm_->new_page(&p0), nullptr);
    EXPECT_NE(bpm_->new_page(&p1), nullptr);
    EXPECT_NE(bpm_->new_page(&p2), nullptr);

    // Do NOT unpin them. All pin counts should be 1.
    // Try to create a 4th page. It should fail and return nullptr.
    page_id_t p3 = 3;
    EXPECT_EQ(bpm_->new_page(&p3), nullptr);
}

TEST_F(BufferPoolManagerTest, EvictionTest) {
    page_id_t p0 = 0, p1 = 1, p2 = 2;
    
    // Fill the pool
    bpm_->new_page(&p0);
    bpm_->new_page(&p1);
    bpm_->new_page(&p2);

    // Unpin page 0 so it becomes the victim
    EXPECT_TRUE(bpm_->unpin_page(p0, false));

    // Create a new page. This should trigger the eviction of p0.
    page_id_t p3 = 3;
    Page* page3 = bpm_->new_page(&p3);
    
    ASSERT_NE(page3, nullptr);
    EXPECT_EQ(page3->get_header()->page_id, p3);

    // Fetching p0 should now cause a read from disk (and require evicting someone else)
    // First, unpin p1 to make room
    EXPECT_TRUE(bpm_->unpin_page(p1, false));
    Page* fetched_p0 = bpm_->fetch_page(p0);
    
    ASSERT_NE(fetched_p0, nullptr);
    EXPECT_EQ(fetched_p0->get_header()->page_id, p0);
}