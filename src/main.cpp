#include <iostream>
#include <memory>

#include "DiskManager.h"

int main(int, char**){
    auto disk_manager = std::make_unique<DiskManager>("data.bin");
    auto out_buffer = std::make_unique<std::array<std::byte, PAGE_SIZE>>();

    disk_manager->read_page(0, *out_buffer);
}
