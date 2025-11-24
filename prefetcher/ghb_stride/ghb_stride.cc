/*
 * ghb_stride.cc
 * File to hold the implementation of the global-histroy-buffer-based stride prefetcher.
 * Author: Frederic zur Bonsen
 * E-Mail: <fzurbonsen@ethz.ch>
*/


#include "ghb_stride.h"


// Initialize the prefetcher structures
void ghb_stride::prefetcher_initialize() {
  
  // set the head pointer in the GHB to zero
  ghb_head = 0;

  // zero the GHB
  for (ghb_entry_t entry : ghb) {
    entry.gma = 0;
    entry.ghb_link = GHB_NULL_PTR;
    entry.head = GHB_NULL_PTR;
    entry.tag = 0;
  }

  // zero the IT
  for (it_entry_t entry : it) {
    entry.ghb_ptr = GHB_NULL_PTR;
    entry.tag = 0;
    entry.valid = false;
  }
}


uint32_t ghb_stride::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                      uint32_t metadata_in)
{
  // assert(addr == ip); // Invariant for instruction prefetchers
  return metadata_in;
}