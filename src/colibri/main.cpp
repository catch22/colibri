//============================================================================
// main.cpp
//
// (c) Michael Walter, 2005-2007
//============================================================================

#include "core/defs.h"
#include "core/version.h"
#include "gui/gui.h"
#include "db/db.h"
#include "db/db_controller.h"
#include "plugins/colibri_plugin.h"
#include "plugins/standard_actions_plugin.h"
#include "plugins/filesystem_plugin.h"
#include "plugins/search_engines_plugin.h"
#include "plugins/control_panel_plugin.h"
#include "plugins/audio_plugin.h"
#include "plugins/winamp_plugin.h"
#include "libraries/win32/shell.h"
#include "libraries/log/log.h"
using namespace std;
using namespace boost;
//----------------------------------------------------------------------------


//============================================================================
// WinMain
//============================================================================
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  try
  {
    // check for other Colibri instances
    HANDLE mutex=CreateMutexW(0, TRUE, L"Local\\Colibri");
    if(!mutex)
      throw runtime_error("Unable to create 'Local\\Colibri' mutex.");
    if(GetLastError()==ERROR_ALREADY_EXISTS)
    {
      // popup other colibri instance
      gui::popup_other_colibri();

      // cleanup and exit
      CloseHandle(mutex);
      return 0;
    }

    // initialize logging
    logger::add_target(logger::create_file_target(profile_folder() / L"log.txt"));
    logger::add_target(logger::create_debug_target());

    // init utility library
    init_utils();

    // don't show error dialog boxes
    SetErrorMode(SEM_NOOPENFILEERRORBOX);

    // print startup info
    logger::infof("This is %S", str(colibri_version()).c_str());
#ifdef _DEBUG
    logger::warn("Colibri: Running debug version.");
#endif

    // parse command line parameters
    if(wcsstr(GetCommandLineW(), L"-whatsnew"))
      launch(L"http://colibri.leetspeak.org/whatsnew");
    
    // loop around the trampoline
    bool restart;
    do
    {
      restart=false;

      // load database
      database db;
      db.add_plugin(std::shared_ptr<plugin>(new colibri_plugin));

      // create gui 
      gui gui(db.get_colibri_plugin());
      db.set_gui(gui);
      gui.update_splash_screen(0.0f, str(colibri_version()));

      // load secondary plugins
      db.add_plugin(std::shared_ptr<plugin>(new standard_actions_plugin));
      db.add_plugin(std::shared_ptr<plugin>(new filesystem_plugin));
      db.add_plugin(std::shared_ptr<plugin>(new search_engines_plugin));
      db.add_plugin(std::shared_ptr<plugin>(new control_panel_plugin));
      db.add_plugin(std::shared_ptr<plugin>(new audio_plugin));
      db.add_plugin(std::shared_ptr<plugin>(new winamp_plugin));
      db.update_index();

      // create main brick
      gui.notify_startup_complete();
      gui.push_option_brick(new db_controller(db));

      // notify plugins that startup has completed
      db.trigger_action(L"global.startup_complete");

      // message loop
      MSG msg;
      while(GetMessage(&msg, 0, 0, 0))
      {
        // restart Colibri?
        if(WM_COLIBRI_RESTART==msg.message)
        {
          logger::info("Restarting Colibri...");
          restart=true;
          break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
    while(restart);

    // shutdown utility library
    shutdown_utils();
  }
  catch(std::exception &e_)
  {
    MessageBoxA(0, e_.what(), "Fatal Error", MB_OK|MB_ICONERROR);
    return 1;
  }

  return 0;
}
//----------------------------------------------------------------------------
