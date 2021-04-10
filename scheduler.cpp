// you need to modify this file

#include "scheduler.h"
#include "common.h"
#include <iostream>
#include <queue>
// this is the function you should edit
//
// runs Round-Robin scheduling simulator
// input:
//   quantum = time slice
//   max_seq_len = maximum length of the reported executing sequence
//   processes[] = list of process with populated IDs, arrival_times, and bursts
// output:
//   seq[] - will contain the execution sequence but trimmed to max_seq_len size
//         - idle CPU will be denoted by -1
//         - other entries will be from processes[].id
//         - sequence will be compressed, i.e. no repeated consecutive numbers
//   processes[]
//         - adjust finish_time and start_time for each process
//         - do not adjust other fields
//

#define INVALID_TIME -1

struct Process_State {
  struct Process *process;
  int64_t rem_burst_t;
  bool completed;
  bool added_to_queue;
  Process_State(struct Process &process) {
    this->process = &process;
    this->rem_burst_t = process.burst;
    this->completed = this->added_to_queue = false;
    this->process->start_time = this->process->finish_time = INVALID_TIME;
  }
};

/* Add starting/finishing time only when they haven't been initialized */
void update_time(int64_t &old_t, int64_t new_t) {
  if (old_t == INVALID_TIME) {
    old_t = new_t;
  }
}

/* Push to sequence keeping in mind the maximum sequence size allowed */
void max_seq_push(std::vector<int> &seq, int64_t max_seq_len, int val) {
  if ((seq.size() < (unsigned long)max_seq_len && seq.back() != val)
      || seq.size() == 0) {
    seq.push_back(val);
  }
}

/* Add processes to queue after arrival time for the first time */
void new_arrivals(std::vector<Process_State> &process_states,
                  std::queue<int> &q, int curr_t) {
  for (auto &ps : process_states) {
    if (curr_t >= ps.process->arrival_time && !ps.added_to_queue) {
      q.push(ps.process->id);
      ps.added_to_queue = true;
    }
  }
}
/* Wrapper around new_arrivals for ranged queries */
void ranged_arrivals(std ::vector<Process_State> &process_states,
                     std ::queue<int> &q, int curr_t, int start, int end) {
  for (int i = start; i <= end; i++) {
    new_arrivals(process_states, q, curr_t + i);
  }
}
/* Clearing a queue */
void clear(std::queue<int> &q) {
  std::queue<int> empty;
  std::swap(q, empty);
}

/* Checking whether all the processes are completed */
bool completed_process_check(std::vector<Process_State> process_states) {
  for (auto &ps : process_states) {
    if (!ps.completed) {
      return false;
    }
  }
  return true;
}

void simulate_rr(int64_t quantum, int64_t max_seq_len,
                 std::vector<Process> &processes, std::vector<int> &seq) {

  seq.clear();
  std::vector<Process_State> ps;
  for (auto &p : processes) {
    ps.push_back(Process_State(p));
  }
  int curr_t = 0;
  bool completed_processes = false;
  std ::queue<int> process_order;

  // new_arrivals_to_queue(ps, process_order, curr_t);
  int x = 0;
  // std :: cout << process_order.front() << std :: endl;
  while (!completed_processes) {
    x++;
    completed_processes = true;
    new_arrivals(ps, process_order, curr_t);
    std ::queue<int> temp_order = process_order;
    clear(process_order);

    if (!temp_order.empty()) {
      while (!temp_order.empty()) {

        int id = temp_order.front();
        // std :: cout << id << std :: endl;
        update_time(ps[id].process->start_time, curr_t);

        if (ps[id].rem_burst_t > quantum) {
          ranged_arrivals(ps, process_order, curr_t, 0, quantum - 1);
          curr_t += quantum;
          ps[id].rem_burst_t -= quantum;
          process_order.push(id);
          new_arrivals(ps, process_order, curr_t);
        } else {
          ranged_arrivals(ps, process_order, curr_t, 0, ps[id].rem_burst_t);
          curr_t += ps[id].rem_burst_t;
          ps[id].rem_burst_t = 0;
          ps[id].completed = true;
          update_time(ps[id].process->finish_time, curr_t);
        }

        max_seq_push(seq, max_seq_len, id);

        temp_order.pop();
      }
    }
    // In case all processes are currently waiting to arrive
    else {
      curr_t += 1;
      max_seq_push(seq, max_seq_len, -1);
    }

    if (!completed_process_check(ps)) {
      completed_processes = false;
    }
  }
}
