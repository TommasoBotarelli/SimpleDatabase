#pragma once

#include <cstdint>
#include <array>

#include "HyperParameters.h"
#include <span>
#include <optional>

constexpr uint32_t INVALID_PAGE_ID = 0xFFFFFFFF;
using page_id_t = uint32_t;

struct PageHeader {
    page_id_t page_id;
    uint16_t tuple_count;
    uint16_t free_space_pointer;
};

struct Slot {
    uint16_t offset;
    uint16_t length;
};

class Page {
    private:
        std::array<std::byte, PAGE_SIZE> data_;
    public:
        Page();
        std::optional<uint16_t> add_row(std::span<const std::byte> row_data);
        PageHeader* get_header();
        std::span<std::byte, PAGE_SIZE> get_data();
};