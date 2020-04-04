1. [C/C++ Addons 入门 Hello world!](https://www.jianshu.com/p/6b0d60672e04)
2. [C/C++ Addons 对象参数及回调函数](https://www.jianshu.com/p/210ab7c53732)
3. [C/C++ Addons 非阻塞多线程回调](https://www.jianshu.com/p/7dacbc9aa8f7)
4. [C/C++ Addons windows 下 .dll 动态链接库](https://www.jianshu.com/p/384bed7faf1c) **实用篇**

> [完整代码](https://github.com/caoxiemeihao/node-addons-learn)

- 啦啦啦，系列文章的最后一篇啦 ^_^
- 其实在我们实际工作中，大部分时间都是用 Addons 做一些 node.js 做不了的事情咯
- 调用 dll(windows动态链接库) 和操作系统打交道就是一个环节，甚至一些硬件也有 dll(可以理解为硬件的驱动)
- 除了我们实现的 C/C++ 调用 dll 插件，其实 npm 上面有一个叫 [ffi(node-ffi)](https://www.npmjs.com/package/ffi) 的包能做相同的事情，下面我们也会演示 😁
- 当然说到硬件调用的时候，如果用的是串口通讯的方式，可以用 [serialport](https://www.npmjs.com/package/serialport) 做串口通讯，当然这个库也是 C 编写的，需要依赖 `node-gyp`、`windows-build-tools`、`python` 有用这个的，碰到问题可以和我讨论哦

### user32.dll

- 这个是 windows 自带的一个 dll，在 windows 的系统路径下
- 我们有 C 去调用它，并且把接口暴露给 node.js

- C/C++ `src/call-dll.c`
```c
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
```

- javascript `test/call-dll.js`
```js
const addon = require('../build/Release/call_dll.node');

addon.alert('Hello world!'); // 如果碰到中午用 iconv-lite 转换成 gbk 编码即可
```

- 运行
```bash
$ node test/call-dll.js
Segmentation fault # 这个问题暂时还木有头绪 😭，不过还是能成功调用哒 😝
```

- 结果
![MessageBoxA](https://upload-images.jianshu.io/upload_images/6263326-cba299a3c32889f5.jpg?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

### node-ffi 使用

```js
// ffi 本质是用 node.js 提供的 Buffer 类和 .dll 共享内存(了解的不是很深入😂)
// 顺带提一下 Buffer 类的内存是 node.js 直接申请的，和 v8 管理的内存(js内置类型)不是在一起的 - 参考深入浅出 node.js
const ffi = require('ffi');
const iconv = require('iconv-lite');

// ffi.Library 加载 user32.dll
// 注册 MessageBoxA 既 user32.dll 内置的函数
const user32Lib = ffi.Library('user32.dll', {
  MessageBoxA: ['int', ['int', 'string', 'string', 'int']], // [返回值类型, [arg1, arg2, arg3, arg4]]
});

console.log(user32Lib);

user32Lib.MessageBoxA(
  0,
  iconv.encode('世界你好 :)', 'gbk'),
  'Hello World!',
  0);
```
- 运行
```bash
$ node user32
{ MessageBoxA: { [Function: proxy] async: [Function] } }
```

- 结果
![node-ffi.jpg](https://upload-images.jianshu.io/upload_images/6263326-3c2b000b1a970f0d.jpg?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

