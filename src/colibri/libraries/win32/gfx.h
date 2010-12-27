//============================================================================
// gfx.h: Win32 graphics components
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef UTILS_WIN32_GFX_H
#define UTILS_WIN32_GFX_H
#include "win32.h"
#include <memory>
#include <boost/filesystem.hpp>
//----------------------------------------------------------------------------

// Interface:
typedef Gdiplus::Bitmap image;
enum e_icon_source;
struct icon_info;
struct icon;
class dc;
void set_theme_for_icons_hack(const std::wstring &name);
std::shared_ptr<image> load_image(const boost::filesystem::wpath&);
std::shared_ptr<icon> load_icon(const icon_info&, const std::wstring &fallback_name);
class image_list;
//----------------------------------------------------------------------------


//============================================================================
// e_icon_source
//============================================================================
enum e_icon_source
{
  icon_source_file=0,
  icon_source_shell,
  icon_source_resource,
  icon_source_new_style_cpl_applet,
  icon_source_theme,
  icon_source_count
};
//----------------------------------------------------------------------------


//============================================================================
// icon_info
//============================================================================
struct icon_info
{
  // construction
  icon_info();
  icon_info(e_icon_source, const std::wstring&);
  //--------------------------------------------------------------------------

  // mutators
  void set(e_icon_source, const std::wstring&);
  //--------------------------------------------------------------------------

  e_icon_source source;
  std::wstring path;
};
//----------------------------------------------------------------------------


//============================================================================
// icon
//============================================================================
struct icon
{
  icon_info info;
  std::shared_ptr<image> icon_32;
  std::shared_ptr<image> icon_48;
};
//----------------------------------------------------------------------------


//============================================================================
// dc
//============================================================================
class dc
{
public:
  // construction and destruction
  dc(unsigned width, unsigned height);
  ~dc();
  //--------------------------------------------------------------------------

  // accessors
  HDC get_dc() const;
  //--------------------------------------------------------------------------

  // updating
  void update(HWND);
  //--------------------------------------------------------------------------

private:
  unsigned m_width;
  unsigned m_height;
  HDC m_dc;
  HBITMAP m_bitmap, m_old_bitmap;
};
//----------------------------------------------------------------------------

#endif