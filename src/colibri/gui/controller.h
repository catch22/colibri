//============================================================================
// controller.h: Brick controller
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef COLIBRI_GUI_CONTROLLER_H
#define COLIBRI_GUI_CONTROLLER_H
#include "../core/defs.h"
#include "../libraries/win32/gfx.h"
class database;
class gui;
//----------------------------------------------------------------------------

// Interface:
class controller;
class menu_controller;
//----------------------------------------------------------------------------


//============================================================================
// controller
//============================================================================
class controller
{
public:
  // destruction
  virtual ~controller();
  //--------------------------------------------------------------------------

  // behavior
  virtual bool on_tab(gui&);
  virtual bool on_enter(gui&);
  virtual void on_input_changed(gui&);
};
//----------------------------------------------------------------------------


//============================================================================
// menu_controller
//============================================================================
class menu_controller: public controller
{
public:
  // construction
  menu_controller(database&);
  //--------------------------------------------------------------------------

  // item management
  void add_item(const std::wstring &title, const std::wstring &description, const std::wstring &icon_name, const std::wstring &action);
  void add_checkbox_item(const std::wstring &title_on, const std::wstring &title_off, bool value, const std::wstring &toggle_action);
  void add_checkbox_item(const std::wstring &title_on, const std::wstring &title_off, bool value, const std::wstring &toggle_action, const std::wstring &icon_name_on, const std::wstring &icon_name_off);
  //--------------------------------------------------------------------------

  // behavior
  virtual bool on_tab(gui&);
  virtual bool on_enter(gui&);
  virtual void on_input_changed(gui&);
  //--------------------------------------------------------------------------

private:
  struct item_info
  {
    bool is_checkbox;
    std::wstring simple_title;
    std::wstring simple_description;
    icon_info simple_icon_info;
    std::wstring checkbox_title_on;
    std::wstring checkbox_title_off;
    icon_info checkbox_icon_info_on;
    icon_info checkbox_icon_info_off;
    bool checkbox_value;
    std::wstring action;
  };
  typedef std::vector<item_info> item_infos;
  //----

  database &m_db;
  item_infos m_item_info;
};
//----------------------------------------------------------------------------

#endif