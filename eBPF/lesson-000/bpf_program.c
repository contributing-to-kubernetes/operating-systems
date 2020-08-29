#include <stdio.h>
#include <linux/bpf.h>
#include "bpf_helpers.h"

#define SEC(NAME) __attribute__((section(NAME), used))

// bpf_trace_printk is defined in bpf_helpr.h.
// you can either include that header file or uncomment the following
// prototype.
//
//static int (*bpf_trace_printk)(const char *fmt, int fmt_size, ...) = (void *)BPF_FUNC_trace_printk;

// Run BPF program when a tracepoint in an scheduler switch or a kernel memory
// page allocation is detected.
//SEC("tracepoint/sched/sched_switch")
//SEC("tracepoint/kmem/mm_page_alloc")

// Run bpf program when a tracepoint in an execve system call is detected.
SEC("tracepoint/syscalls/sys_enter_execve")
int bpf_prog(void* ctx) {
  // bpf trace printk is just one way of surfacing information from an "event".
  // Other wyas of surfacing useful information are through maps or perf
  // events. bpf_trace_printk is just for debugging purposes.
  char msg[] = "";
  bpf_trace_printk(msg, sizeof(msg));

  return 0;
}

// Without the license, the kernel will not want to allow our program to be
// loaded.
char _license[] SEC("license") = "GPL";
