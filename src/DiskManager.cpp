#include "DiskManager.h"

#include <iostream>
#include <algorithm>

DiskManager::DiskManager(const std::string& file_name) : file_name_(file_name) {
    // Try to open the file with read, write and binary mode
    db_file_.open(file_name, std::ios::in | std::ios::out | std::ios::binary);

    // If the file can't be opened then the file does not exist
    if (!db_file_.is_open()) {
        std::cout << "File not found. Creation of a new file: " << file_name << std::endl;
        
        // Clean the previous flags
        db_file_.clear(); 
        
        // Creates the file
        db_file_.open(file_name, std::ios::out | std::ios::binary);
        db_file_.close(); // Lo chiudiamo immediatamente
        
        // Open the file with the correct operation
        db_file_.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        
        // If it can't be opened there is a critical error
        if (!db_file_.is_open()) {
            throw std::runtime_error("[ERROR] Can't open or create file " + file_name);
        }
    }
}

DiskManager::~DiskManager() {
    // RAII (Resource Acquisition Is Initialization): 
    // Close the file
    if (db_file_.is_open()) {
        db_file_.close();
    }
}

void DiskManager::read_page(uint32_t page_id, std::span<std::byte, PAGE_SIZE> out_buffer)
{
    std::streamoff offset = static_cast<std::streamoff>(page_id) * PAGE_SIZE;
    
    db_file_.clear();
    db_file_.seekg(offset, std::ios::beg);

    if (!db_file_.good()) {
        std::fill(out_buffer.begin(), out_buffer.end(), std::byte{0});
        db_file_.clear();
        return;
    }
    
    db_file_.read(reinterpret_cast<char*>(out_buffer.data()), out_buffer.size());

    if (db_file_.gcount() < out_buffer.size()) {
        std::streamsize bytes_read = db_file_.gcount();
        std::fill(out_buffer.begin() + bytes_read, out_buffer.end(), std::byte{0});
        db_file_.clear();
    }
}

void DiskManager::write_page(uint32_t page_id, std::span<std::byte, PAGE_SIZE> out_buffer)
{
    std::streamoff offset = static_cast<std::streamoff>(page_id) * PAGE_SIZE;

    db_file_.clear();
    db_file_.seekp(offset, std::ios::beg);

    db_file_.write(reinterpret_cast<char*>(out_buffer.data()), out_buffer.size());
}
