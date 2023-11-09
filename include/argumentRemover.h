#pragma once

#include<string>

// find the closing ">" in string s starting at position pos
size_t argumentRemover(std::string const & s, size_t pos) {
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


