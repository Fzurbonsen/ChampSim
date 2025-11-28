/*
 * delta_correlation.h
 * Header file to hold the definitions of the globalhistory-buffer-based delta
 * correlation prefetcher with system awareness.
 * Author: Frederic zur Bonsen
 * E-Mail: <fzurbonsen@ethz.ch>
*/


#ifndef PREFETCHER_DELTA_CORRELATION_H
#define PREFETCHER_DELTA_CORRELATION_H

#include <cstdint>

#include "champsim.h"
#include "address.h"
#include "modules.h"
#include "cache.h"
#include "dpc_api.h"


// define system parameters
#define GHB_DEPTH 2
#define GHB_L 4 // prefetch length, the length of the memory we prefetch
#define GHB_N_MAX 6 // maximum distance (degree), i.e. how many instances do we prefetch at most
#define GHB_N_MIN 1 // minimum distance (degree), i.e. how many instances do we prefetch at least
#define GHB_N_START GHB_N_MAX // start value for the prefetch distance (degree)
#define GHB_EPOCH_LENGTH 1000 // length of a sampling epoch

// system aware decision tree config
#define GHB_ACCURACY_UPPER_THRESHOLD 0.9
#define GHB_ACCURACY_LOWER_THRESHOLD 0.5
#define GHB_MEMORY_BW_USAGE_UPPER_THRESHOLD 12
#define GHB_MEMORY_BW_USAGE_LOWER_THRESHOLD 4
#define GHB_N_VOLATILITY 1


// define the size of the global-history-buffer
#define GHB_SIZE_LOG2 8 // 8
#define GHB_SIZE (1 << GHB_SIZE_LOG2)
#define GHB_MAX_ADDR (GHB_SIZE - 1)
#define GHB_PTR_BITS (GHB_SIZE_LOG2 + 4) // Paper: We have found that increasing the width of the pointers by four-bits (the number of bits used in our simulations) makes the probability of incorrect matches very low.
#define GHB_PTR_SIZE (1 << GHB_PTR_BITS)
#define GHB_PTR_MAX_VALUE (GHB_PTR_SIZE - 1)

// define the size of the delta correlation table
#define DCT_SIZE_LOG2 8 // 8
#define DCT_SIZE (1 << DCT_SIZE_LOG2)
#define DCT_MAX_ADDR (DCT_SIZE - 1)
#define DCT_NUM_CANDIDATES 4

// define the size of the index table
#define IT_SIZE_LOG2 8 // 8
#define IT_SIZE (1 << IT_SIZE_LOG2)
#define IT_MAX_ADDR (IT_SIZE - 1)
#define IT_TAG_BITS 8 // 8
#define IT_TAG_SIZE (1 << IT_TAG_BITS)
#define IT_TAG_MAX_VALUE (IT_TAG_SIZE - 1)

// define a nullptr for the GHB
#define GHB_NULL_PTR UINT16_MAX // this value can never be reached as the maximum allowed value for any GHB-pointer is 255


// global-history-buffer entry struct
typedef struct {
	uint64_t gm_addr; // global-miss-address (this is a block/line address)
	uint16_t ghb_link; // link to previous ghb_entry
	uint16_t tag; // index table tag
	uint16_t head; // ghb_pointer that holds the head at the time off adding the entry
} ghb_entry_t;

// delta correlation table entry struct
typedef struct {
	int64_t delta;
	int64_t next_deltas[DCT_NUM_CANDIDATES];
	uint8_t counters[DCT_NUM_CANDIDATES];
	bool sorted;
} dct_entry_t;

// index table entry struct
typedef struct {
	uint16_t ghb_ptr; // pointer to the GHB entry
	uint16_t tag; // tag optained from the PC
	bool valid; // valid bit for the entry (this is only relevant until the IT table is full)
} it_entry_t;


// ghb_stride class
class delta_correlation : public champsim::modules::prefetcher
{
public:
	using prefetcher::prefetcher;

	void prefetcher_initialize();

	// void prefetcher_branch_operate(champsim::address ip,
	// 																	uint8_t branch_type,
	// 																	champsim::address branch_target);

	uint32_t prefetcher_cache_operate(champsim::address addr,
																		champsim::address ip,
																		uint8_t cache_hit,
																		bool useful_prefetch,
																		access_type type,
																		uint32_t metadata_in);

	// uint32_t prefetcher_cache_fill(champsim::address addr,
	// 																long set,
	// 																long way,
	// 																uint8_t prefetch,
	// 																champsim::address evicted_addr,
	// 																uint32_t metadata_in);

	void prefetcher_cycle_operate();

	// void prefetcher_final_stats();
	
private:
	// define GHB and IT
	ghb_entry_t ghb[GHB_SIZE];
	it_entry_t it[IT_SIZE];
	dct_entry_t dct[DCT_SIZE];
	uint16_t ghb_head;

	// system awareness
	uint16_t epoch_counter; // should never exeed 1000 therefore unit16 is enough
	int16_t prefetch_distance; // to avoid roll over errors we use int16
	int16_t n_predictions;
	uint64_t prefetch_issued_total; // tracker to calculate prefetches issued per epoch
	uint64_t prefetch_useful_total; // tracker to calcualte prefetches useful per epoch


	// helper methods to handle the GHB
	void print_ghb();
	void print_it();
	int check_dct(int64_t delta, dct_entry_t& dct_entry, bool update);
	void predict_delta(dct_entry_t& dct_entry);
	int check_ghb_pointer(uint16_t ptr, uint16_t tag);
	uint16_t sanitize_pointer(uint16_t ptr, uint16_t tag);

	int8_t prefetch_delta(dct_entry_t& dct_entry,
												int64_t& block_addr,
												int64_t& prev_delta,
												uint16_t index,
												bool update);
	// mehtod to handle prefetching
	uint16_t prefetch_history(it_entry_t& it_entry,
														uint16_t tag,
														uint64_t gm_addr);
	// method to calculate the prefetch distance (degree)
	int16_t calculate_prefetch_distance(float accuracy,
																			uint8_t memory_bw_usage);
};

#endif // PREFETCHER_DELTA_CORRELATION_H
