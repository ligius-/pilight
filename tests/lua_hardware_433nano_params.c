/*
  Copyright (C) CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <wiringx.h>

#include "../libs/pilight/core/log.h"
#include "../libs/pilight/core/CuTest.h"
#include "../libs/pilight/core/pilight.h"
#include "../libs/pilight/core/common.h"
#include "../libs/pilight/core/eventpool.h"
#include "../libs/pilight/config/config.h"
#include "../libs/pilight/lua_c/lua.h"
#include "../libs/pilight/lua_c/lualibrary.h"
#include "../libs/pilight/lua_c/table.h"

#include "alltests.h"

static CuTest *gtc = NULL;

static int plua_print(lua_State* L) {
	plua_stack_dump(L);
	return 0;
}

static void close_cb(uv_handle_t *handle) {
	FREE(handle);
}

static void walk_cb(uv_handle_t *handle, void *arg) {
	if(!uv_is_closing(handle)) {
		uv_close(handle, close_cb);
	}
}

static void plua_overwrite_print(void) {
	struct lua_state_t *state[NRLUASTATES];
	struct lua_State *L = NULL;
	int i = 0;

	for(i=0;i<NRLUASTATES;i++) {
		state[i] = plua_get_free_state();

		if(state[i] == NULL) {
			return;
		}
		if((L = state[i]->L) == NULL) {
			uv_mutex_unlock(&state[i]->lock);
			return;
		}

		lua_getglobal(L, "_G");
		lua_pushcfunction(L, plua_print);
		lua_setfield(L, -2, "print");
		lua_pop(L, 1);
	}
	for(i=0;i<NRLUASTATES;i++) {
		uv_mutex_unlock(&state[i]->lock);
	}
}

static void run(CuTest *tc, const char *func, char *config, int status) {
	char *file = NULL;

	printf("[ %-48s ]\n", func);
	fflush(stdout);

	gtc = tc;
	memtrack();

	uv_replace_allocator(_MALLOC, _REALLOC, _CALLOC, _FREE);

	plua_init();

	test_set_plua_path(tc, __FILE__, "lua_hardware_433nano_params.c");

	plua_overwrite_print();

	file = STRDUP(__FILE__);
	CuAssertPtrNotNull(tc, file);

	str_replace("lua_hardware_433nano_params.c", "", &file);

	config_init();

	FILE *f = fopen("lua_hardware_433nano.json", "w");
	fprintf(f, config, file);
	fclose(f);

	FREE(file);

	CuAssertIntEquals(tc, 0, config_read("lua_hardware_433nano.json", CONFIG_SETTINGS));

	hardware_init();

	unlink("/tmp/usb0");
	int fd = open("/tmp/usb0", O_CREAT | O_RDWR, 0777);
	close(fd);

	CuAssertIntEquals(tc, status, config_read("lua_hardware_433nano.json", CONFIG_HARDWARE));

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	uv_walk(uv_default_loop(), walk_cb, NULL);
	uv_run(uv_default_loop(), UV_RUN_ONCE);

	while(uv_loop_close(uv_default_loop()) == UV_EBUSY) {
		uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	}

	storage_gc();
	plua_gc();
	hardware_gc();

	CuAssertIntEquals(tc, 0, xfree());
}

void test_lua_hardware_433nano_param1(CuTest *tc) {
	char config[1024] =
		"{\"devices\":{},\"gui\":{},\"rules\":{},\
		\"settings\":{\
				\"hardware-root\":\"%s../libs/pilight/hardware/\"\
		},\
		\"hardware\":{\"433nano\":{}},\
		\"registry\":{}}";

	run(tc, __FUNCTION__, config, -1);
}

void test_lua_hardware_433nano_param2(CuTest *tc) {
	char config[1024] =
		"{\"devices\":{},\"gui\":{},\"rules\":{},\
		\"settings\":{\
				\"hardware-root\":\"%s../libs/pilight/hardware/\"\
		},\
		\"hardware\":{\"433nano\":{\"comport\":\"/tmp/usb0\"}},\
		\"registry\":{}}";

	run(tc, __FUNCTION__, config, 0);
}

void test_lua_hardware_433nano_param3(CuTest *tc) {
	char config[1024] =
		"{\"devices\":{},\"gui\":{},\"rules\":{},\
		\"settings\":{\
				\"hardware-root\":\"%s../libs/pilight/hardware/\"\
		},\
		\"hardware\":{\"433nano\":{\"comport\":\"/tmp/usb1\"}},\
		\"registry\":{}}";

	run(tc, __FUNCTION__, config, -1);
}

void test_lua_hardware_433nano_param4(CuTest *tc) {
	char config[1024] =
		"{\"devices\":{},\"gui\":{},\"rules\":{},\
		\"settings\":{\
				\"hardware-root\":\"%s../libs/pilight/hardware/\"\
		},\
		\"hardware\":{\"433nano\":{\"comport\":\"/tmp/usb0\",\"foo\":1}},\
		\"registry\":{}}";

	run(tc, __FUNCTION__, config, -1);
}