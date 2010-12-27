//============================================================================
// gui.h: Bricks GUI
//
// (c) Michael Walter, 2005-2007
//============================================================================

#ifndef COLIBRI_GUI_GUI_H
#define COLIBRI_GUI_GUI_H
#include "splash_screen.h"
#include "../libraries/core/dynlib.h"
#include <memory>
#include <vector>
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
class plugin;
class colibri_plugin;
class database;
class preferences_dialog;
class controller;
//----------------------------------------------------------------------------

// Interface:
class gui;
class theme;
//----------------------------------------------------------------------------


//============================================================================
// gui
//============================================================================
class gui
{
public:
  // nested types
  struct option;
  //--------------------------------------------------------------------------

  // construction and destruction
  gui(colibri_plugin&);
  ~gui();
  void update_splash_screen(ufloat1 progress, const std::wstring &text);
  void notify_startup_complete();
  void restart_colibri();
  static void popup_other_colibri();
  //--------------------------------------------------------------------------

  // hotkey management
  void enable_hotkey(bool enable_);
  bool is_hotkey_enabled() const;
  //--------------------------------------------------------------------------

  // brick management
  void push_option_brick(controller*);
  void push_text_brick(controller*, boost::optional<icon_info> custom_icon=boost::none);
  void push_credits_brick();
  void push_hotkey_brick();
  void pop_brick();
  void hide();
  void refresh();
  //--------------------------------------------------------------------------

  // option brick accessors
  const wchar_t *get_current_term() const;
  boost::optional<boost::uint64_t> get_current_option_data() const;
  template<typename Iter> void set_options(Iter begin, Iter end);
  void set_current_option(unsigned index);
  void add_term_char(wchar_t);
  //--------------------------------------------------------------------------

  // text brick accessors
  const wchar_t *get_current_text() const;
  //--------------------------------------------------------------------------

private:
  gui(const gui&); // not implemented
  void operator=(const gui&); // not implemented
  static gui *s_instance;
  struct brick;
  enum e_brick_type {brick_type_option, brick_type_text, brick_type_credits, brick_type_hotkey, brick_type_count};
  typedef bool topaz_init(HWND);
  typedef void topaz_enable_hotkey(bool);
  typedef bool topaz_is_hotkey_enabled();
  typedef void topaz_set_hotkey(bool ctrl_, bool alt_, bool shift_, bool lwin_, bool rwin_, DWORD vk_);
  friend class theme;
  //--------------------------------------------------------------------------
  static LRESULT CALLBACK wnd_proc_brick(HWND, UINT, WPARAM, LPARAM);
  static LRESULT CALLBACK wnd_proc_dropdown(HWND, UINT, WPARAM, LPARAM);
  static LRESULT CALLBACK wnd_proc_dimmer(HWND, UINT, WPARAM, LPARAM);
  void push_brick(e_brick_type, controller&, boost::optional<icon_info> custom_icon=boost::none);
  const brick &get_current_brick() const;
  brick &get_current_brick();
  std::vector<option> &get_options();
  bool try_add_char(wchar_t);
  void on_input_changed();
  void on_options_changed();
  void update_tray_icon(bool add_=false);
  void update_hotkey();
  void relayout();
  void repaint(brick&);
  void repaint_dropdown();
  //--------------------------------------------------------------------------

  // theme
  std::shared_ptr<theme> m_theme;

  // stuff
  colibri_plugin &m_colibri;
  std::shared_ptr<splash_screen> m_splash_screen;
  HICON m_icon_colibri, m_icon_colibri_l, m_icon_colibri_d, m_icon_colibri_dl;
  HMENU m_context_menu;
  dynamic_library m_topaz;
  topaz_init *m_topaz_init;
  topaz_enable_hotkey *m_topaz_enable_hotkey;
  topaz_is_hotkey_enabled *m_topaz_is_hotkey_enabled;
  topaz_set_hotkey *m_topaz_set_hotkey;
  bool m_in_startup;

  // bricks
  boost::ptr_deque<brick> m_bricks;
  RECT m_rc_monitor;

  // dropdown
  HWND m_dropdown;

  // dimmer
  HWND m_dimmer;
};
//----------------------------------------------------------------------------

//============================================================================
// gui::option
//============================================================================
struct gui::option
{
  // construction
  option(const std::wstring &title, const std::wstring &description, const icon_info&, boost::uint64_t data);
  //--------------------------------------------------------------------------

  std::wstring title;
  std::wstring description;
  icon_info icon_info;
  boost::uint64_t data;
  bool has_arrow_overlay;
};
//----------------------------------------------------------------------------


//============================================================================
// theme
//============================================================================
class theme
{
public:
  // construction
  theme(const std::wstring &name);
  //--------------------------------------------------------------------------

  // loading
  bool load(const std::wstring &name, std::wstring &errors);
  static void load_about(const std::wstring &name, std::wstring &title, std::wstring &description, std::wstring &author);
  //--------------------------------------------------------------------------

  // metrics
  unsigned get_brick_width() const;
  unsigned get_brick_height() const;
  unsigned get_dropdown_width() const;
  unsigned get_dropdown_height() const;
  unsigned get_dropdown_rows_per_page() const;
  unsigned get_splash_screen_width() const;
  unsigned get_splash_screen_height() const;
  //--------------------------------------------------------------------------

  // painting
  void paint_brick(Gdiplus::Graphics&, gui::brick&, colibri_plugin&);
  void paint_dropdown(Gdiplus::Graphics&, gui::brick&);
  void paint_splash_screen(Gdiplus::Graphics&, ufloat1 progress, const wchar_t *text);
  //--------------------------------------------------------------------------

  // loading
  std::shared_ptr<icon> load_icon(const icon_info&);
  //--------------------------------------------------------------------------

private:
  static void load_theme_image(const std::wstring &ini, const wchar_t *filename_, Gdiplus::Image*&, std::wstring &errors);
  void load_theme_font(const std::wstring &ini, const wchar_t *section, const wchar_t *key, std::auto_ptr<Gdiplus::Font>&, std::wstring &errors);
  static void load_theme_brush(const std::wstring &ini, const wchar_t *section, const wchar_t *key, std::auto_ptr<Gdiplus::SolidBrush>&, std::wstring &errors);
  static void load_theme_rect(const std::wstring &ini, const wchar_t *section, const wchar_t *key, Gdiplus::RectF&, std::wstring &errors);
  static void load_theme_pos(const std::wstring &ini, const wchar_t *section, const wchar_t *key, Gdiplus::Point&, std::wstring &errors);
  static void load_theme_int(const std::wstring &ini, const wchar_t *section, const wchar_t *key, unsigned&, std::wstring &errors);
  static bool get_ini(const std::wstring &theme, std::wstring&);
  static bool get(const std::wstring &ini, const wchar_t *section, const wchar_t *key, std::wstring &str, std::wstring &errors);
  //--------------------------------------------------------------------------

  // default font
  std::wstring m_default_font;

  // brick
  Gdiplus::Image *m_brick;

  // brick title
  std::auto_ptr<Gdiplus::Font> m_brick_title_font;
  std::auto_ptr<Gdiplus::SolidBrush> m_brick_title_brush;
  Gdiplus::RectF m_brick_title_rect;

  // brick hint
  std::auto_ptr<Gdiplus::Font> m_brick_hint_font;
  std::auto_ptr<Gdiplus::SolidBrush> m_brick_hint_brush;
  Gdiplus::RectF m_brick_hint_rect;

  // brick icon
  Gdiplus::Point m_brick_icon_pos;
  Gdiplus::Image *m_default_text_brick_icon;
  Gdiplus::Image *m_hotkey_brick_icon;

  // overlay icons
  Gdiplus::Image *m_arrow_overlay_32;
  Gdiplus::Image *m_arrow_overlay_48;

  // credits
  Gdiplus::Image *m_credits_brick_icon;
  std::auto_ptr<Gdiplus::Font> m_credits_font_1, m_credits_font_2, m_credits_font_3, m_credits_font_4;
  std::auto_ptr<Gdiplus::SolidBrush> m_credits_brush;
  Gdiplus::RectF m_credits_rect;

  // dropdown
  unsigned m_dropdown_height;

  // dropdown header
  Gdiplus::Image *m_dropdown_header;
  std::auto_ptr<Gdiplus::Font> m_dropdown_header_text_font;
  std::auto_ptr<Gdiplus::SolidBrush> m_dropdown_header_text_brush;
  Gdiplus::RectF m_dropdown_header_text_rect;

  // dropdown row
  Gdiplus::Image *m_dropdown_row;
  Gdiplus::Image *m_dropdown_row_active;
  Gdiplus::Point m_dropdown_row_icon_pos;
  std::auto_ptr<Gdiplus::Font> m_dropdown_row_title_font;
  std::auto_ptr<Gdiplus::Font> m_dropdown_row_description_font;
  std::auto_ptr<Gdiplus::SolidBrush> m_dropdown_row_title_brush;
  std::auto_ptr<Gdiplus::SolidBrush> m_dropdown_row_description_brush;
  Gdiplus::RectF m_dropdown_row_title_rect;
  Gdiplus::RectF m_dropdown_row_description_rect;

  // dropdown footer
  Gdiplus::Image *m_dropdown_footer;
  std::auto_ptr<Gdiplus::Font> m_dropdown_footer_text_font;
  std::auto_ptr<Gdiplus::SolidBrush> m_dropdown_footer_text_brush;
  Gdiplus::RectF m_dropdown_footer_text_rect;

  // splash screen
  Gdiplus::Image *m_splash_screen;

  // dependant values
  unsigned m_brick_width;
  unsigned m_brick_height;
  unsigned m_dropdown_width;
  unsigned m_dropdown_row_height;
  unsigned m_dropdown_page_height;
  unsigned m_dropdown_rows_per_page;
  unsigned m_splash_screen_width;
  unsigned m_splash_screen_height;
};
//----------------------------------------------------------------------------

#include "gui.inl"
#endif