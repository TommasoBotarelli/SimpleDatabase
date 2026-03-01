#include "DiskManager.h"

#include <iostream>
#include <algorithm>

DiskManager::DiskManager(const std::string& file_name) : file_name(file_name) {
    // Try to open the file with read, write and binary mode
    db_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);

    // If the file can't be opened then the file does not exist
    if (!db_file.is_open()) {
        std::cout << "File not found. Creation of a new file: " << file_name << std::endl;
        
        // Clean the previous flags
        db_file.clear(); 
        
        // Creates the file
        db_file.open(file_name, std::ios::out | std::ios::binary);
        db_file.close(); // Lo chiudiamo immediatamente
        
        // Open the file with the correct operation
        db_file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        
        // If it can't be opened there is a critical error
        if (!db_file.is_open()) {
            throw std::runtime_error("[ERROR] Can't open or create file " + file_name);
        }
    }
}

DiskManager::~DiskManager() {
    // RAII (Resource Acquisition Is Initialization): 
    // Close the file
    if (db_file.is_open()) {
        db_file.close();
    }
}

void DiskManager::read_page(uint32_t page_id, std::span<std::byte, PAGE_SIZE> out_buffer)
{
    std::streamoff offset = static_cast<std::streamoff>(page_id) * PAGE_SIZE;
    
    this->db_file.clear();
    this->db_file.seekg(offset, std::ios::beg);

    if (!db_file.good()) {
        std::fill(out_buffer.begin(), out_buffer.end(), std::byte{0});
        this->db_file.clear();
        return;
    }
    
    this->db_file.read(reinterpret_cast<char*>(out_buffer.data()), out_buffer.size());

    if (this->db_file.gcount() < out_buffer.size()) {
        std::streamsize bytes_read = db_file.gcount();
        std::fill(out_buffer.begin() + bytes_read, out_buffer.end(), std::byte{0});
        db_file.clear();
    }
}

void DiskManager::write_page(uint32_t page_id, std::span<std::byte, PAGE_SIZE> out_buffer)
{
    std::streamoff offset = static_cast<std::streamoff>(page_id) * PAGE_SIZE;

    this->db_file.clear();
    this->db_file.seekp(offset, std::ios::beg);

    this->db_file.write(reinterpret_cast<char*>(out_buffer.data()), out_buffer.size());
}
