#pragma once

#include<iosfwd>

namespace mallocProfiler {

   enum class SortBy {none, tot, live, max, ncalls};

   bool loaded();

   bool active(bool allThreads=true);

   void activate(bool allThreads=true);
   void deactivate(bool allThreads=true);

   void setVerbose(bool isVerbose);

   std::ostream &  dump(std::ostream &, SortBy mode, bool allThreads);

}
