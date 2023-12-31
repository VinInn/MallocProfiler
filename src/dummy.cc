#include "mallocProfiler.h"

namespace mallocProfiler {

   bool loaded() { return false;}

   bool active(bool) { return false;}
   bool enabled () { return false;}

   void enable (){}
   void activate(bool){}
   void disable () {}
   void deactivate(bool) {}
   void clear(bool){}
   void setThreshold(std::size_t,bool){}

  void setRemangler(remangleType){}
  remangleType getDefaultRemangler(){return nullptr;}
  remangleType getCurrentRemangler(){return nullptr;}

   void setVerbose(bool){}
   std::size_t getThreshold() { return 0;}

   void noFinalDump(){}

   void noZeroLiveDump(bool){}

   Stat summary(bool) { return Stat();}
   Hist memTotHistogram() { return Hist();}
   Hist memLiveHistogram() { return Hist();}


   std::ostream &  dump(std::ostream & out, char, SortBy, bool) { return out;}
   std::ostream &  dumpDetails(std::ostream & out, std::string const &, bool) { return out;}

   void setDumpingInterval(std::chrono::seconds const &, std::chrono::seconds const &) {}

}
