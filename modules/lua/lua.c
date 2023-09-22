// Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    Portions Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <frontend.h>
#include <vxt/vxtu.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#include "crc32.h"

#define MAX_NAME_LEN 128

#define GET_DEV struct lua_env *e = (struct lua_env*)lua_touserdata(L, lua_upvalueindex(1));

struct lua_env {
	char name[MAX_NAME_LEN];
	lua_State *L;
};

static vxt_byte in(struct lua_env *e, vxt_word port) {
    vxt_byte data = 0xFF;
	if (lua_getglobal(e->L, "io_in") == LUA_TFUNCTION) {
		lua_pushinteger(e->L, port);
    	lua_call(e->L, 1, 1);
		data = (vxt_byte)lua_tointeger(e->L, -1);
	}
	lua_pop(e->L, 1);
    return data;
}

static void out(struct lua_env *e, vxt_word port, vxt_byte data) {
	if (lua_getglobal(e->L, "io_out") == LUA_TFUNCTION) {
		lua_pushinteger(e->L, port);
		lua_pushinteger(e->L, data);
    	lua_call(e->L, 2, 0);
	} else {
		lua_pop(e->L, 1);
	}
}

static vxt_byte mem_read(struct lua_env *e, vxt_pointer addr) {
    vxt_byte data = 0xFF;
	if (lua_getglobal(e->L, "mem_read") == LUA_TFUNCTION) {
		lua_pushinteger(e->L, addr);
    	lua_call(e->L, 1, 1);
		data = (vxt_byte)lua_tointeger(e->L, -1);
	}
	lua_pop(e->L, 1);
    return data;
}

static void mem_write(struct lua_env *e, vxt_pointer addr, vxt_byte data) {
	if (lua_getglobal(e->L, "mem_write") == LUA_TFUNCTION) {
		lua_pushinteger(e->L, addr);
		lua_pushinteger(e->L, data);
    	lua_call(e->L, 2, 0);
	} else {
		lua_pop(e->L, 1);
	}
}

static vxt_error config(struct lua_env *e, const char *section, const char *key, const char *value) {
	if (lua_getglobal(e->L, "config") == LUA_TFUNCTION) {
		lua_pushstring(e->L, section);
		lua_pushstring(e->L, key);
		lua_pushstring(e->L, value);
    	lua_call(e->L, 3, 0);
	} else {
		lua_pop(e->L, 1);
	}
    return VXT_NO_ERROR;
}

static const char *name(struct lua_env *e) {
	if (lua_getglobal(e->L, "name") == LUA_TFUNCTION) {
    	lua_call(e->L, 0, 1);
		strncpy(e->name, lua_tostring(e->L, -1), MAX_NAME_LEN - 1);
		return e->name;
	} else {
		lua_pop(e->L, 1);
		return "Lua Script";
	}
}

static vxt_error install(struct lua_env *e, vxt_system *s) {
	(void)s;
    if (lua_getglobal(e->L, "install") == LUA_TFUNCTION)
    	lua_call(e->L, 0, 0);
	else
		lua_pop(e->L, 1);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct lua_env *e) {
    if (lua_getglobal(e->L, "destroy") == LUA_TFUNCTION)
    	lua_call(e->L, 0, 0);
	else
		lua_pop(e->L, 1);

	lua_close(e->L);
	vxt_system_allocator(VXT_GET_SYSTEM(e))(VXT_GET_PIREPHERAL(e), 0);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct lua_env *e) {
    if (lua_getglobal(e->L, "reset") == LUA_TFUNCTION)
    	lua_call(e->L, 0, 0);
	else
		lua_pop(e->L, 1);
    return VXT_NO_ERROR;
}

static vxt_error timer(struct lua_env *e, vxt_timer_id id, int cycles) {
	if (lua_getglobal(e->L, "timer") == LUA_TFUNCTION) {
		lua_pushinteger(e->L, id);
		lua_pushinteger(e->L, cycles);
    	lua_call(e->L, 2, 0);
	} else {
		lua_pop(e->L, 1);
	}
    return VXT_NO_ERROR;
}

static void *lua_allocator(void *ud, void *ptr, size_t osize, size_t nsize) {
	(void)ud; (void)osize;
	if (!nsize) {
		free(ptr);
		return NULL;
	} else {
		return realloc(ptr, nsize);
	}
}

static int lua_error_handle(lua_State *L) {
	VXT_PRINT("%s\n", lua_tostring(L, -1));
	return 0;
}

static int lua_install_io(lua_State *L) {
	GET_DEV
	vxt_system_install_io(VXT_GET_SYSTEM(e), VXT_GET_PIREPHERAL(e), (vxt_word)lua_tointeger(L, -2), (vxt_word)lua_tointeger(L, -1));
	return 0;
}

static int lua_install_io_at(lua_State *L) {
	GET_DEV
	vxt_system_install_io_at(VXT_GET_SYSTEM(e), VXT_GET_PIREPHERAL(e), (vxt_word)lua_tointeger(L, -1));
	return 0;
}

static int lua_install_mem(lua_State *L) {
	GET_DEV
	vxt_system_install_mem(VXT_GET_SYSTEM(e), VXT_GET_PIREPHERAL(e), (vxt_pointer)lua_tointeger(L, -2), (vxt_pointer)lua_tointeger(L, -1));
	return 0;
}

static int lua_install_timer(lua_State *L) {
	GET_DEV
	lua_pushinteger(L, vxt_system_install_timer(VXT_GET_SYSTEM(e), VXT_GET_PIREPHERAL(e), (unsigned int)lua_tointeger(L, -1)));
	return 1;
}

static int lua_interrupt(lua_State *L) {
	GET_DEV
	vxt_system_interrupt(VXT_GET_SYSTEM(e), (int)lua_tointeger(L, -1));
	return 0;
}

static int lua_wait(lua_State *L) {
	GET_DEV
	vxt_system_wait(VXT_GET_SYSTEM(e), (int)lua_tointeger(L, -1));
	return 0;
}

static int lua_frequency(lua_State *L) {
	GET_DEV
	lua_pushinteger(L, vxt_system_frequency(VXT_GET_SYSTEM(e)));
	return 1;
}

static int lua_read_byte(lua_State *L) {
	GET_DEV
	lua_pushinteger(L, vxt_system_read_byte(VXT_GET_SYSTEM(e), (vxt_pointer)lua_tointeger(L, -1)));
	return 1;
}

static int lua_read_word(lua_State *L) {
	GET_DEV
	lua_pushinteger(L, vxt_system_read_word(VXT_GET_SYSTEM(e), (vxt_pointer)lua_tointeger(L, -1)));
	return 1;
}

static int lua_write_byte(lua_State *L) {
	GET_DEV
	vxt_system_write_byte(VXT_GET_SYSTEM(e), (vxt_pointer)lua_tointeger(L, -2), (vxt_byte)lua_tointeger(L, -1));
	return 0;
}

static int lua_write_word(lua_State *L) {
	GET_DEV
	vxt_system_write_word(VXT_GET_SYSTEM(e), (vxt_pointer)lua_tointeger(L, -2), (vxt_word)lua_tointeger(L, -1));
	return 0;
}

static int lua_os_opendir(lua_State *L) {
	if (!lua_isstring(L, -1))
		return 0;
	DIR *dir = opendir(lua_tostring(L, -1));
	if (!dir)
		return 0;
	lua_pushlightuserdata(L, dir);
	return 1;
}

static int lua_os_closedir(lua_State *L) {
	if (lua_islightuserdata(L, -1))
		lua_pushboolean(L, closedir(lua_touserdata(L, -1)) ? 0 : 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}

static int lua_os_readdir(lua_State *L) {
	if (lua_islightuserdata(L, -1)) {
		struct dirent *e = readdir(lua_touserdata(L, -1));
		if (!e)
			return 0;

		lua_pushlightuserdata(L, e);
		lua_pushstring(L, e->d_name);
		return 2;
	}
	return 0;
}

static int lua_os_stat(lua_State *L) {
	if (lua_isstring(L, -1)) {
		struct stat stbuf;
        if (stat(lua_tostring(L, -1), &stbuf))
			return 0;
		
		lua_pushinteger(L, stbuf.st_size);
        lua_pushboolean(L, S_ISDIR(stbuf.st_mode) ? 1 : 0);

		struct tm *t = localtime(&stbuf.st_mtime);
		lua_newtable(L);
		{
			lua_pushinteger(L, t->tm_hour);
			lua_setfield(L, -2, "hour");
			lua_pushinteger(L, t->tm_min);
			lua_setfield(L, -2, "min");
			lua_pushinteger(L, t->tm_sec);
			lua_setfield(L, -2, "sec");

			lua_pushinteger(L, t->tm_year);
			lua_setfield(L, -2, "year");
			lua_pushinteger(L, t->tm_mon);
			lua_setfield(L, -2, "mon");
			lua_pushinteger(L, t->tm_mday);
			lua_setfield(L, -2, "mday");
		}
		return 3;
	}
	return 0;
}

static int lua_os_mkdir(lua_State *L) {
	if (!lua_isstring(L, -1)) {
		lua_pushboolean(L, 0);
	} else {
		const char *path = lua_tostring(L, -1);
		int ret =
		#ifdef _WIN32
			mkdir(path);
		#else
			mkdir(path, S_IRWXU);
		#endif
		lua_pushboolean(L, ret ? 0 : 1);
	}
	return 1;
}

static int lua_math_crc32(lua_State *L) {
	if (!lua_isstring(L, -1))
		return 0;
	
	const char *data = lua_tostring(L, -1);
	lua_pushinteger(L, crc32((const vxt_byte*)data, (int)luaL_len(L, -1)));
	return 1;
}

static bool initialize_environement(struct lua_env *e, const char *args, void *frontend) {
	char script[FILENAME_MAX];
	strncpy(script, args, FILENAME_MAX - 1);

	const char *params = strchr(args, ',');
	if (params) {
		int ln = (int)(params - args);
		script[ln] = 0;
		params++;
	}

	struct frontend_interface *fi = (struct frontend_interface*)frontend;
    if (fi && fi->resolve_path)
		strcpy(script, fi->resolve_path(FRONTEND_MODULE_PATH, script));

	if (!(e->L = lua_newstate(&lua_allocator, NULL)))
		return false;

	lua_atpanic(e->L, &lua_error_handle);
	luaL_openlibs(e->L);

	#define MAP_FUNC(name) {						\
		lua_pushlightuserdata(e->L, e);				\
		lua_pushcclosure(e->L, &lua_ ## name, 1);	\
		lua_setglobal(e->L, #name);	}				\

	MAP_FUNC(install_io);
	MAP_FUNC(install_io_at);
	MAP_FUNC(install_mem);
	MAP_FUNC(install_timer);

	MAP_FUNC(interrupt);
	MAP_FUNC(wait);
	MAP_FUNC(frequency);

	MAP_FUNC(read_byte);
	MAP_FUNC(read_word);
	MAP_FUNC(write_byte);
	MAP_FUNC(write_word);

	#undef MAP_FUNC

	lua_getglobal(e->L, "os");
	{
		lua_pushcfunction(e->L, &lua_os_opendir);
		lua_setfield(e->L, -2, "opendir");
		lua_pushcfunction(e->L, &lua_os_closedir);
		lua_setfield(e->L, -2, "closedir");
		lua_pushcfunction(e->L, &lua_os_readdir);
		lua_setfield(e->L, -2, "readdir");
		lua_pushcfunction(e->L, &lua_os_stat);
		lua_setfield(e->L, -2, "stat");
		lua_pushcfunction(e->L, &lua_os_mkdir);
		lua_setfield(e->L, -2, "mkdir");
	}
	lua_pop(e->L, 1);

	lua_getglobal(e->L, "math");
	{
		lua_pushcfunction(e->L, &lua_math_crc32);
		lua_setfield(e->L, -2, "crc32");
	}
	lua_pop(e->L, 1);

	if (luaL_loadfile(e->L, script) != LUA_OK) {
		VXT_LOG(lua_tostring(e->L, -1));
		return false;
	}

	int num_params = 0;
	if (params) {
		char *pch = NULL;
		strncpy(e->name, params, MAX_NAME_LEN - 1);
		pch = strtok(e->name, ",");
		while (pch) {
			lua_pushstring(e->L, pch);
			num_params++;
			pch = strtok(NULL, ",");
		}
	}

	lua_call(e->L, num_params, LUA_MULTRET);
	return true;
}

VXTU_MODULE_CREATE(lua_env, {
	if (!initialize_environement(DEVICE, ARGS, FRONTEND))
		return NULL;

    PIREPHERAL->install = &install;
	PIREPHERAL->destroy = &destroy;
	PIREPHERAL->reset = &reset;
    PIREPHERAL->config = &config;
    PIREPHERAL->name = &name;
    PIREPHERAL->timer = &timer;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
    PIREPHERAL->io.read = &mem_read;
    PIREPHERAL->io.write = &mem_write;
})
