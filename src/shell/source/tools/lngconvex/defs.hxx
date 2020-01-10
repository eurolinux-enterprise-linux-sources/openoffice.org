#ifndef _DEFS_HXX_
#define _DEFS_HXX_

#include <vector>
#include <string>
#include <memory>

typedef std::vector<std::string> StringList_t;
typedef std::auto_ptr<StringList_t> StringListPtr_t;

typedef std::vector<int> IntegerList_t;
typedef std::auto_ptr<IntegerList_t> IntegerListPtr_t;

#endif
