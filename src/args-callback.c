#include <stdio.h>
#include <node_api.h>
#include <assert.h>
#include <windows.h>

static void call_js_fn(napi_env env, napi_callback_info info) {
	size_t argc = 2;     // 入参个数
	napi_value args[2];  // 入参数组
	assert(napi_get_cb_info(env, info, &argc, args, NULL, NULL) == napi_ok);

	napi_value json = args[0]; // 第一个参数 json 对象
	napi_value name_key, name; // json 中的 name
	napi_value age_key, age;   // json 中的 age
	napi_value cb = args[1];   // 第二个参数 function 回调

	// 将 C 语言的 char* name、age 变成 napi 的 string
	// 用于 napi 提供的一些列 API，如 napi_get_property
	napi_create_string_utf8(env, "name", NAPI_AUTO_LENGTH, &name_key);
	napi_create_string_utf8(env, "age", NAPI_AUTO_LENGTH, &age_key);

	napi_get_property(env, json, name_key, &name); // 取出 json 中的 name
	napi_get_property(env, json, age_key, &age);   // 取出 json 中的 age
	
	napi_value argv[] = {name, age}; // 调用 js 回调时候传入的参数
	napi_value global;
	napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
	napi_value result;
	Sleep(1000);
	napi_call_function( // 调用 js 回调函数
		env,    // 当前程序执行上下文
		global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
		cb,     // js 回调函数句柄
		2,      // js 回调函数接受参数个数
		argv,   // js 回调函数参数数组
		&result // js 回调函数中如果有 retrun，将会被 result 接受到
	);
}

/** Addons 入口 */
napi_value Init(napi_env env, napi_value exports) {
	// 以 function 形式导出
	// 使用：addon(args);
	napi_value new_exports;
	napi_status status = napi_create_function(env,
		"",                // function 名字(fn.name)
		NAPI_AUTO_LENGTH,  // 应该是(fn.length)
		call_js_fn,        // function 句柄
		NULL,              // 个人理解应该是 call 或者 applay 提供的第一个参数，欢迎大神补充 😭
		&new_exports);
	assert(status == napi_ok);
	return new_exports;

	/* 以对象的格式导出
	 * 使用：addon.call(args);
	napi_value obj;
	napi_create_object(env, &obj);
	napi_property_descriptor desc = {
		"call",
		NULL,
		call_js_fn,
		NULL,
		NULL,
		NULL,
		napi_default,
		NULL };
	napi_define_properties(env, obj, 1, &desc);
	return obj;*/
	// 可以理解为 exports == obj，所以导出用自定义的 obj 和注入的 exports 都可以的 :)
	// napi_define_properties(env, exports, 1, &desc);
	// return exports;
}


NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
