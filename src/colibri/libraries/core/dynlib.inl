//============================================================================
// dynlib.inl: Dynamic library component
//
// (c) Michael Walter, 2006
//============================================================================


//============================================================================
// dynamic_library
//============================================================================
template<typename T>
T *dynamic_library::lookup(const char *name)
{
  if(T *proc=try_lookup<T>(name))
    return proc;
  throw_errorf("Unable to lookup entry point '%s' in dynamic library '%S'", name, m_filename.c_str());
  return 0;
}
//----

template<typename T>
T *dynamic_library::lookup(unsigned ordinal)
{
  if(T *proc=try_lookup<T>(ordinal))
    return proc;
  throw_errorf("Unable to lookup entry point #%u in dynamic library '%S'", ordinal, m_filename.c_str());
  return 0;
}
//----

template<typename T> T *dynamic_library::try_lookup(const char *name)
{
  return reinterpret_cast<T*>(try_lookup_impl(name));
}
//----

template<typename T> T *dynamic_library::try_lookup(unsigned ordinal)
{
#pragma warning(push)
#pragma warning(disable:4312)
  return reinterpret_cast<T*>(try_lookup_impl(reinterpret_cast<const char*>(ordinal)));
#pragma warning(pop)
}
//----------------------------------------------------------------------------
