//============================================================================
// gfx.cpp: Win32 graphics components
//
// (c) Michael Walter, 2006
//============================================================================

#include "gfx.h"
#include "../core/dynlib.h"
#include "../log/log.h"
#include "../../core/defs.h"
#include <hash_map>
#include <vector>
#include <sstream>
#pragma comment(lib, "gdiplus.lib")
#define SHIL_LARGE          0   // normally 32x32
#define SHIL_SMALL          1   // normally 16x16
#define SHIL_EXTRALARGE     2
#define SHIL_SYSSMALL       3   // like SHIL_SMALL, but tracks system small icon metric correctly
extern "C" const IID IID_IImageList;
using namespace std;
using namespace stdext;
using namespace boost;
//----------------------------------------------------------------------------


//============================================================================
// anonymous namespace
//============================================================================
namespace
{
  // icon hash traits
  struct icon_hash_traits
  {
    static const size_t bucket_size = 4;
    static const size_t min_buckets = 8;
    size_t operator()(const icon_info &info) const
    {
      return size_t(info.source) + hash_compare<wstring>()(info.path);
    }
    bool operator()(const icon_info &a, const icon_info &b) const
    {
      return a.source<b.source || (a.source==b.source && a.path<b.path);
    }
  };
  //--------------------------------------------------------------------------

  // container types
  typedef hash_map<wstring, std::shared_ptr<image> > image_hash;
  typedef hash_map<icon_info, std::shared_ptr<icon>, icon_hash_traits> icon_hash;
  typedef vector<DWORD*> hicon_bytes;
  //--------------------------------------------------------------------------

  //==========================================================================
  // gfx_backend
  //==========================================================================
  class gfx_backend
  {
  public:
    // singleton
    static gfx_backend &get()
    {
      static gfx_backend *s_backend=new gfx_backend();
      return *s_backend;
    }
    //----

    ~gfx_backend()
    {
      // clear cache
      clear_cache();

      // shutdown gdi+
      Gdiplus::GdiplusShutdown(m_gdi_plus);
    }
    //------------------------------------------------------------------------

    // theme hack
    void set_current_theme(const std::wstring &name)
    {
      m_current_theme=name;
    }

    const std::wstring &current_theme() const
    {
      return m_current_theme;
    }
    //------------------------------------------------------------------------

    // cache management
    std::shared_ptr<image> lookup_image(const wstring &filename)
    {
      image_hash::iterator iter=m_images.find(filename);
      return iter==m_images.end() ? std::shared_ptr<image>() : iter->second;
    }
    //----

    std::shared_ptr<icon> lookup_icon(const icon_info &info)
    {
      icon_hash::iterator iter=m_icons.find(info);
      return iter==m_icons.end() ? std::shared_ptr<icon>() : iter->second;
    }
    //----

    void cache_image(const wstring &filename, std::shared_ptr<image> image)
    {
      m_images[filename]=image;
    }
    //----

    void cache_icon(const icon_info &info, std::shared_ptr<icon> icon)
    {
      m_icons[info]=icon;
    }
    //----

    void clear_cache()
    {
      // clear images and icons
      m_images.clear();
      m_icons.clear();

      // clear HICON bytes
      for(hicon_bytes::iterator iter=m_hicon_bytes.begin(); iter!=m_hicon_bytes.end(); ++iter)
        delete []*iter;
      m_hicon_bytes.clear();
    }
    //------------------------------------------------------------------------

    // icon loading
    void load_shell_icon(const wstring &filename, std::shared_ptr<image> &icon_32, std::shared_ptr<image> &icon_48)
    {
      if(m_image_list_32 && m_image_list_48)
      {
        // get shell file information
        SHFILEINFOW info;
        if(!SHGetFileInfoW(filename.c_str(), FILE_ATTRIBUTE_NORMAL, &info, sizeof(SHFILEINFOW), SHGFI_SYSICONINDEX|SHGFI_LARGEICON|SHGFI_USEFILEATTRIBUTES))
          throw_errorf("Unable to get file information for file: %S", filename.c_str());

        // get hicons
        HICON hicon_32=ImageList_GetIcon(m_image_list_32, info.iIcon, ILD_TRANSPARENT);
        HICON hicon_48=ImageList_GetIcon(m_image_list_48, info.iIcon, ILD_TRANSPARENT);

        // convert 32x32 icon
        if(!hicon_32)
          logger::warnf("Unable to get 32x32 icon for %S at index %u of system image list", filename.c_str(), info.iIcon);
        else
        {
          icon_32=convert_hicon(hicon_32);
          DestroyIcon(hicon_32);
        }

        // convert 48x48 icon
        if(!hicon_48)
          logger::warnf("Unable to get 48x48 icon for %S at index %u of system image list", filename.c_str(), info.iIcon);
        else
        {
          icon_48=convert_hicon(hicon_48);
          DestroyIcon(hicon_48);
        }
      }
      else
      {
        // load 32x32 icon only
        SHFILEINFOW info;
        HIMAGELIST image_list=(HIMAGELIST)SHGetFileInfoW(filename.c_str(), FILE_ATTRIBUTE_NORMAL, &info, sizeof(SHFILEINFOW), SHGFI_SYSICONINDEX|SHGFI_LARGEICON|SHGFI_USEFILEATTRIBUTES);
        if(!image_list)
          throw_errorf("Unable to get file information for file: %S", filename.c_str());

        // get hicon
        HICON hicon_32=ImageList_GetIcon(image_list, info.iIcon, ILD_TRANSPARENT);

        // convert 32x32 icon
        if(!hicon_32)
          logger::warnf("Unable to get 32x32 icon for %S at index %u of system image list", filename.c_str(), info.iIcon);
        {
          icon_32=convert_hicon(hicon_32);
          icon_48=convert_hicon(hicon_32);
          DestroyIcon(hicon_32);
        }
      }
    }
    //----

    void load_new_style_control_panel_applet_icon(const wstring &filename, LONG index, std::shared_ptr<image> &icon_32, std::shared_ptr<image> &icon_48)
    {
      // load binary
      dynamic_library lib(filename);

      // get entry point
      typedef LONG CALLBACK cpl_applet(HWND, UINT, LPARAM, LPARAM);
      cpl_applet *applet=lib.try_lookup<cpl_applet>("CPlApplet");
      if(!applet)
        return;

      // initialize applet
      applet(0, CPL_INIT, 0, 0);

      // load icon
      NEWCPLINFOW info;
      info.dwSize=info.dwFlags=0;
      applet(0, CPL_NEWINQUIRE, index, reinterpret_cast<LPARAM>(&info));
      if(!info.hIcon)
      {
        logger::warnf("Unable to get icon for %S#%u", filename.c_str(), index);
        return;
      }

      // convert icon (XXX: there must be a way to access all variants of the icon)
      std::shared_ptr<image> icon=convert_hicon(info.hIcon);
      if(48==icon->GetWidth())
      {
        icon_32.reset(icon->Clone(0, 0, 48, 48, PixelFormat32bppARGB));
        icon_32->SetResolution(32, 32);
        icon_48=icon;
      }
      else if(32==icon->GetWidth())
      {
        icon_32=icon;
        icon_48.reset(icon->Clone(0, 0, 32, 32, PixelFormat32bppARGB));
        icon_48->SetResolution(48, 48);
      }
      else
      {
        icon_32.reset(icon->Clone(0, 0, icon->GetWidth(), icon->GetHeight(), PixelFormat32bppARGB));
        icon_32->SetResolution(32, 32);
        icon_48.reset(icon->Clone(0, 0, icon->GetWidth(), icon->GetHeight(), PixelFormat32bppARGB));
        icon_32->SetResolution(48, 48);
      }

      // cleanup
      applet(0, CPL_EXIT, 0, 0);
    }
    //----

    void load_resource_icon(const wstring &filename, int id, std::shared_ptr<image> &icon_32, std::shared_ptr<image> &icon_48)
    {
      // load binary
      dynamic_library lib(filename);

      // load 32x32 icon
      HICON hicon_32=reinterpret_cast<HICON>(LoadImage(lib.get_hmodule(), MAKEINTRESOURCE(id), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
      if(!hicon_32)
      {
        logger::warnf("Unable to get 32x32 icon in %S with id %u, trying 16x16", filename.c_str(), id);
        hicon_32=reinterpret_cast<HICON>(LoadImage(lib.get_hmodule(), MAKEINTRESOURCE(id), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
        if(!hicon_32)
          logger::warnf("Unable to get 16x16 icon either", filename.c_str(), id);
      }

      // convert 32x32 icon
      if(hicon_32)
      {
        icon_32=convert_hicon(hicon_32);
        DestroyIcon(hicon_32);
      }

      // load 48x48 icon
      HICON hicon_48=reinterpret_cast<HICON>(LoadImage(lib.get_hmodule(), MAKEINTRESOURCE(id), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR));
      if(!hicon_48)
      {
        logger::warnf("Unable to get 48x48 icon in %S with id %u, trying 32x32", filename.c_str(), id);
        hicon_48=reinterpret_cast<HICON>(LoadImage(lib.get_hmodule(), MAKEINTRESOURCE(id), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
        if(!hicon_48)
        {
          logger::warnf("Unable to get 32x32 icon in %S with id %u, trying 16x16", filename.c_str(), id);
          hicon_48=reinterpret_cast<HICON>(LoadImage(lib.get_hmodule(), MAKEINTRESOURCE(id), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
          if(!hicon_48)
            logger::warnf("Unable to get 16x16 icon either", filename.c_str(), id);
        }
      }

      // convert 48x48 icon
      if(hicon_48)
      {
        icon_48=convert_hicon(hicon_48);
        DestroyIcon(hicon_48);
      }
    }
    //----

    std::shared_ptr<image> convert_hicon(HICON hicon)
    {
      // get icon information
      ICONINFO info;
      if(!GetIconInfo(hicon, &info))
        throw_errorf("Unable to get icon information.");
      DeleteObject(info.hbmMask);

      // create compatible DC and select icon bitmap
      HDC dc=CreateCompatibleDC(0);
      HGDIOBJ old_bitmap=SelectObject(dc, reinterpret_cast<HGDIOBJ>(info.hbmColor));

      // get bitmap information
      BITMAP bmp;
      GetObject(reinterpret_cast<HGDIOBJ>(info.hbmColor), sizeof(BITMAP), &bmp);

      // 32 bit icon?
      std::shared_ptr<image> image;
      if(bmp.bmBitsPixel==32)
      {
        // alloce buffer and fill icon bytes
        DWORD *bytes=new DWORD[bmp.bmWidth*bmp.bmHeight];
        m_hicon_bytes.push_back(bytes);
        GetBitmapBits(info.hbmColor, bmp.bmWidthBytes*bmp.bmHeight, bytes);

        // determine whether icon has an alpha channel
        bool has_alpha=false;
        for(unsigned i=0; i<unsigned(bmp.bmWidth*bmp.bmHeight); ++i)
        {
          const unsigned alpha=(bytes[i]&0xff000000)>>24;
          if(alpha!=0x00 && alpha!=0xff)
          {
            has_alpha=true;
            break;
          }
        }

        // if icon has an alpha channel, create bitmap from memory buffer (so that we get proper transparency)
        if(has_alpha)
          image.reset(new Gdiplus::Bitmap(bmp.bmWidth, bmp.bmHeight, bmp.bmWidthBytes, PixelFormat32bppARGB, reinterpret_cast<BYTE*>(bytes)));
      }

      // create image the default way
      if(!image)
        image.reset(new Gdiplus::Bitmap(hicon));

      // cleanup
      SelectObject(dc, old_bitmap);
      DeleteObject(info.hbmColor);
      DeleteDC(dc);
      return image;
    }
    //------------------------------------------------------------------------

  private:
    // construction and destruction
    gfx_backend()
      :m_image_list_32(0)
      ,m_image_list_48(0)
    {
      // initialize gdi+
      Gdiplus::GdiplusStartupInput input;
      if(Gdiplus::Ok!=Gdiplus::GdiplusStartup(&m_gdi_plus, &input, 0))
        throw runtime_error("Unable to initialize GDI+.");

      // load shell32.dll
      static dynamic_library s_shell32(L"shell32.dll");

      // get SHGetImageList function pointer
      typedef HRESULT STDMETHODCALLTYPE sh_get_image_list(int, REFIID, void**);
      sh_get_image_list *sgil=s_shell32.try_lookup<sh_get_image_list>("SHGetImageList");
      if(!sgil)
      {
        logger::warn("Unable to get SHGetImageList entry point by name, re-trying by ordinal.");
        sgil=s_shell32.try_lookup<sh_get_image_list>(727);
        if(!sgil)
          logger::warn("Unable to get SHGetImageList entry point. This could happen if you are running Windows 2000. Colibri will fall back to 32x32 icons.");
      }

      // get image lists
      if(sgil)
      {
        if(FAILED(sgil(SHIL_LARGE, IID_IImageList, reinterpret_cast<void**>(&m_image_list_32))))
          throw runtime_error("Unable to get 32x32 shell image list.");
        if(FAILED(sgil(SHIL_EXTRALARGE, IID_IImageList, reinterpret_cast<void**>(&m_image_list_48))))
          throw runtime_error("Unable to get 48x48 shell image list.");
      }
    }
    //------------------------------------------------------------------------

    ULONG_PTR m_gdi_plus;
    HIMAGELIST m_image_list_32, m_image_list_48;
    image_hash m_images;
    icon_hash m_icons;
    hicon_bytes m_hicon_bytes;
    std::wstring m_current_theme;
  };
}
//----------------------------------------------------------------------------


//============================================================================
// init_win32_gfx()
//============================================================================
void init_win32_gfx()
{
  // construct backend instance
  gfx_backend::get();
}
//----------------------------------------------------------------------------


//============================================================================
// shutdown_win32_gfx()
//============================================================================
void shutdown_win32_gfx()
{
  // delete backend instance
  delete &gfx_backend::get();
}
//----------------------------------------------------------------------------


//============================================================================
// icon_info
//============================================================================
icon_info::icon_info()
{
}
//----

icon_info::icon_info(e_icon_source source, const std::wstring &path)
  :source(source)
  ,path(path)
{
}
//----------------------------------------------------------------------------

void icon_info::set(e_icon_source source, const std::wstring &path)
{
  this->source=source;
  this->path=path;
}
//----------------------------------------------------------------------------


//============================================================================
// load_image()
//============================================================================
std::shared_ptr<image> load_image(const boost::filesystem::wpath &path)
{
  // lookup image in cache
  if(std::shared_ptr<image> ptr=gfx_backend::get().lookup_image(path.string()))
    return ptr;

  // load image
  std::shared_ptr<image> image(new image(path.string().c_str()));
  if(Gdiplus::Ok!=image->GetLastStatus())
    throw_errorf("Unable to load image: %S", path.string().c_str());
  
  // cache and return image
  gfx_backend::get().cache_image(path.string(), image);
  return image;
}
//----------------------------------------------------------------------------


//============================================================================
// set_theme_for_icons_hack()
//============================================================================
void set_theme_for_icons_hack(const wstring &name)
{
  gfx_backend::get().set_current_theme(name);
}
//----------------------------------------------------------------------------


//============================================================================
// load_icon()
//============================================================================
std::shared_ptr<icon> load_icon(const icon_info &info, const std::wstring &fallback_name)
{
  // lookup icon in cache
  if(std::shared_ptr<icon> ptr=gfx_backend::get().lookup_icon(info))
    return ptr;

  // branch on icon source type
  std::shared_ptr<icon> icon(new icon);
  icon->info=info;
  switch(info.source)
  {
  case icon_source_file:
    // load icon from image files
    icon->icon_32=load_image(info.path+L"_32.png");
    icon->icon_48=load_image(info.path+L"_48.png");
    break;

  case icon_source_shell:
    // load shell icon
    gfx_backend::get().load_shell_icon(info.path, icon->icon_32, icon->icon_48);
    break;

  case icon_source_new_style_cpl_applet:
    {
    // parse "path" (filename and index)
    wstring filename;
    LONG index;
    wistringstream is(info.path);
    getline(is, filename, L'#');
    is>>index;

    // load icon from new-style control panel applet
    gfx_backend::get().load_new_style_control_panel_applet_icon(filename, index, icon->icon_32, icon->icon_48);
    }
    break;

  case icon_source_resource:
    {
    // parse "path" (filename and id)
    wstring filename;
    int id;
    wistringstream is(info.path);
    getline(is, filename, L'#');
    is>>id;

    // load icon from resource
    gfx_backend::get().load_resource_icon(filename, id, icon->icon_32, icon->icon_48);
    }
    break;

  case icon_source_theme:
    // load icon from image files
    try
    {
      icon->icon_32=load_image(install_folder() / L"themes" / gfx_backend::get().current_theme() / (info.path+L"_32.png"));
    }
    catch(std::exception&)
    {
      logger::infof("Using icon \"%S\" (32x32) from default icon theme.", info.path.c_str());
      icon->icon_32=load_image(install_folder() / L"themes" / L"default" / (info.path+L"_32.png"));
    }
    try
    {
      icon->icon_48=load_image(install_folder() / L"themes" / gfx_backend::get().current_theme() / (info.path+L"_48.png"));
    }
    catch(std::exception&)
    {
      logger::infof("Using icon \"%S\" (48x48) from default icon theme.", info.path.c_str());
      icon->icon_48=load_image(install_folder() / L"themes" / L"default" / (info.path+L"_48.png"));
    }
    break;
  }

  // handle loading problems
  if(!icon->icon_32)
    icon->icon_32=load_image(install_folder() / L"themes" / L"default" / (fallback_name+L"_32.png"));
  if(!icon->icon_48)
    icon->icon_48=load_image(install_folder() / L"themes" / L"default" / (fallback_name+L"_48.png"));

  // cache and return image
  gfx_backend::get().cache_icon(info, icon);
  return icon;
}
//----------------------------------------------------------------------------


//============================================================================
// dc
//============================================================================
dc::dc(unsigned width, unsigned height)
{
  // initialize image
  m_width=width;
  m_height=height;

  // create DC
  m_dc=CreateCompatibleDC(0);
  if(!m_dc)
    throw runtime_error("Unable to create screen-compatible DC.");

  // create DIB
  BITMAPINFO bmi={0};
  bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth=width;
  bmi.bmiHeader.biHeight=-LONG(height);
  bmi.bmiHeader.biPlanes=1;
  bmi.bmiHeader.biBitCount=32;
  bmi.bmiHeader.biCompression=BI_RGB;
  bmi.bmiHeader.biSizeImage=width*height*4;
  void *pixels;
  m_bitmap=CreateDIBSection(m_dc, &bmi, DIB_RGB_COLORS, &pixels, 0, 0);
  if(!m_bitmap)
  {
    DeleteDC(m_dc);
    throw_errorf("Unable to create %ux%ux32 DIB section.", width, height);
  }

  // select bitmap into DC
  m_old_bitmap=reinterpret_cast<HBITMAP>(SelectObject(m_dc, m_bitmap));
}
//----

dc::~dc()
{
  // select old bitmap into DC
  if(m_old_bitmap)
    SelectObject(m_dc, m_old_bitmap);

  // cleanup
  DeleteObject(m_bitmap);
  DeleteDC(m_dc);
}
//----------------------------------------------------------------------------

HDC dc::get_dc() const
{
  return m_dc;
}
//----------------------------------------------------------------------------

void dc::update(HWND handle)
{
  SIZE size={m_width, m_height};
  POINT src={0};
  BLENDFUNCTION blend={AC_SRC_OVER, 0, 0xff, AC_SRC_ALPHA};
  if(!UpdateLayeredWindow(handle, 0, 0, &size, m_dc, &src, 0, &blend, ULW_ALPHA))
    throw runtime_error("Unable to update layered window.");
}
//----------------------------------------------------------------------------
