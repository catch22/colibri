//============================================================================
// db/sqlite.h: SQLite database components
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef UTILS_DB_SQLITE_H
#define UTILS_DB_SQLITE_H
#include "../core/defs.h"
#include <memory>
#include <boost/optional.hpp>
//----------------------------------------------------------------------------

// Interface:
class sqlite_connection;
class sqlite_statement;
//----------------------------------------------------------------------------


//============================================================================
// sqlite_connection
//============================================================================
class sqlite_connection
{
public:
  // construction
  sqlite_connection(const std::wstring &filename);
  //--------------------------------------------------------------------------

  // query
  std::shared_ptr<sqlite_statement> prepare(const std::wstring &sql);
  unsigned get_num_affected_rows() const;
  //--------------------------------------------------------------------------

  // customization
  void reg_function(const wchar_t *name, unsigned num_args, void (*function)(struct sqlite3_context*, int, struct Mem**));
  //--------------------------------------------------------------------------

  // versioning
  unsigned begin_schema_update();
  void end_schema_update(const wchar_t *name, unsigned new_version);
  //--------------------------------------------------------------------------

private:
  sqlite_connection(const sqlite_connection&); // not implemented
  void operator=(const sqlite_connection&); // not implemented
  //--------------------------------------------------------------------------

  struct sqlite3 *m_sqlite;
  unsigned m_current_version;
};
//----------------------------------------------------------------------------


//============================================================================
// sqlite_statement
//============================================================================
class sqlite_statement
{
public:
  // construction, assignment and destruction
  sqlite_statement(struct sqlite3*, struct sqlite3_stmt*, const std::wstring &sql);
  ~sqlite_statement();
  //--------------------------------------------------------------------------

  // binding
  sqlite_statement &bind(unsigned idx, bool);
  sqlite_statement &bind(unsigned idx, unsigned);
  sqlite_statement &bind(unsigned idx, boost::uint64_t);
  sqlite_statement &bind(unsigned idx, float);
  sqlite_statement &bind(unsigned idx, const wchar_t*);
  sqlite_statement &bind(unsigned idx, const std::wstring&);
  sqlite_statement &bind_null(unsigned idx);
  //----

  template<typename T>
  sqlite_statement &bind(unsigned idx, const boost::optional<T> &option)
  {
    if(const T *value=option.get_ptr())
      return bind(idx, *value);
    return bind_null(idx);
  }
  //--------------------------------------------------------------------------

  // execution and iteration
  sqlite_statement &exec();
  sqlite_statement &next();
  operator const void*() const;
  //--------------------------------------------------------------------------

  // column accessors
  bool get_bool(unsigned idx=0) const;
  unsigned get_unsigned(unsigned idx=0) const;
  boost::uint64_t get_uint64(unsigned idx=0) const;
  float get_float(unsigned idx=0) const;
  std::wstring get_string(unsigned idx=0) const;
  boost::optional<unsigned> get_unsigned_option(unsigned idx=0) const;
  boost::optional<boost::uint64_t> get_uint64_option(unsigned idx=0) const;
  boost::optional<std::wstring> get_string_option(unsigned idx=0) const;
  //--------------------------------------------------------------------------

private:
  sqlite_statement(const sqlite_statement&); // not implemented
  void operator=(const sqlite_statement&); // not implemented
  //--------------------------------------------------------------------------

  struct sqlite3 *m_sqlite;
  struct sqlite3_stmt *m_stmt;
  std::wstring m_sql;
  bool m_executed;
  bool m_more;
};
//----------------------------------------------------------------------------

#endif