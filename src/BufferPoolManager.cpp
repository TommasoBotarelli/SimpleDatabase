#include "BufferPoolManager.h"

frame_id_t INVALID_FRAME_ID = -1;

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager) : pool_size_(pool_size), disk_manager_(disk_manager)
{
    frames_.resize(pool_size_);
    pin_counts_.resize(pool_size_, 0);
    is_dirty_.resize(pool_size_, false);
    
    for (size_t i = 0; i < pool_size_; ++i) {
        free_frames_.push_back(i);
    }
}

BufferPoolManager::~BufferPoolManager()
{
    // Itera su tutte le pagine attualmente caricate nella page_table_
    for (const auto& pair : page_table_) {
        page_id_t page_id = pair.first;
        frame_id_t frame_id = pair.second;

        // Se la pagina è stata modificata rispetto alla versione su disco,
        // dobbiamo forzarne la scrittura (flush) per non perdere i dati.
        if (is_dirty_[frame_id]) {
            flush_page(page_id);
        }
    }

    // Nota 1: Le strutture dati della STL (std::vector, std::unordered_map) 
    // chiameranno automaticamente i propri distruttori liberando la memoria della RAM.
    
    // Nota 2: NON facciamo "delete disk_manager_;" perché il BPM riceve il 
    // puntatore nel costruttore (Dependency Injection). Il BPM "usa" il DiskManager, 
    // ma non lo "possiede". Sarà chi ha creato il DiskManager a doverlo deallocare.
}

Page *BufferPoolManager::fetch_page(page_id_t page_id)
{
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        // We found the page in RAM!
        frame_id_t frame_id = it->second;

        pin_counts_[frame_id]++;

        return &frames_[frame_id];
    }

    // The page isn't in the RAM so we need to read it from disk
    // First we search for a frame id that can be trashed out
    frame_id_t victim_frame_id = INVALID_FRAME_ID;

    // TODO: a more sophisticated search can be implemented
    for (size_t i = 0; i < pool_size_; i++) {
        if (pin_counts_[i] == 0) {
            victim_frame_id = i;
            break;
        }
    }

    // If all the pin counts are > 0 then all the pages are used at the moment.
    // We left the management of this case to the caller
    if (victim_frame_id == INVALID_FRAME_ID) {
        return nullptr;
    }

    // Now i have an id of the frame that can be eliminated
    // I need to get the page connected to that frame
    page_id_t victim_page_id = INVALID_PAGE_ID;

    for (auto pair : page_table_) {
        if (pair.second == victim_frame_id) {
            victim_page_id = pair.first;
            break;
        }
    }
    
    if (victim_page_id != INVALID_PAGE_ID) {
        // We need to check if the page is dirty
        // in this case tha page should be saved before overwrite
        if (is_dirty_[victim_frame_id]) {
            disk_manager_->write_page(victim_page_id, frames_[victim_frame_id].get_data());
        }

        page_table_.erase(victim_page_id);
    }

    // At this point we can read the page from disk
    disk_manager_->read_page(page_id, frames_[victim_frame_id].get_data());

    // We can now update the metadata of this class
    is_dirty_[victim_frame_id] = false;
    pin_counts_[victim_frame_id] = 1;
    page_table_[page_id] = victim_frame_id;

    return &frames_[victim_frame_id];
}

/**
 * It creates a new page in the RAM. If the RAM is full at the moment
 * 
 */
Page *BufferPoolManager::new_page(page_id_t* out_page_id)
{
    // In this case we don't have free space in RAM
    // We need to remove a page from the RAM and then use the same frame_id
    if (free_frames_.empty()) {
        frame_id_t victim_frame_id = INVALID_FRAME_ID;

        // TODO: a more sophisticated search can be implemented
        for (size_t i = 0; i < pool_size_; i++) {
            if (pin_counts_[i] == 0) {
                victim_frame_id = i;
                break;
            }
        }

        // If all the pin counts are > 0 then all the pages are used at the moment.
        // We cannot delete any page from the RAM
        // We left the management of this case to the caller
        if (victim_frame_id == INVALID_FRAME_ID) {
            return nullptr;
        }

        // We need to find the page_id associated to victim_frame_id
        page_id_t victim_page_id = INVALID_PAGE_ID;

        for (auto pair : page_table_) {
            if (pair.second == victim_frame_id) {
                victim_page_id = pair.first;
                break;
            }
        }

        // Here we have found a correct victim_frame_id and the associated victim_page_id
        // We have to check if the bit is_dirty associates is true.
        // If so we need to save the the page to the disk
        if (is_dirty_[victim_frame_id]) {
            disk_manager_->write_page(victim_page_id, frames_[victim_frame_id].get_data());
        }

        page_table_.erase(victim_page_id);
        // To use the following code without repetition
        // we free an index in free_frames_ vector
        free_frames_.push_back(victim_frame_id);
    }
    // Here we have space in RAM (both because we found an index to remove
    // and because we had free space from the first moment)
    // Now is simple: we just pop from the free_frames_ vector
    // an index and then create a Page and save metadata
    frame_id_t frame_id = free_frames_[free_frames_.size()-1];
    free_frames_.pop_back();

    page_table_[*out_page_id] = frame_id;
    pin_counts_[frame_id] = 1;
    is_dirty_[frame_id] = false;

    frames_[frame_id] = Page();
    frames_[frame_id].get_header()->page_id = *out_page_id;

    return &frames_[frame_id];
}

/**
 * If the caller do an update operation in the page then it should call:
 * unpin_page(page_id, true)
 * 
 * For read operation it can call:
 * unpin_page(page_id, false)
 */
bool BufferPoolManager::unpin_page(page_id_t page_id, bool is_dirty)
{
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        return false;
    }

    frame_id_t frame_id = it->second;

    if (pin_counts_[frame_id] <= 0) {
        return false;
    }

    pin_counts_[frame_id]--;

    if (is_dirty) {
        is_dirty_[frame_id] = true;
    }

    return true;
}

/**
 * This method is used to save the page to the disk
 */
bool BufferPoolManager::flush_page(page_id_t page_id)
{
    // First we need the associated frame id
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        // The caller ask to flush a page that it isn't in RAM
        return false;
    }

    frame_id_t frame_id = it->second;
    
    // We can now check if the page is dirty
    // We only save the page if it is dirty
    if (!is_dirty_[frame_id]) {
        return false;
    }

    // If we arrive here the page is dirty so we need to save it to the disk
    disk_manager_->write_page(page_id, frames_[frame_id].get_data());

    is_dirty_[frame_id] = false;

    return true;
}
