/*
 * delta_correlation.cc
 * File to hold the implementation of the global-histroy-buffer-based delta
 * correlation prefetcher with system awareness to adapt the prefetch distance.
 * Author: Frederic zur Bonsen
 * E-Mail: <fzurbonsen@ethz.ch>
*/


#include "delta_correlation.h"
#include <iostream>
#include <iomanip>


// debug helpers
void delta_correlation::print_ghb()
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

void delta_correlation::print_it()
{
  std::cerr << "IT:" << std::endl;
  for (int i = 0; i < IT_SIZE; ++i) {
    std::cerr << it[i].ghb_ptr << " | " << it[i].tag << " | " << it[i].valid << std::endl;
  }
}


// Initialize the prefetcher structures
void delta_correlation::prefetcher_initialize()
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

  // zero the DCT
  for (dct_entry_t& entry : dct) {
    entry.delta = 0;
    for (int64_t& next_delta : entry.next_deltas) {
      next_delta = 0;
    }
    for (uint8_t& counter : entry.counters) {
      counter = 0;
    }
    entry.sorted = true;
  }

  // zero the IT
  for (it_entry_t& entry : it) {
    entry.ghb_ptr = GHB_NULL_PTR;
    entry.tag = 0;
    entry.valid = false;
  }

  // system awarness parameters
  epoch_counter = 0;
  prefetch_distance = GHB_N_START;
  prefetch_issued_total = 0;
  prefetch_useful_total = 0;
}


// method to check if a delta already exists
int delta_correlation::check_dct(int64_t delta, dct_entry_t& dct_entry, bool update)
{  
  uint16_t min_counter_idx = 0;

  // iterate over all candidates to check if the delta is alreay there
  for (uint16_t i = 0; i < DCT_NUM_CANDIDATES; ++i) {
    if (delta == dct_entry.next_deltas[i]) {
      if (update) {
        dct_entry.counters[i] = dct_entry.counters[i] < 255 ? dct_entry.counters[i] + 1 : dct_entry.counters[i];
        dct_entry.sorted = false;
      }
      return 1;
    }
    min_counter_idx = dct_entry.counters[i] <= dct_entry.counters[min_counter_idx] ? i : min_counter_idx;
  }

  // evict the candidate with the lowest counter
  if (update) {
    dct_entry.counters[min_counter_idx] = 1;
    dct_entry.next_deltas[min_counter_idx] = delta;
    dct_entry.sorted = false;
  }
  return 0;
}


// method to find the most likely delta
int64_t delta_correlation::predict_delta(dct_entry_t& dct_entry)
{
  uint16_t max_counter_idx = 0;

  // find the delta with the highest count
  for (uint16_t i = 0; i < DCT_NUM_CANDIDATES; ++i) {
    max_counter_idx = dct_entry.counters[i] > dct_entry.counters[max_counter_idx] ? i : max_counter_idx;
  }

  // return the most common delta
  return dct_entry.next_deltas[max_counter_idx];
}


// method to sort the dct_entry candidates by likelyhood
void delta_correlation::sort_predictions(dct_entry_t& dct_entry)
{
  if (dct_entry.sorted)
    return;

  // as the array is never big we can use insertion sort
  for (size_t i = 1; i < DCT_NUM_CANDIDATES; ++i) {
    uint8_t key = dct_entry.counters[i];
    int64_t delta = dct_entry.next_deltas[i];
    size_t j = i;
    while(j > 0 && dct_entry.counters[j - 1] > key) {
      dct_entry.counters[j] = dct_entry.counters[j - 1];
      dct_entry.next_deltas[j] = dct_entry.next_deltas[j - 1];
      --j;
    }
    dct_entry.counters[j] = key;
    dct_entry.next_deltas[j] = delta;
  }
  dct_entry.sorted = true;
}


// method to check whether a pointer/tag pair is valid
int delta_correlation::check_ghb_pointer(uint16_t ptr, uint16_t tag)
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
uint16_t delta_correlation::sanitize_pointer(uint16_t ptr, uint16_t tag)
{
  return check_ghb_pointer(ptr, tag) ? ptr : GHB_NULL_PTR;
}


// method to try to prefetch the history
uint16_t delta_correlation::prefetch_history(it_entry_t& it_entry,
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

  // calculate the delta
  int64_t delta1 = static_cast<int64_t>(gm_addr) - static_cast<int64_t>(history[0].gm_addr);
  int64_t delta2 = static_cast<int64_t>(history[0].gm_addr) - static_cast<int64_t>(history[1].gm_addr);

  // check that the first delta is non-zero
  if (delta1 == 0)
    return ghb_ptr;

  // generate the dct entry
  dct_entry_t& dct_entry = dct[(delta2 + DCT_SIZE/2) & DCT_MAX_ADDR];

  if(check_dct(delta1, dct_entry, true))
    return ghb_ptr;

  int64_t prefetch_block_addr = gm_addr;

  dct_entry = dct[(delta1 + DCT_SIZE/2) & DCT_MAX_ADDR];
  
  // loop to prefetch multiple deltas
  for (int16_t i = 0; i < prefetch_distance+1; ++i) {
    // calculate the most likely next delta
    sort_predictions(dct_entry);
    int64_t predicted_delta = predict_delta(dct_entry);
    prefetch_block_addr += predicted_delta;

    // physical addresses have to be positive
    if (prefetch_block_addr < 0)
      return ghb_ptr;

    // prefetch the cache line
    uint64_t prefetch_addr = static_cast<uint64_t>(prefetch_block_addr) << LOG2_BLOCK_SIZE;
    prefetch_line(champsim::address{prefetch_addr}, true, 0);

    // update dct_entry
    dct_entry = dct[(predicted_delta + DCT_SIZE/2) & DCT_MAX_ADDR];
    if (!check_dct(predicted_delta, dct_entry, false))
      return ghb_ptr;
  }
  return ghb_ptr;
}


// method to operate our prefetcher whenever a cache access happens
uint32_t delta_correlation::prefetcher_cache_operate(champsim::address addr,
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
  ghb_entry_t& ghb_entry = ghb[ghb_head & GHB_MAX_ADDR];
  ghb_entry.ghb_link = ghb_ptr;
  ghb_entry.head = ghb_head;
  ghb_entry.gm_addr = gm_addr;
  ghb_entry.tag = tag;

  it_entry.ghb_ptr = ghb_head;
  it_entry.tag = tag;
  it_entry.valid = 1;

  // print_ghb();
  // print_it();

  // ghb_head = (ghb_head + 1) & GHB_MAX_ADDR;
  ghb_head = (ghb_head + 1) & GHB_PTR_MAX_VALUE;

  return metadata_in;
}


// method to calculate the prefetch distance
int16_t delta_correlation::calculate_prefetch_distance(float accuracy, uint8_t memory_bw_usage)
{
  if (memory_bw_usage >= GHB_MEMORY_BW_USAGE_UPPER_THRESHOLD) {
    if (accuracy >= GHB_ACCURACY_UPPER_THRESHOLD) 
      return prefetch_distance;
    if (accuracy >= GHB_ACCURACY_LOWER_THRESHOLD)
      return prefetch_distance - 1*GHB_N_VOLATILITY;
    return prefetch_distance - 2*GHB_N_VOLATILITY;
  }

  if (memory_bw_usage >= GHB_MEMORY_BW_USAGE_LOWER_THRESHOLD) {
    if (accuracy >= GHB_ACCURACY_UPPER_THRESHOLD) 
      return prefetch_distance + 1*GHB_N_VOLATILITY;
    if (accuracy >= GHB_ACCURACY_LOWER_THRESHOLD)
      return prefetch_distance;
    return prefetch_distance - 1*GHB_N_VOLATILITY;
  }

  if (accuracy >= GHB_ACCURACY_UPPER_THRESHOLD) 
    return prefetch_distance + 2*GHB_N_VOLATILITY;
  if (accuracy >= GHB_ACCURACY_LOWER_THRESHOLD)
    return prefetch_distance + 1*GHB_N_VOLATILITY;
  return prefetch_distance;
}


// method to perform prefetcher operations every cycle
void delta_correlation::prefetcher_cycle_operate()
{
  epoch_counter++; // increase the epoch counter every cycle

  // check if an epoch has passed
  if (epoch_counter < GHB_EPOCH_LENGTH)
    return;
  // reset epoch
  epoch_counter = 0;

  // fetch new prefetch stats
  int64_t prefetch_issued = intern_->sim_stats.pf_issued;
  int64_t prefetch_useful = intern_->sim_stats.pf_useful;

  // calculate prefetch stats for the last epoch
  int64_t prefetch_issued_epoch = prefetch_issued - prefetch_issued_total;
  int64_t prefetch_useful_epoch = prefetch_useful - prefetch_useful_total;

  // update prefetch trackers
  prefetch_issued_total = prefetch_issued;
  prefetch_useful_total = prefetch_useful;

  float prefetch_accuracy = (float)prefetch_useful_epoch / (float)prefetch_issued_epoch;
  uint8_t memory_bw_usage = get_dram_bw();

  // calculate the prefetch distance
  prefetch_distance = calculate_prefetch_distance(prefetch_accuracy, memory_bw_usage);
  
  // check the prefetch distance bounds
  if (prefetch_distance < GHB_N_MIN)
    prefetch_distance = GHB_N_MIN;
  if (prefetch_distance > GHB_N_MAX)
    prefetch_distance = GHB_N_MAX;
}