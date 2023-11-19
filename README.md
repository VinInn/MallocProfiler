# MallocProfiler
a profiler of "malloc" activities".

It will trace location (stacktrace) and size of memory allocations (```malloc```, etc) for each thread and report them at the end of the process.
The dump tries to reproduce the flagraph input format https://github.com/jlfwong/speedscope/wiki/Importing-from-custom-sources#brendan-greggs-collapsed-stack-format accepted by speedscape as well.
An API is provided to configure it and get reports on user request.

## Prerequisite
GCC12 or newer. a version not older than Nov 15, 2023.
configured with ```--enable-libstdcxx-backtrace=yes```.
This tool has been tested with GCC14.

## Quick Start
clone this repository ad ```cd``` in it.

```source compile```

```export LD_PRELOAD=./mallocProfiler.so```

invoke the application to profile

```export LD_PRELOAD=""```





