#pragma once

#include<iosfwd>

namespace mallocProfiler {

   enum class SortBy {none, tot, live, max, ncalls};

   bool loaded();

   bool active(bool allThreads=true);

   void activate(bool allThreads=true);
   void deactivate(bool allThreads=true);

   void setThreshold(std::size_t value);

   void setVerbose(bool isVerbose);

   void noFinalDump();

   std::ostream &  dump(std::ostream &, char separator, SortBy mode, bool allThreads);

}
