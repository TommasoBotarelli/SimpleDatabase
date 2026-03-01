#pragma once

#include <fstream>
#include <string>
#include <cstddef>
#include <cstdint>
#include <array>
#include <span>
#include <cstddef>


constexpr size_t PAGE_SIZE = 4096;

class DiskManager {
private:
    std::fstream db_file;
    std::string file_name;
public:
    DiskManager(const std::string& file_name);
    ~DiskManager();
    void read_page(uint32_t page_id, std::span<std::byte, PAGE_SIZE> out_buffer);
    void write_page(uint32_t page_id, std::span<std::byte, PAGE_SIZE> out_buffer);
};