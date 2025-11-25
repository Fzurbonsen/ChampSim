/*
 * ghb_stride.cc
 * File to hold the implementation of the global-histroy-buffer-based stride prefetcher.
 * Author: Frederic zur Bonsen
 * E-Mail: <fzurbonsen@ethz.ch>
*/


#include "ghb_stride.h"
#include <iostream>
#include <iomanip>


// debug helpers
void ghb_stride::print_ghb()
{
  std::cerr << "GHB:" << std::endl;
  std::cerr << "GHB_HEAD: " << (ghb_head & GHB_MAX_ADDR) << " | " << ghb_head << std::endl; 
  for (int i = 0; i < GHB_SIZE; ++i) {
    std::cerr 
        << "0x" << std::hex << std::setw(8) << std::setfill('0') << ghb[i].gm_addr 
        << std::dec << " | "
        << ghb[i].tag << " | " 
        << ghb[i].ghb_link << " | " 
        << ghb[i].head 
        << std::endl;
  }
}

void ghb_stride::print_it()
{
  std::cerr << "IT:" << std::endl;
  for (int i = 0; i < IT_SIZE; ++i) {
    std::cerr << it[i].ghb_ptr << " | " << it[i].tag << " | " << it[i].valid << std::endl;
  }
}


// Initialize the prefetcher structures
void ghb_stride::prefetcher_initialize()
{  
  // set the head pointer in the GHB to zero
  ghb_head = 0;

  // zero the GHB
  for (ghb_entry_t& entry : ghb) {
    entry.gm_addr = 0;
    entry.ghb_link = GHB_NULL_PTR;
    entry.head = GHB_NULL_PTR;
    entry.tag = 0;
  }

  // zero the IT
  for (it_entry_t& entry : it) {
    entry.ghb_ptr = GHB_NULL_PTR;
    entry.tag = 0;
    entry.valid = false;
  }
}


// method to check whether a pointer/tag pair is valid
int ghb_stride::check_ghb_pointer(uint16_t ptr, uint16_t tag)
{
  // check if the pointer is initzialized
  if (ptr == GHB_NULL_PTR)
    return 0;

  // check if the pointer is inside of the valid range
  // std::cerr << "Distance calcualtion:" << std::endl;
  // std::cerr << ghb_head << " - " << ptr << " = " << (ghb_head - ptr) << std::endl;
  // std::cerr << "(ghb_head - ptr) & GHB_PTR_MAX_VALUE) = " << ((ghb_head - ptr) & GHB_PTR_MAX_VALUE) << std::endl;
  // std::cerr << ((ghb_head - ptr) & GHB_PTR_MAX_VALUE) << " > " << GHB_MAX_ADDR << std::endl;
  if (((ghb_head - ptr) & GHB_PTR_MAX_VALUE) > GHB_MAX_ADDR)
    return 0;

  ghb_entry_t& ghb_entry = ghb[ptr & GHB_MAX_ADDR];
  // check if we have an outdated entry
  if (ptr != ghb_entry.head)
    return 0;

  // check the tag
  if (ghb_entry.tag != tag)
    return 0;
  return 1;
}


// method to sanitize the pointer
uint16_t ghb_stride::sanitize_pointer(uint16_t ptr, uint16_t tag)
{
  return check_ghb_pointer(ptr, tag) ? ptr : GHB_NULL_PTR;
}


// method to try to prefetch the history
uint16_t ghb_stride::prefetch_history(it_entry_t& it_entry,
                                      uint16_t tag,
                                      uint64_t gm_addr)
{
  const uint16_t ghb_ptr = it_entry.ghb_ptr;

  // check if the entry is valid
  if (!it_entry.valid)
    return GHB_NULL_PTR;

  // check if the tag matches the entry
  if (tag != it_entry.tag)
    return GHB_NULL_PTR;
    
  // check if the entry points to a valid address/tag pair in the GHB
  if (!check_ghb_pointer(ghb_ptr, tag))
    return GHB_NULL_PTR;

  // init history as zero
  ghb_entry_t history[GHB_DEPTH];
  for (ghb_entry_t& entry : history) {
    entry.ghb_link = GHB_NULL_PTR;
    entry.gm_addr = 0;
    entry.head = GHB_NULL_PTR;
    entry.tag = 0;
  }
  uint16_t history_length = 0;
  uint16_t history_ptr = ghb_ptr;

  // get the history
  while ((history_length < GHB_DEPTH) && check_ghb_pointer(history_ptr, tag)) {
    ghb_entry_t& ghb_entry = ghb[history_ptr & GHB_MAX_ADDR];
    history[history_length++] = ghb_entry;
    history_ptr = sanitize_pointer(ghb_entry.ghb_link, tag);
  }

  // check the history size
  if (history_length < 2)
    return ghb_ptr;

  // int64_t as we can have both positive and negative strides
  int64_t stride1, stride2; // we only care about the first two strides due to the choice of GHB_DEPTH = 2 
                            // which gives us a history of lenght 3 with the current entry
  stride1 = static_cast<int64_t>(gm_addr) - static_cast<int64_t>(history[0].gm_addr);
  stride2 = static_cast<int64_t>(history[0].gm_addr) - static_cast<int64_t>(history[1].gm_addr);

  // check that the first stride is nonzero, i.e. that we have different addresses
  if (stride1 == 0)
    return ghb_ptr;

  // check whether the strides are identical
  if (stride1 != stride2)
    return ghb_ptr;

  // std::cerr << "Prefetching:" << std::endl;
  // std::cerr << "History:" << std::endl;
  // std::cerr << (ghb_head & GHB_MAX_ADDR) << " | " << ghb_head << std::endl;
  // std::cerr << (ghb_ptr & GHB_MAX_ADDR) << " | " << ghb_ptr << std::endl;
  // std::cerr << (history[0].ghb_link & GHB_MAX_ADDR) << " | " << history[0].ghb_link << std::endl;

  // loop over strides to prefetch
  for (int i = 0; i < GHB_N+1; ++i) {
    int64_t prefetch_block_addr = static_cast<int64_t>(gm_addr) + stride1 * static_cast<int64_t>(GHB_L + i);

    // physical addresses have to positive
    if (prefetch_block_addr < 0)
      return ghb_ptr;

    // prefetch the cache line
    uint64_t prefetch_addr = static_cast<uint64_t>(prefetch_block_addr) << LOG2_BLOCK_SIZE;
    prefetch_line(champsim::address{prefetch_addr}, true, 0);
  }
  // std::cerr << "Prefetch successful" << std::endl;
  return ghb_ptr;
}


// method to operate our prefetcher whenever a cache access happens
uint32_t ghb_stride::prefetcher_cache_operate(champsim::address addr,
                                              champsim::address ip,
                                              uint8_t cache_hit,
                                              bool useful_prefetch,
                                              access_type type,
                                              uint32_t metadata_in)
{
  uint64_t gm_addr = addr.to<uint64_t>() >> LOG2_BLOCK_SIZE;
  uint16_t it_index = static_cast<uint16_t>(ip.to<uint64_t>() & IT_MAX_ADDR); // the lower 8 bits of the ip are the position in the index table
  uint16_t tag = static_cast<uint16_t>((ip.to<uint64_t>() >> IT_SIZE_LOG2) & IT_TAG_MAX_VALUE); // the next higher 8 bits make up our tag

  // get the IT entry
  it_entry_t& it_entry = it[it_index];

  // prefetch if possible
  uint16_t ghb_ptr = prefetch_history(it_entry, tag, gm_addr);

  // write new entries
  ghb_entry_t& ghb_entry = ghb[ghb_head & GHB_MAX_ADDR]; // GHB_MAX_ADDR
  ghb_entry.ghb_link = ghb_ptr;
  ghb_entry.head = ghb_head;
  ghb_entry.gm_addr = gm_addr;
  ghb_entry.tag = tag;

  it_entry.ghb_ptr = ghb_head; // GHB_MAX_ADDR
  it_entry.tag = tag;
  it_entry.valid = 1;

  // print_ghb();
  // print_it();

  // ghb_head = (ghb_head + 1) & GHB_MAX_ADDR;
  ghb_head = (ghb_head + 1) & GHB_PTR_MAX_VALUE;

  return metadata_in;
}