//============================================================================
// search_engines_plugin.cpp: Search Engines plugin
//
// (c) Michael Walter, 2005-2007
//============================================================================

#include "search_engines_plugin.h"
#include "../gui/gui.h"
#include "../libraries/win32/shell.h"
using namespace std;
using namespace boost;
//----------------------------------------------------------------------------


//============================================================================
// default search engines
//============================================================================
static struct search_engine
{
  const wchar_t *uid;
  const wchar_t *title;
  const wchar_t *description;
  const wchar_t *icon_basename;
  const wchar_t *index_url;
  const wchar_t *detail_url;
} default_search_engines[]=
{
  {L"Google (en)",    L"Google",        L"Search the web",               L"Google",    L"http://www.google.com",        L"http://www.google.com/search?q=%s"},
  {L"LEO (en<->de)",  L"LEO (en<->de)", L"Translate text",               L"LEO",       L"http://dict.leo.org/?lp=ende", L"http://dict.leo.org/?lp=ende&search=%s"},
  {L"LEO (fr<->de)",  L"LEO (fr<->de)", L"Translate text",               L"LEO",       L"http://dict.leo.org/?lp=frde", L"http://dict.leo.org/?lp=frde&search=%s"},
  {L"LEO (es<->de)",  L"LEO (es<->de)", L"Translate text",               L"LEO",       L"http://dict.leo.org/?lp=esde", L"http://dict.leo.org/?lp=esde&search=%s"},
  {L"Wikipedia (en)", L"Wikipedia",     L"Search Wikipedia",             L"Wikipedia", L"http://wikipedia.org/",        L"http://wikipedia.org/search-redirect.php?language=en&search=%s"},
  {L"arXiv",          L"arXiv",         L"Search arXiv e-Print archive", L"Arxiv",     L"http://arxiv.org/",            L"http://arxiv.org/find/grp_q-bio,grp_cs,grp_physics,grp_math,grp_nlin/1/all:+AND+%s/0/1/0/all/0/1"},
  {L"Answers.com",    L"Answers.com",   L"Ask Answers.com",              L"Generic",   L"http://www.answers.com/",      L"http://www.answers.com/%s"},
  {0}
};
//----------------------------------------------------------------------------

  
//============================================================================
// search_engines_plugin
//============================================================================
const wchar_t *search_engines_plugin::get_name() const
{
  return L"search_engines";
}
//----

const wchar_t *search_engines_plugin::get_title() const
{
  return L"Search Engines";
}
//----------------------------------------------------------------------------

unsigned search_engines_plugin::update_config(unsigned current_version)
{
  switch(current_version)
  {
  case 0:
    // fresh install
    {
    get_config().prepare(L"CREATE TABLE search_engines (uid TEXT PRIMARY KEY NOT NULL, title TEXT NOT NULL, description TEXT NOT NULL, icon_basename TEXT NOT NULL, homepage_url TEXT NOT NULL, search_url_template TEXT NOT NULL)")->exec();
    std::shared_ptr<sqlite_statement> stmt=get_config().prepare(L"INSERT INTO search_engines (uid, title, description, icon_basename, homepage_url, search_url_template) VALUES (?, ?, ?, ?, ?, ?)");
    for(search_engine *e=default_search_engines; e->title; ++e)
    {
      // add search engine
      stmt->bind(0, e->uid);
      stmt->bind(1, e->title);
      stmt->bind(2, e->description);
      stmt->bind(3, e->icon_basename);
      stmt->bind(4, e->index_url);
      stmt->bind(5, e->detail_url);
      stmt->exec();
    }
    }
  
  case 1:
    // remove all unindexed search engines
    get_db().delete_unindexed_items(get_name());
  }
  return 2;
}
//----------------------------------------------------------------------------

void search_engines_plugin::index(boost::uint64_t new_index_version)
{
  // add search engines
  std::shared_ptr<sqlite_statement> stmt=get_config().prepare(L"SELECT uid, title, description, icon_basename FROM search_engines");
  stmt->exec();
  while(*stmt)
  {
    // prepare database item
    database_item item;
    item.plugin_id=get_name();
    item.item_id=stmt->get_string(0);
    item.title=stmt->get_string(1);
    item.description=stmt->get_string(2);
    item.is_transient=false;
    item.icon_info.set(icon_source_theme, L"search_engines/"+stmt->get_string(3));
    item.on_enter=L"search_engine.open_homepage";
    item.on_tab=L"search_engine.open_brick";
    item.index_version=new_index_version;

    // add or update database
    get_db().add_or_update_item(item);
    stmt->next();
  }
}
//----------------------------------------------------------------------------

bool search_engines_plugin::on_action(const std::wstring &name, boost::optional<database_item> target)
{
  if(L"search_engine.open_homepage"==name && target)
  {
    // open search engine homepage
    std::shared_ptr<sqlite_statement> stmt=get_config().prepare(L"SELECT homepage_url FROM search_engines WHERE uid = ?");
    stmt->bind(0, target->item_id);
    bool ok = stmt->exec() && launch(stmt->get_string(0));
    get_gui().hide();
    return ok;
  }
  else if(L"search_engine.open_brick"==name && target)
  {
    // open search brick
    std::shared_ptr<sqlite_statement> stmt=get_config().prepare(L"SELECT search_url_template FROM search_engines WHERE uid = ?");
    stmt->bind(0, target->item_id);
    if(!stmt->exec())
      return false;
    get_gui().push_text_brick(new search_engine_controller(stmt->get_string(0)), target->icon_info);
    return true;
  }

  return false;
}
//----------------------------------------------------------------------------


//============================================================================
// search_engine_controller
//============================================================================
search_engine_controller::search_engine_controller(const std::wstring &url_template)
  :m_url_template(url_template)
{
}
//----------------------------------------------------------------------------

bool search_engine_controller::on_enter(gui &gui)
{
  // format URL
  wchar_t url[1024];
  _snwprintf(url, 1023, m_url_template.c_str(), gui.get_current_text());
  url[1023]=0;

  // open URL
  bool ok=launch(url);
  gui.hide();
  return ok;
}
//----------------------------------------------------------------------------
