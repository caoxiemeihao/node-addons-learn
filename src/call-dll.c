#include <windows.h>
#include <stdio.h>
#include <node_api.h>

void alert(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv;
	char msg[90];

	napi_get_cb_info(env, info, &argc, &argv, NULL, NULL);
	napi_get_value_string_utf8(env, argv, &msg, sizeof(msg), NULL);

	// user32.dll 在系统路径下，所以直接加载即可
	// C:\Windows\System32\user32.dll
	// HANDLE 代表句柄
	HANDLE module = LoadLibrary("user32.dll");
	if (module == NULL) {
		printf("Failed to load user32.dll");
		exit(1);
	}

	// 定义指针函数，用于接收 dll 中的方法 MessageBoxA
	// 返回类型(*函数名)(参数表)
	typedef int(*MessageBoxA)(int, char[90], char[90], int);
	MessageBoxA box = (MessageBoxA)GetProcAddress(module, "MessageBoxA");
	box(
		0,    // 窗口句柄
		msg,  // content
		":)", // title
		0     // 提示图标和按钮类型
	);
}

void init(napi_env env, napi_value exports) {
	napi_property_descriptor desc = { "alert", NULL, alert, };
	napi_define_properties(env, exports, 1, &desc);
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
