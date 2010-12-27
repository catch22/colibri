//============================================================================
// db/sqlite.cpp: SQLite database components
//
// (c) Michael Walter, 2006
//============================================================================

#include "sqlite.h"
#include "../../thirdparty/sqlite/sqlite3.h"
#include "../log/log.h"
using namespace std;
using namespace boost;
//----------------------------------------------------------------------------


//============================================================================
// sqlite_connection
//============================================================================
sqlite_connection::sqlite_connection(const wstring &filename)
  :m_current_version(0)
{
  // open sqlite database
  int result=sqlite3_open16(filename.c_str(), &m_sqlite);

  // handle errors
  if(SQLITE_OK!=result)
  {
    sqlite3_close(m_sqlite);
    throw_errorf("Unable to open SQLite database: %S", filename);
  }
}
//----------------------------------------------------------------------------

std::shared_ptr<sqlite_statement> sqlite_connection::prepare(const wstring &sql)
{
  // prepare statement
  sqlite3_stmt *stmt;
  const void *tail;
  if(SQLITE_OK!=sqlite3_prepare16(m_sqlite, sql.c_str(), -1, &stmt, &tail))
    throw_errorf("Unable to prepare SQL query '%S': %S", sql.c_str(), sqlite3_errmsg16(m_sqlite));
  return std::shared_ptr<sqlite_statement>(new sqlite_statement(m_sqlite, stmt, sql));
}
//----

unsigned sqlite_connection::get_num_affected_rows() const
{
  return sqlite3_changes(m_sqlite);
}
//----------------------------------------------------------------------------

void sqlite_connection::reg_function(const wchar_t *name, unsigned num_args, void (*function)(sqlite3_context*,int,sqlite3_value**))
{
  if(SQLITE_OK!=sqlite3_create_function16(m_sqlite, name, num_args, SQLITE_UTF16LE, 0, function, 0, 0))
    throw_errorf("Unable to register custom function '%S': %S", name, sqlite3_errmsg16(m_sqlite));
}
//----------------------------------------------------------------------------

unsigned sqlite_connection::begin_schema_update()
{
  // create meta information, if necessary
  prepare(L"CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT)")->exec();
  prepare(L"INSERT OR IGNORE INTO meta (key, value) VALUES ('version', 0)")->exec();

  // query current version
  unsigned current_version=prepare(L"SELECT value FROM meta WHERE key='version'")->exec().get_unsigned(0);

  // start transaction
  prepare(L"BEGIN TRANSACTION")->exec();
  m_current_version=current_version;
  return current_version;
}
//----

void sqlite_connection::end_schema_update(const wchar_t *name, unsigned new_version)
{
  // set new version
  prepare(L"UPDATE meta SET value=? WHERE key='version'")->bind(0, new_version).exec();
  prepare(L"COMMIT TRANSACTION")->exec();
  if(!m_current_version)
    logger::infof("[%S] Created version %u database schema", name, new_version);
  else if(m_current_version!=new_version)
    logger::infof("[%S] Upgraded database schema from version %u to version %u", name, m_current_version, new_version);
  m_current_version=new_version;
}
//----------------------------------------------------------------------------


//============================================================================
// sqlite_statement
//============================================================================
sqlite_statement::sqlite_statement(sqlite3 *sqlite, sqlite3_stmt *stmt, const wstring &sql)
  :m_sqlite(sqlite)
  ,m_stmt(stmt)
  ,m_sql(sql)
  ,m_executed(false)
  ,m_more(false)
{
}
//----

sqlite_statement::~sqlite_statement()
{
  sqlite3_finalize(m_stmt);
}
//----------------------------------------------------------------------------

sqlite_statement &sqlite_statement::bind(unsigned idx, bool value)
{
  if(m_executed)
  {
    sqlite3_reset(m_stmt);
    m_executed=false;
  }
  if(SQLITE_OK!=sqlite3_bind_int(m_stmt, 1+idx, value))
    throw_errorf("Unable to bind column %u to %S: %S", idx, value?"true":"false", sqlite3_errmsg16(m_sqlite));
  return *this;
}
//----

sqlite_statement &sqlite_statement::bind(unsigned idx, unsigned value)
{
  if(m_executed)
  {
    sqlite3_reset(m_stmt);
    m_executed=false;
  }
  if(SQLITE_OK!=sqlite3_bind_int(m_stmt, 1+idx, value))
    throw_errorf("Unable to bind column %u to value %u: %S", idx, value, sqlite3_errmsg16(m_sqlite));
  return *this;
}
//----

sqlite_statement &sqlite_statement::bind(unsigned idx, boost::uint64_t value)
{
  if(m_executed)
  {
    sqlite3_reset(m_stmt);
    m_executed=false;
  }
  if(SQLITE_OK!=sqlite3_bind_int64(m_stmt, 1+idx, value))
    throw_errorf("Unable to bind column %u to value %lu: %S", idx, value, sqlite3_errmsg16(m_sqlite));
  return *this;
}
//----

sqlite_statement &sqlite_statement::bind(unsigned idx, float value)
{
  if(m_executed)
  {
    sqlite3_reset(m_stmt);
    m_executed=false;
  }
  if(SQLITE_OK!=sqlite3_bind_double(m_stmt, 1+idx, value))
    throw_errorf("Unable to bind column %u to value %f: %S", idx, value, sqlite3_errmsg16(m_sqlite));
  return *this;
}
//----

sqlite_statement &sqlite_statement::bind(unsigned idx, const wchar_t *value)
{
  if(m_executed)
  {
    sqlite3_reset(m_stmt);
    m_executed=false;
  }
  if(SQLITE_OK!=sqlite3_bind_text16(m_stmt, 1+idx, value, -1, SQLITE_TRANSIENT))
    throw_errorf("Unable to bind column %u to value '%S': %S", idx, value, sqlite3_errmsg16(m_sqlite));
  return *this;
}
//----

sqlite_statement &sqlite_statement::bind(unsigned idx, const wstring &value)
{
  if(m_executed)
  {
    sqlite3_reset(m_stmt);
    m_executed=false;
  }
  if(SQLITE_OK!=sqlite3_bind_text16(m_stmt, 1+idx, value.c_str(), -1, SQLITE_TRANSIENT))
    throw_errorf("Unable to bind column %u to value '%S': %S", idx, value.c_str(), sqlite3_errmsg16(m_sqlite));
  return *this;
}
//----

sqlite_statement &sqlite_statement::bind_null(unsigned idx)
{
  if(m_executed)
  {
    sqlite3_reset(m_stmt);
    m_executed=false;
  }
  if(SQLITE_OK!=sqlite3_bind_null(m_stmt, 1+idx))
    throw_errorf("Unable to bind column %u to NULL.", idx, sqlite3_errmsg16(m_sqlite));
  return *this;
}
//----------------------------------------------------------------------------

sqlite_statement &sqlite_statement::exec()
{
  // reset and step
  if(m_executed)
    sqlite3_reset(m_stmt);
  return next();
}
//----

sqlite_statement &sqlite_statement::next()
{
  // execute query, or fetch another row
  int result=sqlite3_step(m_stmt);
  if(SQLITE_DONE!=result && SQLITE_ROW!=result)
  {
    sqlite3_reset(m_stmt);
    throw_errorf("Unable to execute statement for query '%S': %S", m_sql.c_str(), sqlite3_errmsg16(m_sqlite));
  }
  m_executed=true;
  m_more=SQLITE_ROW==result;
  return *this;
}
//----

sqlite_statement::operator const void*() const
{
  return reinterpret_cast<const void*>(m_executed && m_more);
}
//----------------------------------------------------------------------------

bool sqlite_statement::get_bool(unsigned idx) const
{
  if(!m_executed)
    throw logic_error("SQL statement hasn't been executed yet.");
  return 0!=sqlite3_column_int(m_stmt, idx);
}
//----

unsigned sqlite_statement::get_unsigned(unsigned idx) const
{
  if(!m_executed)
    throw logic_error("SQL statement hasn't been executed yet.");
  return unsigned(sqlite3_column_int(m_stmt, idx));
}
//----

boost::uint64_t sqlite_statement::get_uint64(unsigned idx) const
{
  if(!m_executed)
    throw logic_error("SQL statement hasn't been executed yet.");
  return sqlite3_column_int64(m_stmt, idx);
}
//----

float sqlite_statement::get_float(unsigned idx) const
{
  if(!m_executed)
    throw logic_error("SQL statement hasn't been executed yet.");
  return float(sqlite3_column_double(m_stmt, idx));
}
//----

wstring sqlite_statement::get_string(unsigned idx) const
{
  if(!m_executed)
    throw logic_error("SQL statement hasn't been executed yet.");
  const unsigned size=sqlite3_column_bytes16(m_stmt, idx)/2;
  wstring buffer;
  buffer.resize(size);
  memcpy(&buffer[0], sqlite3_column_text16(m_stmt, idx), size*sizeof(wchar_t));
  return buffer;
}
//----

boost::optional<unsigned> sqlite_statement::get_unsigned_option(unsigned idx) const
{
  if(!m_executed)
    throw logic_error("SQL statement hasn't been executed yet.");
  boost::optional<unsigned> option;
  if(SQLITE_NULL!=sqlite3_column_type(m_stmt, idx))
    option=get_unsigned(idx);
  return option;
}
//----

boost::optional<boost::uint64_t> sqlite_statement::get_uint64_option(unsigned idx) const
{
  if(!m_executed)
    throw logic_error("SQL statement hasn't been executed yet.");
  boost::optional<boost::uint64_t> option;
  if(SQLITE_NULL!=sqlite3_column_type(m_stmt, idx))
    option=get_uint64(idx);
  return option;
}
//----

boost::optional<std::wstring> sqlite_statement::get_string_option(unsigned idx) const
{
  if(!m_executed)
    throw logic_error("SQL statement hasn't been executed yet.");
  boost::optional<std::wstring> option;
  if(SQLITE_NULL!=sqlite3_column_type(m_stmt, idx))
    option=get_string(idx);
  return option;
}
//----------------------------------------------------------------------------
