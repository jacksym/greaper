// greaper — git version control for REAPER projects.
// Hello-world stage: registers one action and puts it in the Extensions menu.

#include <cstring>

// Pull in only the REAPER API functions we use, and define their pointers in this file.
// (See reaper-sdk/sdk/reaper_plugin_functions.h:37-48 for how this mechanism works.)
#define REAPERAPI_IMPLEMENT
#define REAPERAPI_MINIMAL
#define REAPERAPI_WANT_plugin_register
#define REAPERAPI_WANT_ShowConsoleMsg
#define REAPERAPI_WANT_AddExtensionsMainMenu
#include "reaper_plugin_functions.h"

namespace {

int g_helloCmd = 0; // command ID REAPER assigns to our action

// Shows up in the Actions list; REAPER uses .accel.cmd as the action ID.
gaccel_register_t g_helloAccel = {{0, 0, 0}, "greaper: Hello"};

void DoHello()
{
    ShowConsoleMsg("Hello from greaper!\n");
}

// REAPER calls this for EVERY action anyone runs. Return true only for ours.
bool OnCommand(int cmd, int /*flag*/)
{
    if (cmd == g_helloCmd) {
        DoHello();
        return true;
    }
    return false;
}

// REAPER calls this when building menus. flag==0 means "menu is being created".
void OnMenu(const char* menuId, HMENU menu, int flag)
{
    if (flag != 0 || strcmp(menuId, "Main extensions") != 0) return;

    MENUITEMINFO mi = {};
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_TYPE | MIIM_ID;
    mi.fType = MFT_STRING;
    mi.wID = g_helloCmd; // clicking the item runs the action -> OnCommand
    mi.dwTypeData = (char*)"greaper: Hello";
    InsertMenuItem(menu, GetMenuItemCount(menu), true, &mi);
}

void Unregister()
{
    plugin_register("-hookcustommenu", (void*)OnMenu);
    plugin_register("-hookcommand", (void*)OnCommand);
    plugin_register("-gaccel", &g_helloAccel);
}

} // namespace

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
    REAPER_PLUGIN_HINSTANCE /*hInstance*/, reaper_plugin_info_t* rec)
{
    // rec == NULL: REAPER is unloading us (on quit).
    if (!rec) {
        if (plugin_register) Unregister();
        return 0;
    }

    if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc) return 0;

    // Look up every REAPERAPI_WANT_ function by name; returns number of failures.
    if (REAPERAPI_LoadAPI(rec->GetFunc) != 0) return 0;

    // 1. Reserve a stable command ID for our action's string name.
    g_helloCmd = plugin_register("command_id", (void*)"GREAPER_HELLO");
    if (!g_helloCmd) return 0;

    // 2. Add it to the Actions list.
    g_helloAccel.accel.cmd = g_helloCmd;
    plugin_register("gaccel", &g_helloAccel);

    // 3. Handle it when it runs, and put it in the Extensions menu.
    rec->Register("hookcommand", (void*)OnCommand);
    rec->Register("hookcustommenu", (void*)OnMenu);
    AddExtensionsMainMenu();

    return 1; // stay loaded
}
