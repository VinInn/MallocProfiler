# run as python3 demo.py | grep _mpTrace | cut -f1,3 -d'$' | tr '$' ' ' >& pyDemo.md
# use 2 to select number-of-calls, 3 to select total memory, 4 to select live-memory or 5 to  select max-live-memory
# mind that max-live does not aggregate properly and, in absence of memory leak, live-memory should be zero at the end of a process
import numpy as np
time = np.linspace(20, 145, 5)  # time scale
data = np.sin(np.arange(20)).reshape(5, 4)  # 4 time-dependent series
time
data
# index of the maxima for each series
ind = data.argmax(axis=0)
ind
# times corresponding to the maxima
time_max = time[ind]
data_max = data[ind, range(data.shape[1])]  # => data[ind[0], 0], data[ind[1], 1]...
time_max
data_max
np.all(data_max == data.max(axis=0))
