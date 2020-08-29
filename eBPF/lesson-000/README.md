# Kernel Observability

If you are into app development then you might have heard about things such as
"distributed tracing".
Well, it turns out that before there was "distributed tracing", there was just
plain tracing.

Tracing is a cool and handy thing that it's supposed to help you "understand"
what is going on in your environment.
When building something, the easiest thing is to rely on logs.
They give you some feeling for what is going on, but you can't really rely on
them unless you aggregate them and use some other complexity to analyze them.
Even then, your logs are only s clever as you were when developing your app.
So when your app hits production you will be at a loss because things that you
did not initially account for will begin to happen and you'll have to go back,
add more and more logs until something happens to make sense.

Telemetry on top of logs is a good enhancement.
Because you can correlate what you see in logs with the overall behaviour of
your app and your platform.
Are you seeing timeouts in your logs during the same periods when the use of
threds is spiking?
Or are you seeing errors opening files (that you know they exist) while the
inode usage is through the roof?

In general, things get easier the more data you have.
Telemetry is a good improvement but it is also limited by the fact that you
have to account for it ahead of time.
You gotta figure out what metrics may be useful and you gotta figure out how to
build them.
What happens if begin to see your app crashing and you are only monitoring
memory usage? You may find out after a lot of poking around that maybe, overall
memory usage wasn't the issue and it was all due to heap management: overall
memory usage was low, but your app was using all the heap it had available.
In this example, heap usage was not part of your telemetry at first.
And if you are in production, in the middle of an incident (or learning
opportunity :smile:) then you will not get the opportunity to go back in time
and begin measuring heap usage before the incident happened.

This is where tracing really shines.
Tracing is something you bake into the code you use, and may be even into the
system itself.
You literally trace your steps with this idea.
If function X handles a request then you get a trace with information about
the request (user ID, task ID, type of workload, etc.).
If you use syscall X, then you get a trace and within the trace some additional
information (e.g., process name, process ID).
The trick is to watch for the traces.
If you do then you can get a better view into your environment because now you
have data for everytime an action was performed.

To carry on with this, we will take a quick detour to define a model we will
use to explore ideas about tracing.


### Virtual Memory

when you write an app, you end up with a bunch of gibberish called source code.
At some point in its lifetime, the source code will be converted into machine
code - operations and data the CPU actually understands.
When a program is executed, the "operations and data" are read from your hard
disk, loaded into the system's main memory (your RAM) and then the necessary
parts are transported into CPU and its caches by a memory bus that moves data
between your CPU and your main memory (i.e., RAM).
See https://en.wikipedia.org/wiki/Computer_data_storage#Primary_storage

**NOTE:** nomenclature is not always clear in this matters so some quick
definitions. Main memory and RAM will be used interchangeably. Secondary
storage (non-RAM storage) will be refered as such or as "disk". A disk can then
be a hard disk drive, a solid state drive, or in older texts it can refer to
one of those drum memories that look like drums.

The main take away is that in order to execute something your program has to be
available in your system's main memory (RAM).
But big multitaskers we are, we absolutely cannot use one thing at a time.
We have to have a zoom window open, a slack app running, some 20 chrome tabs,
another 5 firefox tabs, 3 terminals with 6 tabs each, and
[skaffold](https://skaffold.dev/) running a pod in a
[kind](https://github.com/kubernetes-sigs/kind) cluster.
Most computers have a good chunk of RAM (main memory) available to them but
surely loading all of these programs at the same time would make it fill up
really fast (and crash).

Because of that, some really cool people came up with the idea of virtual
memory.
Virutal memory enables the system to "expand" its main memory by borrowing some
of the hard disk (or some other storage media).
And it does this in a way that every process running in your machine believes
it has all the RAM to itself (and that the RAM is larger than it actually is).

The way this is done is by having the virtual memory management system organize
the available memory into chunks called pages.
The memory a running process needs is allocated into a page and the page is put
into the main memory for faster access.
Whenever that memory is not needed anymore, then it is swapped with another
page - or just deallocated if nothing is needed.

Pages are always fixed-sized memory chunks.
In x86 machines, pages are most commonly 4kb (4096 bytes) in size.
Although there are also huge pages, which in x86 machines can be 2Mb or even
1Gb in size.

With this basic model we will now go back to talking about tracing and
observability :rocket:.

## Tracepoints

### Overview

To drive the importance of tracing, there is a good conversation on a patch
introducing some tracepoints into the virtual management system:
https://lwn.net/Articles/346484/

In particular, there is this question:

> Is that useful [tracepoints]?  Any time I've wanted to find out things like this, I
> just don't run other stuff on the machine at the same time.

Normally, if people wanted to tune an application, the mantra was to get a
whole machine for yourself and run your code through some test cases and see
how long it took.
Maybe apply some changes and do it all over again.

The ultimate problem with this is one of reliability: code matters when it is
in used. And when it is used, it isn't in an environment that looks anything
like the environment you used to perform benchmarks (or testing for that
matter).

Then there is the other huge problem that anything we build is a lot more
complex than the mental model we have of it - even if we are the only
developers working on it.
Our mental models only reflect what we wish happened, and sometimes what errors
we know and can predict.
There will always b a plethora of events that will cause our code to behave in
some new and interesting way.

It is for this instances (that will absolutely always happen) that we need the
tooling and the platforms to "observe" what is really going on, instead of only
looking at how far away from the desiered end-state a component is going.


We talked a lot about the importance of tracing.
Now it is time to try some of it out.

## Systemtap

If you read https://lwn.net/Articles/346484/ then you saw a mention of a
systemtap script, which we included here for your use, see
[page_allocs.stp](./page_allocs.stp).

> SystemTap is a tracing and probing tool that allows users to study and
> monitor the activities of the computer system (particularly, the kernel) in
> fine detail. It provides information similar to the output of tools like
> netstat, ps, top, and iostat, but is designed to provide more filtering and
> analysis options for collected information.
> ref: https://sourceware.org/systemtap/SystemTap_Beginners_Guide/introduction.html

Systemtap is a cool tool to explore your operating system.
However, we will not dive much into it other than right now because it is not
as secure as BPF (which we will cover next) and using it and installing it can
get complicated.

## eBPF

### Configuration

Carrying on with the knowledge that tracepoints in the kernel exist, but
without having to use systemtap.
We will now try our luck with eBPF :smie:.


First off, you will need to make sure you have installed the BPF Compiler
Collection toolkit into your machine, follow the instructions here
https://github.com/iovisor/bcc/blob/master/INSTALL.md.

This will end up installing `libbcc.so` and `libbcc_bpf.so`, if you build from
source.
If you don't build from source then please read carefully.
We will need the bpf library to link some object files down the road.

For our experiment with bpf, we will also borrow some of the sample code from
the linux kernel.
While writing this, we were using a machine with a kernel version 5.3.0.
Checking the available releases from
https://cdn.kernel.org/pub/linux/kernel/v5.x/ we found the following:
```
wget -c https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.3.tar.gz -O - | tar -xz
```

This copy of the source code for linux 5.3 is used as reference in the
[Makefile](./Makefile) included with this write up.
**remember to update the [Makefile](./Makefile) accordingly if your copy of the
linux source code is different**.


### Experiment

Now to do what we came for!
The program we will use was inspired by
https://github.com/bpftools/linux-observability-with-bpf/tree/master/code/chapter-2/hello_world
We will use the same program structure and explore other kinds of tracepoints.
Paraphrasing the flow of the program:
1. [bpf_program.c](./bpf_program.c) is the program we want to load into the bpf
   vm.
2. [loader.c](./loader.c) will help us load our program into the kernel by
   making use of the helper function `load_bpf_file` implemented by the kernel
   in `linux-5.3/samples/bpf/bpf_load.c`.

Running `make` will compile the first program and then it will compile the
loader and link it with the source code from our copy of the linux kernel and
the shared library `libbcc_bpf.so` from our compilation of BCC earlier.

There are some interesting mechanics with bpf programs.
If you noticed we use an attribute atthe beginning of
[bpf_program.c](./bpf_program.c):

> One of the best (but little known) features of GNU C is the `__attribute__`
> mechanism, which allows a developer to attach characteristics to function
> declarations to allow the compiler to perform more error checking.
> It was designed in a way to be compatible with non-GNU implementations, and
> we've been using this for years in highly portable code with very good
> results. ref: http://unixwiz.net/techtips/gnu-c-attributes.html

In our case, we define an attribute called `SEC` that we use for specifying a
license and the tracepoint of interest.
The `section` attribute, as explianed below:

> The `section` attribute specifies that a variable must be placed in a
> particular data section.
> ref: https://www.keil.com/support/man/docs/armcc/armcc_chr1359124982450.htm

will store the data we specify somewhere in the compiled code.

The `used` attribute is to make sure it stays in place even though we are not
explicitly using the information within our program:

> This function attribute informs the compiler that a static function is to be
> retained in the object file, even if it is unreferenced.
> Functions marked with `__attribute__((used))` are tagged in the object file
> to avoid removal by linker unused section removal.
> ref: https://www.keil.com/support/man/docs/armcc/armcc_chr1359124978363.htm


One could only then guess that the vbpf vm will looks for this pieces of
information within the program to validate what it is that we want to do.
On that note, let's get back to what we actually want to do: we want to see
those tracepoints happening.

If we use the tracepoint `syscalls/sys_enter_execve` then we will be ping'd
whenever our system executes an execve(2) syscall.
If you compile and run the program
```
make && sudo ./bpf-program
```

Then you will eventually see some things being printed out to the screen.
The format of the output is as follows:

* The name of the application running when the tracepoint fired
* Its PID
* The CPU it was running on, in [brackets]
* Various process context flags
* A timestamp
ref: https://lwn.net/Articles/742082/

If instead, you wanted to see every time that a new page is allocated, then
comment the `SEC` for execve and uncomment the `SEC` for `mm_page_alloc`.
Similarly, if you want to see traces for whenver a context switch happens, then
comment all other `SEC`s and uncomment the one for `sched_switch`.

To see all available tracepoints, go and check out
`/sys/kernel/debug/tracing/events`.

## Conclusion

This write up is meant to help you get some context on the usefulness of
tracing and what information to look for in order to understand your system and
the applications running on it.

The eBPF section is meant to show you the usefulness of the tool, give you a
brief overview of how it works, and tell you about
https://github.com/bpftools/linux-observability-with-bpf - the book is pretty
damn awesome, go and give it a read!

Until next time.
