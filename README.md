# MallocProfiler
a profiler of "malloc" activities"
It will trace location (stacktrace) and size of memory allocations (```malloc```, etc) ofr each thread and report them (by default) the end of the process.
The dump tries to reproduce the flagraph input format https://github.com/jlfwong/speedscope/wiki/Importing-from-custom-sources#brendan-greggs-collapsed-stack-format accepted by speedscape as well

## Prerequisite
GCC12 or newer. a version not older than Nov 15, 2023.
configured with ```--enable-libstdcxx-backtrace=yes```.
This tool has been tested with GCC14.



