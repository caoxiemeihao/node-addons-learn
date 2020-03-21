#include <stdio.h>
#include <node_api.h>
#include <string.h>

napi_value Hello(napi_env env, napi_callback_info info) {
	size_t argc = 1;         // 只接受一个参数
	napi_value argv;         // 接收到的参数
	char n[40];
	char hello[90] = "Hello ";
	napi_value result;
	napi_get_cb_info(env, info, &argc, &argv, NULL, NULL);                     // 获取接收参数
	napi_get_value_string_utf8(env, argv, n, sizeof(n), NULL);                 // 将接收到的参数转换为 C 语言类型
	napi_create_string_utf8(env, strcat(hello, n), NAPI_AUTO_LENGTH, &result); // 拼接字符串

	return result;
}

napi_value Init(napi_env env, napi_value exports) {
	// 描述 hello 属性
	napi_property_descriptor desc = {
		"hello",
		NULL,
		Hello,
		NULL,
		NULL,
		NULL,
		napi_default,
		NULL };
	// 将 hello 挂载到 exports 上面 === require('hello.node').hello;
	napi_define_properties(env, exports, 1, &desc);

	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
