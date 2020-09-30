# BPF and Maps


We moved our local copy of the linux kernel tree to the directory above this
one (`../`), hence the new includes during compilation 
```
clang -o bpf-program -lelf -I./../linux-5.3/tools/lib/bpf -L/usr/lib -lbcc_bpf map.c
```

The libraries we need are `libbpf`, from the Linux source code in
`/tools/lib/bpf`, and `libbcc` (or `libbcc_bpc` if compiled from source) from
IOVisor BCC.
This, if you are runing a 5.x kernel.
If you are using a 4.x kernel then you will find the helper functions in
`/samples/bpf/bpf_load.h`.

```
$ ./bpf-program
map updated with new element
failed to update map with new value: -1 (File exists)
failed to update map with new values: -1 (No such file or directory)
```

---

Turns out that the lovely IO Visor people build a container with bpftrace and
some other utilities.

For example, it includes all the scripts present in the bpftrace repo
https://github.com/iovisor/bpftrace#tools.

Let's say you want to trace TCP session lifespans with connection details

```
sudo ./run-bpftrace.sh tcplife.bt
```

* https://github.com/iovisor/bpftrace/blob/master/tools/tcplife.bt
* http://www.brendangregg.com/blog/2016-11-30/linux-bcc-tcplife.html
* https://manpages.debian.org/testing/bpftrace/tcplife.bt.8.en.html

If you wanted to, let's say, list all the tracepoints for all the open-like
syscalls, you could run the following:
```
sudo ./run-bpftrace.sh bpftrace -l 'tracepoint:syscalls:sys_enter_open*'
```

It is even possible to count the number of times each one of the
"sys_enter_open" like probes was called:
```
sudo ./run-bpftrace.sh bpftrace \
  -e 'tracepoint:syscalls:sys_enter_open* { @[probe] = count(); }'
```

If you recall, we already tried to see each time an open(2) syscall was
executed.
But if you tried the previous count one-liner then you might have noticed that
openat(2) is used a lot more often than open(2).
So if we wanted to trace this one wen could do the following:

```
sudo ./run-bpftrace.sh bpftrace \
  -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'
```

