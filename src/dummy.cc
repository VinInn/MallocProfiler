#include "mallocProfiler.h"

namespace mallocProfiler {

   bool loaded() { return false;}

   bool active(bool) { return false;}

   void activate(bool){}
   void deactivate(bool) {}

   void setVerbose(bool) {}

   std::ostream &  dump(std::ostream & out, SortBy, bool) { return out;}
}
