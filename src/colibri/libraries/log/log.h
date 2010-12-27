//============================================================================
// log.h: Light-weight logging library
//
// (c) 2006, Michael Walter
//============================================================================

#ifndef LOG_H
#define LOG_H
#include <string>
#include <memory>
#include <boost/filesystem/path.hpp>
namespace logger
{
//----------------------------------------------------------------------------

// interface:
struct record;
class target;
void debug(const char *text);
void debugf(const char *format, ...);
void info(const char *text);
void infof(const char *format, ...);
void warn(const char *text);
void warnf(const char *format, ...);
void error(const char *text);
void errorf(const char *format, ...);
void add_target(std::shared_ptr<target>);
std::shared_ptr<target> create_debug_target();
std::shared_ptr<target> create_stdout_target();
std::shared_ptr<target> create_file_target(const boost::filesystem::wpath&);
//----------------------------------------------------------------------------


//============================================================================
// e_level
//============================================================================
enum e_level
{
  level_debug,
  level_info,
  level_warn,
  level_error,
  level_count
};
//----

const char *str(e_level level);
//--------------------------------------------------------------------------


//============================================================================
// record
//============================================================================
struct record
{
  e_level level;
  time_t time;
  std::string text;
};
//----

std::string str(const record &rec);
//----------------------------------------------------------------------------


//============================================================================
// target
//============================================================================
class target
{
public:
  // destruction
  virtual ~target();
  //--------------------------------------------------------------------------

  // logging
  virtual void log(const record&)=0;
};
//----------------------------------------------------------------------------

}
#endif
