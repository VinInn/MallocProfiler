#include "mallocProfiler.h"

namespace mallocProfiler {

   bool loaded() { return false;}

   bool active(bool) { return false;}

   void activate(bool){}
   void deactivate(bool) {}
   void setThreshold(std::size_t){}

   void setVerbose(bool){}

   void noFinalDump(){}

   std::ostream &  dump(std::ostream & out, char, SortBy, bool) { return out;}
}
