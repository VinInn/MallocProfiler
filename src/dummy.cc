#include "mallocProfiler.h"

namespace mallocProfiler {

   bool loaded() { return false;}

   bool active(bool) { return false;}

   void activate(bool){}
   void deactivate(bool) {}
   void setThreshold(std::size_t,bool){}

   void setVerbose(bool){}
   std::size_t getThreshold() { return 0;}

   void noFinalDump(){}

   Stat summary(bool) { return Stat();}

   std::ostream &  dump(std::ostream & out, char, SortBy, bool) { return out;}
}
