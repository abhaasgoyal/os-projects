#include "memsim.h"
#include <cassert>
#include <iostream>
#include <list>
#include <set>
#include <unordered_map>

struct Partition {
  int tag, size;
  int64_t addr;
  Partition(int tag, int size, int64_t addr)
  {
    this->tag = tag;
    this->size = size;
    this->addr = addr;
  }
};

typedef std::list<Partition>::iterator PartitionRef;

struct scmp {
  bool operator()(const PartitionRef & c1, const PartitionRef & c2) const
  {
    if (c1->size == c2->size) return c1->addr < c2->addr;
    else
      return c1->size > c2->size;
  }
};

class Simulator {
  private:
  int64_t page_size;
  int64_t n_req_pages = 0;

  public:
  // all partitions, in a linked list
  std::list<Partition> all_blocks;

  // sorted partitions by size/address
  std::set<PartitionRef, scmp> free_blocks;

  // quick access to all tagged partitions
  std::unordered_map<long, std::vector<PartitionRef>> tagged_blocks;

  Simulator(int64_t page_size)
  {
    // constructor
    // std :: cout << "idk";
    this->page_size = page_size;

    // Create an empty free block
    all_blocks.push_back(Partition(-1, 0, 0));
    free_blocks.insert(all_blocks.begin());
    // tagged_blocks[-1].push_back(all_blocks.begin());
  }
  void allocate(int tag, int req_size)
  {
    // Pseudocode for allocation request:
    // - search through the list of partitions from start to end, and
    //   find the largest partition that fits requested size
    //     - in case of ties, pick the first partition found
    // - if no suitable partition found:
    //     - get minimum number of pages from OS, but consider the
    //       case when last partition is free
    //     - add the new memory at the end of partition list
    //     - the last partition will be the best partition
    // - split the best partition in two if necessary
    //     - mark the first partition occupied, and store the tag in it
    //     - mark the second partition free

    PartitionRef max_free = *(free_blocks.begin());

    /* No suitable partition found */
    if (max_free->size < req_size) {
      // Iterator to final block
      auto final_block = std::prev(all_blocks.end());

      /* Getting Minimum number of pages from os */
      int64_t upd_req_size;
      if (final_block->tag == -1) {
        upd_req_size = req_size - final_block->size;
      } else {
        upd_req_size = req_size;
      }
      int upd_req_pages = upd_req_size / page_size;
      if (upd_req_size % page_size != 0) { upd_req_pages += 1; }
      int64_t upd_rem_size = upd_req_pages * page_size - upd_req_size;

      /* Final block is free */
      if (final_block->tag == -1) {
        free_blocks.erase(final_block);
        final_block->tag = tag;
        final_block->size = req_size;
        all_blocks.push_back(Partition(-1, upd_rem_size, final_block->addr + req_size));
      }
      /* Final block isn't free */
      else {
        all_blocks.push_back(Partition(tag, req_size, final_block->addr + final_block->size));
        all_blocks.push_back(
            Partition(-1, upd_rem_size, final_block->addr + final_block->size + req_size));
      }
      // Add the new free partition
      free_blocks.insert(std::prev(all_blocks.end()));
      // Pushing the second last block to tagged block
      tagged_blocks[tag].push_back(std::prev(all_blocks.end(), 2));

      // Changing total reqested pages
      n_req_pages += upd_req_pages;
    }
    /* Suitable partition found */
    else {
      /* Split best partition in 2 parts */
      // First part as occupied
      free_blocks.erase(max_free);
      all_blocks.insert(max_free, Partition(tag, req_size, max_free->addr));
      // Second part as free
      max_free->addr += req_size;
      max_free->size -= req_size;
      max_free->tag = -1;
      free_blocks.insert(max_free);
      // Pushing tagged block
      tagged_blocks[tag].push_back(std::prev(max_free));
    }
    /* Optional print stats */
    // print_stats(0, tag, req_size);
  }

  void merge_block(PartitionRef & main_ref, PartitionRef & relative_ref)
  {
    main_ref->size += relative_ref->size;
    free_blocks.erase(relative_ref);
    all_blocks.erase(relative_ref);
  }

  void deallocate(int tag)
  {
    // Pseudocode for deallocation request:
    // - for every partition
    //     - if partition is occupied and has a matching tag:
    //         - mark the partition free
    //         - merge any adjacent free partitions

    for (auto & t_block : tagged_blocks[tag]) {
      t_block->tag = -1;

      auto next_block = std::next(t_block);
      auto prev_block = std::prev(t_block);

      /* Check forward and backward for free tags and merge */
      if (t_block != all_blocks.begin() && prev_block->tag == -1) {
        merge_block(t_block, prev_block);
        t_block->addr = prev_block->addr;
      }

      if (t_block != std::prev(all_blocks.end()) && next_block->tag == -1) {
        merge_block(t_block, next_block);
      }

      free_blocks.insert(t_block);
    }
    tagged_blocks.erase(tag);
    /* Optional print stats */
    // print_stats(1, tag, -1);
  }
  MemSimResult getStats()
  {
    // let's guess the result... :)
    MemSimResult result;
    if (! free_blocks.empty()) {
      PartitionRef max_free = *(free_blocks.begin());
      result.max_free_partition_address = max_free->addr;
      result.max_free_partition_size = max_free->size;
    }
    result.n_pages_requested = this->n_req_pages;
    return result;
  }

  void print_stats(int op_type, int tag, int size)
  {
    switch (op_type) {
    case 0:
      std ::cout << "*Allocation:* T:" << tag << " S:" << size << std ::endl;
      break;
    case 1:
      std ::cout << "*Deallocation:* T:" << tag << std ::endl;
      break;
    default:
      std ::cout << "Other op";
    }

    std ::cout << "1. All blocks" << std ::endl;
    for (auto x : all_blocks) { print_part(x); }
    std ::cout << std ::endl << "2. Free Blocks" << std ::endl;
    for (auto x : free_blocks) { print_part(*x); }
    std ::cout << std ::endl << "3. Tagged Blocks" << std ::endl;
    ;
    for (auto x : tagged_blocks) {
      std ::cout << "For tag " << x.first << ":" << std ::endl;
      for (auto y : x.second) {
        print_part(*y);
        std ::cout << std ::endl;
      }
    }
  }

  void print_part(Partition p)
  {
    std ::cout << "T: " << p.tag << " S:" << p.size << " A:" << p.addr << " | ";
  }
};

// re-implement the following function
// ===================================
// parameters:
//    page_size: integer in range [1..1,000,000]
//    requests: array of requests
// return:
//    some statistics at the end of simulation
MemSimResult mem_sim(int64_t page_size, const std::vector<Request> & requests)
{
  Simulator sim(page_size);
  for (const auto & req : requests) {
    if (req.tag < 0) {
      sim.deallocate(-req.tag);
    } else {
      sim.allocate(req.tag, req.size);
    }
  }
  return sim.getStats();
}
