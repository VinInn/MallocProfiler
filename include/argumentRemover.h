#pragma once

#include<string>

// find the closing ">" in string s starting at position pos
size_t identifyArgument(std::string const & s, size_t pos) {
  // find first
  for (;pos<s.size(); ++pos) {
    if ('<' ==s[pos] ) break;
  }
  int n=0;
  for (auto p = pos; p<s.size(); ++p) {
    if ('<' ==s[p] ) ++n;
    if ('>' ==s[p] ) --n;
    if (n==0) return p;
  }
  return std::string::npos;
}

// rmove argument that follows p
inline
bool argumentRemover(std::string & s, size_t pos) {
  auto p = identifyArgument(s,pos);
  if (p==std::string::npos) return false;
  s.erase(pos, p-pos+1);
  return true;
}

// remove all occurence of ", Sym<...>" in string s
inline 
void removeTemplate(std::string & s, std::string const & Sym)  {
  std::string target = ", " + Sym;
  size_t p;  
  while (std::string::npos != (p=s.find(target))) {
    if (!argumentRemover(s,p)) return;
  }
}
