/*
 * ghb_stride.h
 * Header file to hold the definitions of the globalhistory-buffer-based stride prefetcher.
 * Author: Frederic zur Bonsen
 * E-Mail: <fzurbonsen@ethz.ch>
*/


#ifndef PREFETCHER_GHB_STRIDE_H
#define PREFETCHER_GHB_STRIDE_H

#include <cstdint>

#include "champsim.h"
#include "address.h"
#include "modules.h"


// define system parameters
#define GHB_DEPTH 2
#define GHB_L 4
#define GHB_N 6

// define the size of the global-history-buffer
#define GHB_SIZE_LOG2 8
#define GHB_SIZE (1 << GHB_SIZE_LOG2)
#define GHB_MAX_ADDR (GHB_SIZE - 1)
#define GHB_PTR_BITS (GHB_SIZE_LOG2 + 4) // Paper: We have found that increasing the width of the pointers by four-bits (the number of bits used in our simulations) makes the probability of incorrect matches very low.
#define GHB_PTR_SIZE (1 << GHB_PTR_BITS)
#define GHB_PTR_MAX_VALUE (GHB_PTR_SIZE - 1)


// define the size of the index table
#define IT_SIZE_LOG2 8
#define IT_SIZE (1 << IT_SIZE_LOG2)
#define IT_MAX_ADDR (IT_SIZE - 1)
#define IT_TAG_BITS 8
#define IT_TAG_SIZE (1 << IT_TAG_BITS)
#define IT_TAG_MAX_VALUE (IT_TAG_SIZE - 1)

// define a nullptr for the GHB
#define GHB_NULL_PTR INT16_MAX // this value can never be reached as the maximum allowed value for any GHB-pointer is 255


// global-history-buffer entry struct
typedef struct {
	uint64_t gm_addr; // global-miss-address
	uint16_t ghb_link; // link to previous ghb_entry
	uint16_t tag; // index table tag
	uint16_t head;
} ghb_entry_t;

// index table entry struct
typedef struct {
	uint16_t ghb_ptr;
	uint16_t tag;
	bool valid;
} it_entry_t;


// ghb_stride class
class ghb_stride : public champsim::modules::prefetcher
{
public:
	using prefetcher::prefetcher;

	void prefetcher_initialize();
	// void prefetcher_branch_operate(champsim::address ip, uint8_t branch_type, champsim::address branch_target) {}
	uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
																		uint32_t metadata_in);
	// uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in);
	// void prefetcher_cycle_operate() {}
	// void prefetcher_final_stats() {}
	
private:
	// define GHB and IT
	ghb_entry_t ghb[GHB_SIZE];
	it_entry_t it[IT_SIZE];
	uint16_t ghb_head;

	// helper functions to handle the GHB
	int check_ghb_pointer(uint16_t ptr, uint16_t tag);

	// function to handle prefetching
	uint16_t prefetch_history(it_entry_t& it_entry,
														uint16_t tag,
														uint64_t gm_addr);

};

#endif // PREFETCHER_GHB_STRIDE_H
