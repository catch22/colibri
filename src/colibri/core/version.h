// Copyright (C) 2005, 2006, 2007 Michael Walter

#ifndef COLIBRI_CORE_VERSION_H
#define COLIBRI_CORE_VERSION_H

#include <string>
#include <sstream>
#include <boost/format.hpp>
#include "../resources.h"


// version_type
enum version_type
{
  version_type_alpha,
  version_type_beta,
  version_type_release,
  version_type_count
};


// version
struct version
{
  // construction
  version()
  {
  }

  version(unsigned number, version_type type)
    :number(number)
    ,type(type)
  {
  }

  // attributes
  unsigned number;
  version_type type;
};

inline bool operator>(const version &a, const version &b)
{
  return a.number>b.number || (a.number==b.number && a.type>b.type);
}

inline std::wstring str(const version &v)
{
  boost::wformat fmt(L"%i %s");
  fmt % v.number;
  fmt % std::wstring(v.type==version_type_alpha ? L"\u03b1" : v.type==version_type_beta ? L"\u03b2" : L"");
  return fmt.str();
}


// colibri_version
inline version colibri_version()
{
  return version(COLIBRI_VERSION_NUMBER, version_type(COLIBRI_VERSION_TYPE));
}

#endif
