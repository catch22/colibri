//============================================================================
// colibri_plugin.cpp: Colibri plugin
//
// (c) Michael Walter, 2006
//============================================================================

#include "colibri_plugin.h"
#include "../core/version.h"
#include "../gui/gui.h"
#include "../gui/controller.h"
#include "../libraries/win32/shell.h"
#include "../libraries/win32/win.h"
#include "../libraries/net/net.h"
#include "../libraries/log/log.h"
#include "../thirdparty/rapidxml/rapidxml.hpp"
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <sstream>
//----------------------------------------------------------------------------

namespace
{
  version get_version_of_entry(rapidxml::xml_node<> &entry)
  {
    // get version node
    rapidxml::xml_node<> *version_node=entry.first_node("colibri:version");
    if(!version_node)
      throw std::runtime_error("No <colibri:version> node.");

    // get version number and type
    rapidxml::xml_node<> *version_number_node=version_node->first_node("colibri:number");
    rapidxml::xml_node<> *version_type_node=version_node->first_node("colibri:type");
    if(!version_number_node)
      throw std::runtime_error("No <colibri:number> node.");
    if(!version_type_node)
      throw std::runtime_error("No <colibri:type> node.");

    // parse version
    version v;
    v.number=atoi(version_number_node->value());
    if(0==strcmp(version_type_node->value(), "alpha"))
      v.type=version_type_alpha;
    else if(0==strcmp(version_type_node->value(), "beta"))
      v.type=version_type_beta;
    else
      v.type=version_type_release;
    return v;
  }
}


//============================================================================
// colibri_plugin
//============================================================================
const wchar_t *colibri_plugin::get_name() const
{
  return L"colibri";
}
//----

const wchar_t *colibri_plugin::get_title() const
{
  return L"Colibri";
}
//----------------------------------------------------------------------------

unsigned colibri_plugin::update_config(unsigned current_version)
{
  sqlite_connection &config=get_config();
  switch(current_version)
  {
  case 0:
    // fresh install
    config.prepare(L"CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, value TEXT)")->exec();
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('splash_screen.enabled', 1)")->exec();
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('hotkey.enabled', 1)")->exec();
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('hotkey.ctrl', 1)")->exec();
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('hotkey.alt', 0)")->exec();
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('hotkey.shift', 0)")->exec();
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('hotkey.lwin', 0)")->exec();
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('hotkey.rwin', 0)")->exec();
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('hotkey.vk', ?)")->bind(0, unsigned(VK_SPACE)).exec();

  case 1:
    // add "monitor" setting
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('monitor', 0)")->exec();

  case 2:
    // add "theme" setting
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('theme', 'Default')")->exec();

  case 3:
    // add "store_custom_items" setting
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('store_custom_items', 0)")->exec();

  case 4:
    // reset "monitor" setting
    get_config().prepare(L"UPDATE settings SET value = ? WHERE key = 'monitor'")->bind(0, get_monitors().front().name).exec();

  case 5:
    // add "show_update_notification" setting
    config.prepare(L"INSERT OR IGNORE INTO settings (key, value) VALUES ('show_update_notification', 1)")->exec();

  case 6:
    // rename "show_update_notification" setting to "check_for_updates"
    config.prepare(L"UPDATE settings SET key = 'check_for_updates' WHERE key = 'show_update_notification'")->exec();
  }
  return 7;
}
//----------------------------------------------------------------------------

void colibri_plugin::index(boost::uint64_t new_index_version)
{
  // add "Colibri" item
  database_item item;
  item.plugin_id=get_name();
  item.item_id=L"Colibri";
  item.title=L"Colibri";
  item.description=L"Manage Colibri";
  item.is_transient=false;
  item.icon_info.set(icon_source_theme, L"colibri/logo");
  item.index_version=new_index_version;
  item.on_enter=L"colibri_actions.open_colibri_menu";
  item.on_tab=L"colibri_actions.open_colibri_menu";
  get_db().add_or_update_item(item);
}
//----------------------------------------------------------------------------

bool colibri_plugin::on_action(const std::wstring &name, boost::optional<database_item>)
{
  if(name==L"colibri_actions.open_colibri_menu")
  {
    // open colibri menu
    menu_controller *ctrl=new menu_controller(get_db());
    ctrl->add_item(L"Preferences", L"Display and modify preferences", L"colibri/preferences", L"colibri_actions.open_preferences_menu");
    ctrl->add_item(L"Credits", L"Show credits", L"colibri/credits", L"colibri_actions.open_credits_brick");
    ctrl->add_item(L"Help", L"Launch documentation", L"colibri/help", L"colibri_actions.open_help");
    ctrl->add_item(L"Update index", L"Update index", L"colibri/update_index", L"colibri_actions.update_index");
    ctrl->add_item(L"Restart", L"Restart Colibri", L"colibri/restart", L"colibri_actions.restart");
    ctrl->add_item(L"Quit", L"Quit Colibri", L"colibri/quit", L"colibri_actions.quit");
    get_gui().push_option_brick(ctrl);
    return true;
  }
  else if(name==L"colibri_actions.open_preferences_menu")
  {
    /*XXX
    get_gui().popup_preferences_dialog();
    get_gui().hide();
    return true;
    */
    // open colibri menu
    menu_controller *ctrl=new menu_controller(get_db());
    ctrl->add_checkbox_item(L"Start Colibri on logon", L"Don't start Colibri on logon", is_auto_startup_enabled(), L"colibri_actions.toggle_auto_startup");
    ctrl->add_checkbox_item(L"Show Splash Screen", L"Hide Splash Screen", is_splash_screen_enabled(), L"colibri_actions.toggle_splash_screen");
    ctrl->add_checkbox_item(L"Check for updates", L"Don't check for updates", check_for_updates(), L"colibri_actions.toggle_check_for_updates");
    ctrl->add_checkbox_item(L"Store user-defined items in database", L"Don't store user-defined items in database", are_custom_items_stored(), L"colibri_actions.toggle_store_custom_items");
    ctrl->add_checkbox_item(L"Hotkey enabled", L"Hotkey disabled", is_hotkey_enabled(), L"colibri_actions.toggle_hotkey");
    ctrl->add_item(L"Hotkey", L"Change hotkey", L"colibri/hotkey", L"colibri_actions.open_hotkey_brick");
    ctrl->add_item(L"Monitor", L"Change monitor", L"colibri/monitor", L"colibri_actions.open_monitor_menu");
    ctrl->add_item(L"Theme", L"Change theme", L"colibri/theme", L"colibri_actions.open_theme_menu");
    get_gui().push_option_brick(ctrl);
    return true;
  }
  else if(name==L"colibri_actions.open_monitor_menu")
  {
    // open colibri menu
    menu_controller *ctrl=new menu_controller(get_db());
    std::vector<monitor> monitors=get_monitors();
    const std::wstring &current_monitor=get_monitor();
    for(std::vector<monitor>::size_type i=0; i<monitors.size(); ++i)
    {
      // format title
      std::wostringstream title;
      title<<monitors[i].title;
      if(monitors[i].name==current_monitor)
        title<<L" (Current)";
      else if(monitors[i].is_primary)
        title<<L" (Primary)";

      // format
      std::wostringstream action;
      action<<L"colibri_actions.set_monitor_";
      action<<monitors[i].name;
      ctrl->add_item(title.str(), L"Move Colibri to monitor", L"colibri/monitor", action.str());
    }
    get_gui().push_option_brick(ctrl);
    return true;
  }
  else if(name==L"colibri_actions.open_theme_menu")
  {
    // open colibri menu
    menu_controller *ctrl=new menu_controller(get_db());
    const std::wstring current_theme=get_theme();
    typedef boost::filesystem::wdirectory_iterator iter_type;
    for(iter_type iter=iter_type(install_folder() / L"themes"); iter!=iter_type(); ++iter)
    {
      if(!is_directory(iter->status()))
        continue;
      if(!exists(iter->path() / L"theme.ini"))
        continue;
      const std::wstring theme=iter->path().leaf();
      std::wstring title, description, author;
      theme::load_about(theme, title, description, author);
      ctrl->add_item(theme==current_theme ? title+L" (Current)" : title, description, L"icon", L"colibri_actions.set_theme_"+theme);
    }
    get_gui().push_option_brick(ctrl);
    return true;
  }
  else if(name==L"colibri_actions.open_credits_brick")
  {
    // open credits brick
    get_gui().push_credits_brick();
    return true;
  }
  else if(name==L"colibri_actions.open_help")
  {
    // launch online help
    bool ok=launch(L"http://colibri.leetspeak.org/help");
    get_gui().hide();
    return ok;
  }
  else if(name==L"colibri_actions.restart")
  {
    // restart colibri
    get_gui().restart_colibri();
    return true;
  }
  else if(name==L"colibri_actions.update_index")
  {
    // XXX: deprecate once preferences->database dialog is in place
    get_db().update_index();
    return true;
  }
  else if(name==L"colibri_actions.quit")
  {
    // quit colibri
    PostQuitMessage(0);
    return true;
  }
  else if(name==L"colibri_actions.toggle_auto_startup")
  {
    enable_auto_startup(!is_auto_startup_enabled());
    return true;
  }
  else if(name==L"colibri_actions.toggle_splash_screen")
  {
    enable_splash_screen(!is_splash_screen_enabled());
    return true;
  }
  else if(name==L"colibri_actions.toggle_check_for_updates")
  {
    set_check_for_updates(!check_for_updates());
    return true;
  }
  else if(name==L"colibri_actions.toggle_store_custom_items")
  {
    store_custom_items(!are_custom_items_stored());
    return true;
  }
  else if(name==L"colibri_actions.toggle_hotkey")
  {
    enable_hotkey(!is_hotkey_enabled());
    return true;
  }
  else if(name==L"colibri_actions.open_hotkey_brick")
  {
    // push hotkey brick
    get_gui().push_hotkey_brick();
    return true;
  }
  else if(0==name.find(L"colibri_actions.set_monitor_"))
  {
    // set monitor
    set_monitor(name.substr(28));
    return true;
  }
  else if(0==name.find(L"colibri_actions.set_theme_"))
  {
    // set theme
    std::wstring theme=name.substr(26);
    set_theme(theme);
    return true;
  }
  else if(name==L"global.startup")
  {
    if(check_for_updates())
      boost::thread(boost::bind(&colibri_plugin::perform_update_check, this));
  }
  else if(name==L"global.startup_complete")
  {
    m_feedback_allowed.notify_one();
    return true;
  }
  return false;
}
//----------------------------------------------------------------------------

bool colibri_plugin::is_auto_startup_enabled() const
{
  // open key and query "Colibri" value
  HKEY key;
  if(ERROR_SUCCESS!=RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &key))
    throw std::runtime_error("Unable to open registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Run");
  bool is_enabled=ERROR_SUCCESS==RegQueryValueExW(key, L"Colibri", 0, 0, 0, 0);
  RegCloseKey(key);
  return is_enabled;
}
//----

void colibri_plugin::enable_auto_startup(bool enable)
{
  // determine executable path
  wchar_t colibri_path[MAX_PATH];
  GetModuleFileNameW(0, colibri_path, MAX_PATH);

  // open key
  HKEY key;
  if(ERROR_SUCCESS!=RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &key))
    throw std::runtime_error("Unable to open registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Run");

  // save/delete "Colibri" value
  if(enable)
    RegSetValueExW(key, L"Colibri", 0, REG_SZ, reinterpret_cast<const BYTE*>(colibri_path), static_cast<DWORD>(2*wcslen(colibri_path)+1));
  else
    RegDeleteValueW(key, L"Colibri");
  RegCloseKey(key);
}
//----

bool colibri_plugin::is_splash_screen_enabled() const
{
  return get_config().prepare(L"SELECT value FROM settings WHERE key = 'splash_screen.enabled'")->exec().get_bool();
}
//----

void colibri_plugin::enable_splash_screen(bool enable)
{
  get_config().prepare(L"UPDATE settings SET value = ? WHERE key = 'splash_screen.enabled'")->bind(0, enable).exec();
}
//----

bool colibri_plugin::check_for_updates() const
{
  return get_config().prepare(L"SELECT value FROM settings WHERE key = 'check_for_updates'")->exec().get_bool();
}
//----

void colibri_plugin::set_check_for_updates(bool enable)
{
  get_config().prepare(L"UPDATE settings SET value = ? WHERE key = 'check_for_updates'")->bind(0, enable).exec();
}
//----

bool colibri_plugin::is_hotkey_enabled() const
{
  return get_config().prepare(L"SELECT value FROM settings WHERE key = 'hotkey.enabled'")->exec().get_bool();
}
//----

void colibri_plugin::enable_hotkey(bool enable)
{
  get_config().prepare(L"UPDATE settings SET value = ? WHERE key = 'hotkey.enabled'")->bind(0, enable).exec();
  get_gui().enable_hotkey(is_hotkey_enabled());
}
//----

bool colibri_plugin::are_custom_items_stored() const
{
  return get_config().prepare(L"SELECT value FROM settings WHERE key = 'store_custom_items'")->exec().get_bool();
}
//----

void colibri_plugin::store_custom_items(bool store)
{
  get_config().prepare(L"UPDATE settings SET value = ? WHERE key = 'store_custom_items'")->bind(0, store).exec();
}
//----

std::wstring colibri_plugin::get_theme() const
{
  return get_config().prepare(L"SELECT value FROM settings WHERE key = 'theme'")->exec().get_string();
}
//----

void colibri_plugin::set_theme(const std::wstring &name)
{
  get_config().prepare(L"UPDATE settings SET value = ? WHERE key = 'theme'")->bind(0, name).exec();
  get_gui().restart_colibri();
}
//----

std::wstring colibri_plugin::get_monitor() const
{
  return get_config().prepare(L"SELECT value FROM settings WHERE key = 'monitor'")->exec().get_string();
}
//----

void colibri_plugin::set_monitor(const std::wstring &name)
{
  get_config().prepare(L"UPDATE settings SET value = ? WHERE key = 'monitor'")->bind(0, name).exec();
  get_gui().restart_colibri();
}
//----

void colibri_plugin::set_hotkey(const hotkey &hk)
{
  sqlite_connection &db=get_config();
  db.prepare(L"INSERT OR REPLACE INTO settings (key, value) VALUES ('hotkey.ctrl', ?)")->bind(0, hk.ctrl).exec();
  db.prepare(L"INSERT OR REPLACE INTO settings (key, value) VALUES ('hotkey.alt', ?)")->bind(0, hk.alt).exec();
  db.prepare(L"INSERT OR REPLACE INTO settings (key, value) VALUES ('hotkey.shift', ?)")->bind(0, hk.shift).exec();
  db.prepare(L"INSERT OR REPLACE INTO settings (key, value) VALUES ('hotkey.lwin', ?)")->bind(0, hk.lwin).exec();
  db.prepare(L"INSERT OR REPLACE INTO settings (key, value) VALUES ('hotkey.rwin', ?)")->bind(0, hk.rwin).exec();
  db.prepare(L"INSERT OR REPLACE INTO settings (key, value) VALUES ('hotkey.vk', ?)")->bind(0, unsigned(hk.vk)).exec();
}
//----

hotkey colibri_plugin::get_hotkey() const
{
  sqlite_connection &db=get_config();
  hotkey hk;
  hk.ctrl=db.prepare(L"SELECT value FROM settings WHERE key = 'hotkey.ctrl'")->exec().get_bool();
  hk.alt=db.prepare(L"SELECT value FROM settings WHERE key = 'hotkey.alt'")->exec().get_bool();
  hk.shift=db.prepare(L"SELECT value FROM settings WHERE key = 'hotkey.shift'")->exec().get_bool();
  hk.lwin=db.prepare(L"SELECT value FROM settings WHERE key = 'hotkey.lwin'")->exec().get_bool();
  hk.rwin=db.prepare(L"SELECT value FROM settings WHERE key = 'hotkey.rwin'")->exec().get_bool();
  hk.vk=db.prepare(L"SELECT value FROM settings WHERE key = 'hotkey.vk'")->exec().get_unsigned();
  return hk;
}
//----------------------------------------------------------------------------

void colibri_plugin::perform_update_check()
{
  // download releases.atom feed
  buffer b;
  if(!download(L"http://colibri.leetspeak.org/releases.atom", b, str(colibri_version())))
  {
    logger::warn("Unable to download releases.atom feed.");
    return;
  }
  b.push_back(0);

  // parse
  rapidxml::xml_document<> doc;
  try
  {
    char *str=doc.allocate_string((const char*)&b[0], b.size());
    doc.parse<rapidxml::parse_trim_whitespace|rapidxml::parse_normalize_whitespace>(str);
  }
  catch(std::exception &e)
  {
    logger::warnf("Unable to parse releases.atom feed: %s.", e.what());
    return;
  }

  // determine best available update
  version best_available_version=colibri_version();
  try
  {
    rapidxml::xml_node<> *feed=doc.first_node("feed");
    if(!feed)
      throw std::runtime_error("No <feed> node.");
    for(rapidxml::xml_node<> *entry=feed->first_node("entry"); entry; entry=entry->next_sibling("entry"))
    {
      version v=get_version_of_entry(*entry);
      if(v>best_available_version)
        best_available_version=v;
    }
  }
  catch(std::exception &e)
  {
    logger::warnf("Malformed releases.atom feed: %s.", e.what());
    return;
  }

  // better version available?
  if(best_available_version>colibri_version())
  {
    logger::warnf("Local release (%S) is older than current public release (%S).", str(colibri_version()).c_str(), str(best_available_version).c_str());

    // wait until feedback is allowed
    boost::mutex mutex;
    boost::mutex::scoped_lock lock(mutex);
    m_feedback_allowed.wait(lock);

    // XXX: integrate updater.
    if(IDOK==MessageBoxW(0, L"A newer version of Colibri is available for download!\n\nPress OK to redirect your web browser to the Colibri update page.", L"Update available", MB_OKCANCEL|MB_ICONINFORMATION|MB_SETFOREGROUND))
      launch(L"http://colibri.leetspeak.org/update");
  }
}
//----------------------------------------------------------------------------


//============================================================================
// str()
//============================================================================
std::wstring str(const hotkey &hk)
{
  enum {BUFFER_SIZE=256};
  wchar_t buffer[BUFFER_SIZE]={0};
  std::wstring result;

  // add ctrl key
  if(hk.ctrl)
  {
    GetKeyNameTextW(MapVirtualKeyW(VK_CONTROL, 0)<<16, buffer, BUFFER_SIZE);
    result+=buffer;
  }

  // add alt key
  if(hk.alt)
  {
    GetKeyNameTextW(MapVirtualKeyW(VK_MENU, 0)<<16, buffer, BUFFER_SIZE);
    if(result.size())
      result+=L"+";
    result+=buffer;
  }

  // add shift key
  if(hk.shift)
  {
    GetKeyNameTextW(MapVirtualKeyW(VK_SHIFT, 0)<<16, buffer, BUFFER_SIZE);
    if(result.size())
      result+=L"+";
    result+=buffer;
  }

  // add left/right windows key
  if(hk.lwin)
  {
    if(result.size())
      result+=L"+";
    result+=L"LWIN";
  }
  if(hk.rwin)
  {
    if(result.size())
      result+=L"+";
    result+=L"RWIN";
  }

  // add actual key
  GetKeyNameTextW(MapVirtualKeyW(hk.vk, 0)<<16, buffer, BUFFER_SIZE);
  if(result.size())
    result+=L"+";
  result+=buffer;
  return result;
}
//----------------------------------------------------------------------------
