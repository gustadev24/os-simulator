#ifndef PAGE_HPP
#define PAGE_HPP

namespace OSSimulator {

struct Page {
    int page_id;
    int frame_number; // -1 if not in memory
    bool valid;       // is in memory?
    bool modified;    // dirty bit
    bool referenced;  // for NRU
    int last_access_time; // for LRU
    int process_id;

    Page() : page_id(-1), frame_number(-1), valid(false), modified(false), referenced(false), last_access_time(0), process_id(-1) {}
    Page(int id) : page_id(id), frame_number(-1), valid(false), modified(false), referenced(false), last_access_time(0), process_id(-1) {}
};

}

#endif
