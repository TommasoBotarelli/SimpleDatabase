#include "Page.h"

Page::Page()
{
    PageHeader* header = reinterpret_cast<PageHeader*>(data_.data());

    header->page_id = INVALID_PAGE_ID;
    header->tuple_count = 0;
    header->free_space_pointer = PAGE_SIZE;
}

std::optional<uint16_t> Page::add_row(std::span<const std::byte> row_data)
{
    PageHeader* header = this->get_header();

    // Check if the row can be added
    size_t slots_end_offset = sizeof(PageHeader) + header->tuple_count * sizeof(Slot);
    size_t required_space = sizeof(Slot) + row_data.size_bytes();
    size_t available_space = header->free_space_pointer - slots_end_offset;

    if (available_space < required_space) {
        return std::nullopt;
    }

    // Creation of the new slot to be inserted
    Slot new_slot;
    new_slot.length = static_cast<uint16_t>(row_data.size_bytes());
    new_slot.offset = header->free_space_pointer - new_slot.length;

    // Fill the data at the correct offset
    auto destination_offset = data_.begin() + new_slot.offset;
    std::copy(
        row_data.begin(),
        row_data.end(),
        destination_offset
    );

    // Save the index of the inserted tuple
    uint16_t slot_id = header->tuple_count;

    // Save the data of the new slot
    Slot* slot_array = reinterpret_cast<Slot*>(data_.data() + sizeof(PageHeader));
    slot_array[slot_id] = new_slot;
    
    // Update the header
    header->free_space_pointer = header->free_space_pointer - new_slot.length;
    header->tuple_count++;
    
    return slot_id;
}

PageHeader *Page::get_header()
{
    return reinterpret_cast<PageHeader*>(data_.data());
}
