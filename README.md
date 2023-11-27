# MallocProfiler
a profiler of "malloc" activities".

It will trace location (stacktrace) and size of memory allocations (```malloc```, etc) for each thread and report them at the end of the process.
The dump tries to reproduce the flamegraph input format (https://github.com/jlfwong/speedscope/wiki/Importing-from-custom-sources#brendan-greggs-collapsed-stack-format) accepted by speedscope as well.
An API is provided to configure it and get reports on user request.

## Prerequisite
GCC12 or newer. A version not older than Nov 15, 2023.
configured with ```--enable-libstdcxx-backtrace=yes```.
This tool has been tested with GCC14.

## Quick Start
clone this repository ad ```cd``` in it.

```source compile```

```export LD_PRELOAD=./mallocProfiler.so```

invoke the application to profile and filter the profile using ```grep _mptrace``` selecting the field of your choice (see below)

```export LD_PRELOAD=""```

drop the resulting file in speedscope (or genarate a flamegraph svg)

### Example

go in the demos directory and run the trivial python example (taken from a numpy tutorial)
```
export LD_PRELOAD=../mallocProfiler.so
python3 demo.py | grep _mpTrace | cut -f1,3 -d'$' | tr '$' ' ' > & pyDemo.md
```
drop the resulting file (pyDemo.md) in https://www.speedscope.app

selecet the _sandwich_ view, sort by total, click on ```file_rules``` and one should get an output like this one showing the typical huge call stacks of python

<img width="1734" alt="image" src="https://github.com/VinInn/MallocProfiler/assets/4143702/a18fe3e3-c6a2-4c4b-ae78-3247e55d17f3">


## Instrumenting user code

demos/instrumentationDemo.cpp contains a simple examaple of how to instrument user code: it is supposed to track and report all allocations performed while filling a hash-map (std::unordered_map)

compile it with
```
c++ -g instrumentationDemo.cpp ../dummyMallocProfiler.so -o instrumentationDemo
```

preload the profiler disabled  by default and run it
```
export LD_PRELOAD=../mallocProfilerOFF.so
../instrumentationDemo
```

compile it again activating the ```reserve``` call
```
c++ -g instrumentationDemo.cpp ../dummyMallocProfiler.so -o instrumentationDemo -DRESERVE
```

and compare the two outputs


##  User API

The user API, to configure the profiler and to instrument the code, is all in the header file include/mallocProfiler.h
and is documented inline.

A simple mechianism to confugure the profiler w/o instrumenting the code is to introduce a _middle-library_ to be preloaded after the profiler itself.
An example can be found in tests/testConfiguration.cc




