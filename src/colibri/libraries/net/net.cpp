//============================================================================
// net.cpp: Networking helpers
//
// (c) Michael Walter, 2006
//============================================================================

#include "net.h"
#include "../log/log.h"
#include "../win32/win32.h"
#include <wininet.h>
#pragma comment(lib, "wininet")
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// download
//============================================================================
bool download(const std::wstring &url, buffer &content, const std::wstring &user_agent)
{
  // open internet connection
  HINTERNET inet=InternetOpenW(user_agent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
  if(!inet)
  {
    logger::warn("Unable to open internet connection.");
    return false;
  }

  // connect to url
  HINTERNET request=InternetOpenUrlW(inet, url.c_str(), 0, -1, INTERNET_FLAG_HYPERLINK|INTERNET_FLAG_NO_UI|INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD|INTERNET_FLAG_RESYNCHRONIZE|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_NO_COOKIES, 0);
  if(!request)
  {
    logger::warnf("Unable to open %S.", url.c_str());
    InternetCloseHandle(inet);
    return false;
  }

  // determine content length
  DWORD contentLength;
  DWORD bufferSize=sizeof(DWORD);
  if(!HttpQueryInfo(request, HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER, &contentLength, &bufferSize, 0) || bufferSize!=sizeof(DWORD))
  {
    logger::warnf("Unable to determine content length while downloading %S.", url.c_str());
    InternetCloseHandle(request);
    InternetCloseHandle(inet);
    return false;
  }

  // resize buffer and read data
  content.resize(contentLength);
  DWORD bytesRead=0;
  if(!InternetReadFile(request, &content[0], contentLength, &bytesRead) || bytesRead<contentLength)
  {
    logger::warnf("Unable to download %u bytes from %S (got %u bytes).", contentLength, url.c_str(), bytesRead);
    InternetCloseHandle(request);
    InternetCloseHandle(inet);
    return false;
  }

  // read 0 bytes so that the connection gets really closed
  boost::uint8_t dummy[1];
  if(!InternetReadFile(request, dummy, 1, &bytesRead) || bytesRead!=0)
  {
    logger::warnf("Unable to read last 0 bytes from %S.", contentLength, url.c_str(), bytesRead);
    InternetCloseHandle(request);
    InternetCloseHandle(inet);
    return false;
  }

  // close handles
  InternetCloseHandle(request);
  InternetCloseHandle(inet);
  return true;
}
//----------------------------------------------------------------------------