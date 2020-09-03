#include <stdio.h>
#include <linux/bpf.h>
#include "bpf_helpers.h"


/*
 * sping locks only work on hash, array, and cgroup storage maps.
 *
 * User-space programs can use the flag BPF_F_LOCK to change the state of the
 * spin lock. The BPF_F_LOCK flag can be passed to the bpf_map_update_elem or
 * bpf_map_lookup_elem_flags helper functions.
 */
struct concurrent_element {
  struct bpf_spin_lock semaphore;
  int count;
}

struct bpf_map_def SEC("maps") concurrent_map {
  .type        = BPF_MAP_TYPE_HASH,
  .key_size    = sizeof(int),
  .value_size  = sizeof(struct concurrent_element),
  .max_entries = 100,
}

// In order to hold our "concurrent_element"s, we will need to annotate our map
// so that the BTF verifier knows how to interpret the structure.
BPF_ANNOTATE_KV_PAIR(concurrent_map, int, struct concurrent_element);

int bpf_prog(struct pt_regs *ctx) {
  int key = 0;
  struct concurrent_element init_value = {};
  struct concurrent_element *read_value;

  bpf_map_create_elem(&concurrent_map, &key, &init_value, BPF_NOEXIST);

  read_value = bpf_map_lookup_elem(&concurrent_map, &key);
  bpf_spin_lock(&read_value->semaphore);
  read_value->count += 100;
  bpf_spin_unlock(&read_value->semaphore);

  return 0;
}

// Without the license, the kernel will not want to allow our program to be
// loaded.
char _license[] SEC("license") = "GPL";
