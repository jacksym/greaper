cmake_minimum_required(VERSION 3.16)
project(greaper LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # compile_commands.json for clangd / vim LSP

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(REAPER_SDK_DIR "${CMAKE_SOURCE_DIR}/reaper-sdk/sdk")
set(WDL_DIR        "${CMAKE_SOURCE_DIR}/wdl/WDL")

# Where REAPER looks for extensions. Override with -DREAPER_USER_PLUGINS=...
if(APPLE)
  set(_default_plugins "$ENV{HOME}/Library/Application Support/REAPER/UserPlugins")
elseif(WIN32)
  set(_default_plugins "$ENV{APPDATA}/REAPER/UserPlugins")
else()
  set(_default_plugins "$ENV{HOME}/.config/REAPER/UserPlugins")
endif()
set(REAPER_USER_PLUGINS "${_default_plugins}" CACHE PATH "REAPER UserPlugins directory")

add_library(greaper SHARED src/main.cpp)

# reaper_plugin.h does #include "../WDL/swell/swell.h", so WDL_DIR must be on the path
target_include_directories(greaper PRIVATE "${REAPER_SDK_DIR}" "${WDL_DIR}")

if(NOT WIN32)
  # SWELL = Win32 API emulation (menus, windows). The modstub doesn't contain SWELL itself;
  # it receives SWELL's function pointers from the running REAPER at load time.
  target_sources(greaper PRIVATE
    "${WDL_DIR}/swell/swell-modstub$<IF:$<BOOL:${APPLE}>,.mm,-generic.cpp>")
  target_compile_definitions(greaper PRIVATE SWELL_PROVIDED_BY_APP)
endif()

set_target_properties(greaper PROPERTIES
  PREFIX ""                       # REAPER only loads files named reaper_*.so, not libreaper_*
  OUTPUT_NAME "reaper_greaper"
  CXX_VISIBILITY_PRESET hidden    # export only ReaperPluginEntry (+ SWELL_dllMain)
)

install(TARGETS greaper
  LIBRARY DESTINATION "${REAPER_USER_PLUGINS}"
  RUNTIME DESTINATION "${REAPER_USER_PLUGINS}")
