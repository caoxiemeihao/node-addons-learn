#include <stdio.h>
#include <windows.h>
#include <assert.h>
// 开启 N-API 实验性功能，如：多线程 napi_threadsafe_function
#define NAPI_EXPERIMENTAL
#include <node_api.h>

typedef struct{
	napi_async_work work;
	napi_threadsafe_function tsfn;
}Addon;

static void print_err(napi_env env, napi_status status) {
	if (status != napi_ok) {
		const napi_extended_error_info* error_info = NULL;
		napi_get_last_error_info(env, &error_info);
		printf("%s\n", error_info->error_message);
		exit(0);
	}
}

static void call_js(napi_env env, napi_value js_cb, void* context, void* data) {
	(void)context; // 用不到它
	
	if (env != NULL) {
		napi_value undefined, js_prime;

		napi_get_undefined(env, &undefined);
		print_err(env, napi_call_function(env, undefined, js_cb, 0, NULL, NULL));
	}
}

static void execute_work(napi_env env, void* data) {
	Addon* addon = (Addon*)data;

	print_err(env, napi_acquire_threadsafe_function(addon->tsfn));
	Sleep(4000);
	print_err(env, napi_call_threadsafe_function(addon->tsfn, NULL, napi_tsfn_blocking));
	print_err(env, napi_release_threadsafe_function(addon->tsfn, napi_tsfn_release));
}

static void work_complete(napi_env env, napi_status status, void* data) {
	Addon* addon = (Addon*)data;

	print_err(env, napi_release_threadsafe_function(addon->tsfn, napi_tsfn_release));
	print_err(env, napi_delete_async_work(env, addon->work));

	addon->work = NULL;
	addon->tsfn = NULL;
}

static napi_value start_thread(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value js_cb, work_name;
	Addon* addon;
	napi_status sts;
	
	sts = napi_get_cb_info(env, info, &argc, &js_cb, NULL, (void**)(&addon));
	print_err(env, sts);

	assert(addon->work == NULL && "Only one work item must exist at a time");

	sts = napi_create_string_utf8(env,
		"N-API Thread-safe Call from Async Work Item",
		NAPI_AUTO_LENGTH,
		&work_name);
	print_err(env, sts);

	sts = napi_create_threadsafe_function(env,
		js_cb,
		NULL,
		work_name,
		0,
		1,
		NULL,
		NULL,
		NULL,
		call_js,
		&(addon->tsfn));
	print_err(env, sts);

	sts = napi_create_async_work(env,
		NULL,
		work_name,
		execute_work,
		work_complete,
		addon,
		&(addon->work));
	print_err(env, sts);

	sts = napi_queue_async_work(env, addon->work);
	print_err(env, sts);

	return NULL;
}

static void addon_getting_unloaded(napi_env env, void* data, void* hint) {
	Addon* addon = (Addon*)data;
	assert(addon->work == NULL && "No work in progress at module unload");
	free(addon);
}

napi_value init(napi_env env, napi_value exports) {
	Addon* addon = (Addon*)malloc(sizeof(*addon));
	addon->work = NULL;

	napi_property_descriptor desc = {
		"start",
		NULL,
		start_thread,
		NULL,
		NULL,
		NULL,
		napi_default,
		addon
	};
	napi_define_properties(env, exports, 1, &desc);
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)

// 另一个导出模块的宏定义(有空再研究🙃)
//NAPI_MODULE_INIT() {
//	Addon* addon = (Addon*)malloc(sizeof(*addon));
//	addon->work = NULL;
//
//	napi_property_descriptor desc = {
//		"start",
//		NULL,
//		start_thread,
//		NULL,
//		NULL,
//		NULL,
//		napi_default,
//		addon
//	};
//
//	napi_define_properties(env, exports, 1, &desc);
//	napi_wrap(env, exports, addon, addon_getting_unloaded, NULL, NULL);
//	// return exports; // 可写可不写
//}
