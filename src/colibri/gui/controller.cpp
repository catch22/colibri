//============================================================================
// controller.cpp: Brick controller
//
// (c) Michael Walter, 2006
//============================================================================

#include "controller.h"
#include "gui.h"
#include "../db/db.h"
#include "../db/match.h"
using namespace std;
using namespace boost;
//----------------------------------------------------------------------------


//============================================================================
// controller
//============================================================================
controller::~controller()
{
}
//----------------------------------------------------------------------------

bool controller::on_tab(gui&)
{
  return false;
}
//----

bool controller::on_enter(gui&)
{
  return false;
}
//----

void controller::on_input_changed(gui&)
{
}
//----------------------------------------------------------------------------


//============================================================================
// menu_controller
//============================================================================
menu_controller::menu_controller(database &db)
  :m_db(db)
{
}
//----------------------------------------------------------------------------

void menu_controller::add_item(const wstring &title, const wstring &description, const wstring &icon_name, const wstring &action)
{
  // add item info
  item_info info;
  info.is_checkbox=false;
  info.simple_title=title;
  info.simple_description=description;
  info.simple_icon_info.set(icon_source_theme, icon_name);
  info.action=action;
  m_item_info.push_back(info);
}
//----

void menu_controller::add_checkbox_item(const std::wstring &title_on, const std::wstring &title_off, bool value, const std::wstring &toggle_action)
{
  add_checkbox_item(title_on, title_off, value, toggle_action, L"yes", L"no");
}
//----

void menu_controller::add_checkbox_item(const std::wstring &title_on, const std::wstring &title_off, bool value, const std::wstring &toggle_action, const std::wstring &icon_name_on, const std::wstring &icon_name_off)
{
  // add item info
  item_info info;
  info.is_checkbox=true;
  info.checkbox_title_on=title_on;
  info.checkbox_title_off=title_off;
  info.checkbox_icon_info_on.set(icon_source_theme, icon_name_on);
  info.checkbox_icon_info_off.set(icon_source_theme, icon_name_off);
  info.checkbox_value=value;
  info.action=toggle_action;
  m_item_info.push_back(info);
}
//----------------------------------------------------------------------------

bool menu_controller::on_tab(gui &gui)
{
  return on_enter(gui);
}
//----

bool menu_controller::on_enter(gui &gui)
{
  const optional<boost::uint64_t> index=gui.get_current_option_data();
  if(!index || *index>=m_item_info.size())
    return false;
  item_info &info=m_item_info[unsigned(*index)];

  if(info.is_checkbox)
  {
    // toggle checkbox
    if(m_db.trigger_action(info.action))
    {
      info.checkbox_value=!info.checkbox_value;
      on_input_changed(gui);
      return true;
    }
    return false;
  }

  return m_db.trigger_action(info.action);
}
//----

void menu_controller::on_input_changed(gui &gui)
{
  // determine whether auto-selection is to be performed
  static wstring last_term=L"";
  const wstring term=normalized_term(gui.get_current_term());
  const bool perform_auto_selection=term!=last_term;
  last_term=term;

  // collect and set options, also determine index of maximally-scoring menu item
  vector<gui::option> options;
  float max_score=score_not_found();
  unsigned max_index=0;
  unsigned index=0;
  for(item_infos::const_iterator iter=m_item_info.begin(); iter!=m_item_info.end(); ++iter, ++index)
  {
    // add option
    if(iter->is_checkbox)
    {
      if(iter->checkbox_value)
        options.push_back(gui::option(iter->checkbox_title_on, L"Press Enter to toggle", iter->checkbox_icon_info_on, index));
      else
        options.push_back(gui::option(iter->checkbox_title_off, L"Press Enter to toggle", iter->checkbox_icon_info_off, index));
    }
    else
      options.push_back(gui::option(iter->simple_title, iter->simple_description, iter->simple_icon_info, index));

    // determine score
    float score;
    std::wstring marked_up_string;
    if(match(term.c_str(), options.back().title.c_str(), score, marked_up_string) && score>max_score)
    {
      max_score=score;
      max_index=index;
      options.back().title=marked_up_string;
    }
  }
  
  // set options
  gui.set_options(options.begin(), options.end());

  // perform auto-selection
  if(perform_auto_selection && max_score!=score_not_found())
    gui.set_current_option(max_index);
}
//----------------------------------------------------------------------------