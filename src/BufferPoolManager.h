
#pragma once

#include <vector>
#include <unordered_map>

#include "Page.h"
#include "DiskManager.h"

using frame_id_t = uint16_t;

class BufferPoolManager {
    private:
        size_t pool_size_;

        DiskManager* disk_manager_;
        
        std::vector<Page> frames_;
        std::unordered_map<page_id_t, frame_id_t> page_table_;

        std::vector<frame_id_t> free_frames_;

        std::vector<int> pin_counts_;
        std::vector<bool> is_dirty_;
    public:
        BufferPoolManager(size_t pool_size, DiskManager* disk_manager);
        ~BufferPoolManager();

        Page* fetch_page(page_id_t page_id);

        Page* new_page(page_id_t* out_page_id);

        bool unpin_page(page_id_t page_id, bool is_dirty);

        bool flush_page(page_id_t page_id);
};