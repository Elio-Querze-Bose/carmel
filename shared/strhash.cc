#include "strhash.h"

std::ostream & operator << (std::ostream & out, const StringKey &s)
{
  out << (char *)(StringKey)s;
  return out;
}

char *StringKey::empty = "";

#ifdef STRINGPOOL
HashTable<StringKey, int> StringPool::counts;
#endif
