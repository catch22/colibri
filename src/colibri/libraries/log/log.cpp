//============================================================================
// log.cpp: Light-weight logging library
//
// (c) 2006, Michael Walter
//============================================================================

#include "log.h"
#include <ctime>
#include <cstdarg>
#include <cstdio>
#include <list>
#include <boost/thread/mutex.hpp>
#include <boost/format.hpp>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef _MSC_VER
#define snprintf _snprintf
#endif
//----------------------------------------------------------------------------


//============================================================================
// <anonymous namespace>
//============================================================================
namespace
{
  // globals:
  typedef std::list<std::shared_ptr<logger::target> > targets;
  targets g_targets;
  boost::mutex g_mutex;
  //--------------------------------------------------------------------------


  //==========================================================================
  // log_impl()
  //==========================================================================
  void log_impl(logger::e_level level, const char *text)
  {
    // acquire mutex
    boost::mutex::scoped_lock lock(g_mutex);

    // log to all targets
    logger::record rec={level, time(0), text};
    for(targets::iterator iter=g_targets.begin(); iter!=g_targets.end(); ++iter)
      (*iter)->log(rec);
  }
  //--------------------------------------------------------------------------


  //==========================================================================
  // debug_target
  //==========================================================================
  class debug_target: public logger::target
  {
  public:
    // logging
    virtual void log(const logger::record &rec)
    {
#ifdef _WIN32
      OutputDebugStringA(str(rec).c_str());
      OutputDebugStringA("\n");
#endif
    }
  };
  //--------------------------------------------------------------------------


  //==========================================================================
  // stdout_target
  //==========================================================================
  class stdout_target: public logger::target
  {
  public:
    // logging
    virtual void log(const logger::record &rec)
    {
      std::puts(str(rec).c_str());
    }
  };
  //--------------------------------------------------------------------------


  //==========================================================================
  // file_target
  //==========================================================================
#ifdef _WIN32
  class file_target: public logger::target
  {
  public:
    // construction and destruction
    file_target(const boost::filesystem::wpath &path)
      :m_file(CreateFileW(path.string().c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL , 0))
    {
      if(INVALID_HANDLE_VALUE==m_file)
      {
        std::string ascii;
        for(std::wstring::const_iterator iter=path.string().begin(); iter!=path.string().end(); ++iter)
          ascii+=*iter<0x80 ? char(*iter) : '?';
        throw std::logic_error(str(boost::format("Unable to log to file '%1%'.") % ascii).c_str());
      }
    }      
    //----

    ~file_target()
    {
      CloseHandle(m_file);
    }
    //------------------------------------------------------------------------

    // logging
    virtual void log(const logger::record &rec)
    {
      // format line
      std::string line=str(rec);
      line+="\r\n";

      // write and flush
      DWORD written=0;
      WriteFile(m_file, line.c_str(), static_cast<DWORD>(line.size()), &written, 0);
      FlushFileBuffers(m_file);
    }
    //--------------------------------------------------------------------------

  private:
    HANDLE m_file;
  };
#else
  class file_target: public logger::target
  {
  public:
    // construction and destruction
    file_target(const std::string &filename)
      :m_file(std::fopen(filename.c_str(), "wb"))
    {
      if(!m_file)
        throw std::logic_error((boost::format("Unable to log to file '%s'.")%filename).str().c_str());
    }
    //----

    ~file_target()
    {
      std::fclose(m_file);
    }
    //------------------------------------------------------------------------

    // logging
    virtual void log(const logger::record &rec)
    {
      // format line
      std::string line=str(rec);
      line+="\r\n";

      // write and flush
      std::fputs(line.c_str(), m_file);
      std::fflush(m_file);
    }
    //--------------------------------------------------------------------------

  private:
    std::FILE *m_file;
  };
#endif
}
//----------------------------------------------------------------------------


//============================================================================
// debug(), debugf(), ...
//============================================================================
#define TEXTIFY(buffer, format)\
  char buffer[1024];\
  {\
    va_list args;\
    va_start(args, format);\
    vsnprintf(buffer, 1024, format, args);\
    buffer[1023]=0;\
    va_end(args);\
  }
//----

void logger::debug(const char *text)
{
  log_impl(level_debug, text);
}
//----

void logger::debugf(const char *format, ...)
{
  TEXTIFY(text, format);
  log_impl(level_debug, text);
}
//----

void logger::info(const char *text)
{
  log_impl(level_info, text);
}
//----

void logger::infof(const char *format, ...)
{
  TEXTIFY(text, format);
  log_impl(level_info, text);
}
//----

void logger::warn(const char *text)
{
  log_impl(level_warn, text);
}
//----

void logger::warnf(const char *format, ...)
{
  TEXTIFY(text, format);
  log_impl(level_warn, text);
}
//----

void logger::error(const char *text)
{
  log_impl(level_error, text);
}
//----

void logger::errorf(const char *format, ...)
{
  TEXTIFY(text, format);
  log_impl(level_error, text);
}
//----------------------------------------------------------------------------


//============================================================================
// add_target()
//============================================================================
void logger::add_target(std::shared_ptr<target> target)
{
  g_targets.push_back(target);
}
//----------------------------------------------------------------------------


//============================================================================
// create_..._target()
//============================================================================
std::shared_ptr<logger::target> logger::create_debug_target()
{
  return std::shared_ptr<logger::target>(new debug_target);
}
//----

std::shared_ptr<logger::target> logger::create_stdout_target()
{
  return std::shared_ptr<logger::target>(new stdout_target);
}
//----

std::shared_ptr<logger::target> logger::create_file_target(const boost::filesystem::wpath &path)
{
  return std::shared_ptr<logger::target>(new file_target(path));
}
//----------------------------------------------------------------------------


//============================================================================
// e_level
//============================================================================
const char *logger::str(e_level level)
{
  switch(level)
  {
  case level_debug: return "debug";
  case level_info:  return "info";
  case level_warn:  return "warn";
  case level_error: return "error";
  default:          return "unknown";
  }
}
//----------------------------------------------------------------------------


//============================================================================
// record
//============================================================================
std::string logger::str(const record &rec)
{
  // calculate date/time from timestamp
  tm *tm=std::localtime(&rec.time);

  // change level to upper case
  std::string level=str(rec.level);
  transform(level.begin(), level.end(), level.begin(), towupper);

  // format message
  char buffer[1024];
  snprintf(buffer, 1024, "%04u-%02u-%02u %02u:%02u:%02u [%s] %s",
    1900+tm->tm_year, 1+tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
    level.c_str(), rec.text.c_str());
  buffer[1023]=0;
  return buffer;
}
//----------------------------------------------------------------------------


//============================================================================
// target
//============================================================================
logger::target::~target()
{
}
//----------------------------------------------------------------------------
