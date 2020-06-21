
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "luajit.h"
#include "lj_arch.h"
#include "lj_buf.h"
#include "lj_lib.h"
#include "lj_bcdump.h"

void DecSingle(lua_State* L, const char* _InputFilePath, const char* _OutputDir);
void DecDirectory(lua_State* L, const char* _InputDir, const char* _OutputDir);