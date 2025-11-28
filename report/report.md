# Report Lab 4
## Introduction
In the fourth installment of the computer architecture lab we are asked to implement a set of prefetchers in the ChampSim simulator environment.

## Warmup
In the warmup we are asked to run the baseline CPU simulation without a prefetcher attatched. In Figure 1 we can see the IPC plot for the baseline configuration. It is easy to see that there are significant differences between the applications performance. We can clearly see that charlie_0 has the highest ICU and is therefore the most performant or fastest trace, while bc-12 and bfs-14 are the least performant or the slowest. The bc-0 trace is also not far off from bc-12 and bfs-14. Further we observe that the traces from the GAP dataset are a lot slower then the traces from the charlie dataset.

![Figure 1](img/task0_no_pref_fullBW_ipc.png)

## Task 1
In the first task we are asked to implement a basic global-history-buffer-based stride prefetcher. A description of the algorithm is given in the paper by Kyle et. al.[^1]

### Implementation
The algorithm makes use of two main structures. A global-history-buffer(GHB) and an index-table(IT).
```
// global-history-buffer entry struct
typedef struct {
	uint64_t gm_addr; // global-miss-address
	uint16_t ghb_link; // link to previous ghb_entry
	uint16_t tag; // index table tag
	uint16_t head; // ghb_pointer that holds the head at the time off adding the entry
} ghb_entry_t;

// index-table entry struct
typedef struct {
	uint16_t ghb_ptr; // pointer to the GHB entry
	uint16_t tag; // tag optained from the PC
	bool valid; // valid bit for the entry
} it_entry_t;
```
On a cache access an index/tag/address triple is created. Where the index refers to the position inside the IT and is made up of the lowest 8 bits of the instruction pointer and the tag of the next higher 8 bits. The address is the block-address of the accessed cache block. The prefetcher probes the IT. If successfull it traverses the GHB to find the two previous memory accesses with the same index and tag. From these it can compute the respective strides in memory. If these match then the prefetcher prefetches the next 6 entries with the same strides. As a last step the prefetcher adds the newest cache access to its IT and GHB.

For this first implementation many values were hardcoded to show the advantage of dynamically changing them during runtime in a later task.


### Results
In Figure 2 we see the relative speedup for the implemented GHB-stride prefetcher relative to the baseline evaluated in the Warmup step of the lab. Charlie_2 and charlie_3 show the smallest speedup with 1.01 but in general it can be observed that the traces from the charlie dataset show barely any performance difference. On the other hand we see very clear improvements for the traces in the GAP dataset. The speedup is seen in bfs-14 which is the trace with the worst baseline ICU. Observe also, that no traces shows a speedup below 1. So adding a prefetcher in this system configuration shows a strict performance increase.

![Figure 2](img/task1_ghb_stride_pref_fullBW_to_no_pref_fullBW_speedup.png)

[^1]: Kyle J. Nesbit and James E. Smith. Data cache prefetching using a global history buffer. In HPCA, 2004

## Task 2
### Results
In the second task we are asked to evalueate the GHB-stride prefetcher in a system with limited memory bandwidth.
In Figure 3 we can see the speedup of the GHB-stride prefetcher relative to the baseline without a prefetcher both run in a system with a limited memory bandwidth. We can make a few important observations. (1) We can directly see that the geometric mean of the speedup has decreased indicating that the GHB-stride prefetcher does not only perform worse in absolute numbers with a limited bandwidth but also its relative performance to the baseline decreases. The biggest performance difference can be seen in the traces sssp-10 and ssp-14, with -0.11 and -0.09 respectively. The smallest performance loss can be observed with charlie_1 and charlie_2 which show a difference of -0.01. In general the traces from the charlie dataset show a smaller performance loss than those from the GAP dataset. But in for the trace charlie_3 we see a speedup of 0.99, meaning that it runs slower with the prefetcher than without.

![Figure 3](img/task2_ghb_stride_pref_limitBW_to_no_pref_limitBW_speedup.png)

### Discussion
In the data we see a performance loss in every trace. This stems from the implementation of the prefetcher. The prefetcher is not aware of the system and therefore does not know how much memory bandwidth is still free. In the implementation we hardcoded the prefetch degree meaning that the prefetcher will always prefetch the next 6 memory addresses. If the memory bus is close to or already at capacity this becomes an issue as it in the best case slows down the prefetching and in the worst case slows down the entire CPU as other memory that is needed cannot be fetched due to the bus being clogged by prefetcher requests.

## Task 3


## Task 4

## Bonus Task