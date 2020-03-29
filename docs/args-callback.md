### 系列文章

1. [C/C++ Addons 入门 Hello world!](https://www.jianshu.com/p/6b0d60672e04)
2. [C/C++ Addons 对象参数及回调函数](https://www.jianshu.com/p/210ab7c53732)
3. C/C++ Addons 非阻塞多线程回调
4. C/C++ Addons windows 下 .dll 动态链接库 **实用篇**

> [完整代码](https://github.com/caoxiemeihao/node-addons-learn)

写 node.js C/C++ 插件目的就是为了扩展一些 node.js 自身不具备的功能，在 node.js 中有些模块自身就是 C/C++ 写的，和 C/C++ 插件可以说很像的东西了，如: `Buffer` `crypro` `evals` `fs` `http` `os` `zlib` `tcp` `ddp` 等(参考深入浅出nodejs)。

-  拿我们常用的 `http` 来举例，如果你想读一个请求一段网络数据 (node.js官方例子)：
```js
const http = require('http');
const postData = querystring.stringify({
  'msg': 'Hello World!'
});

const options = {
  hostname: 'www.google.com',
  port: 80,
  path: '/upload',
  method: 'POST',
  headers: {
    'Content-Type': 'application/x-www-form-urlencoded',
    'Content-Length': Buffer.byteLength(postData)
  }
};

const req = http.request(options, (res) => {
  console.log(`STATUS: ${res.statusCode}`);
  console.log(`HEADERS: ${JSON.stringify(res.headers)}`);
  res.setEncoding('utf8');
  res.on('data', (chunk) => {
    console.log(`BODY: ${chunk}`);
  });
  res.on('end', () => {
    console.log('No more data in response.');
  });
});

req.on('error', (e) => {
  console.error(`problem with request: ${e.message}`);
});

// Write data to request body
req.write(postData);
req.end();
```
可以看出来，`http.request` 第一个参数是 `Object` 第二个参数是 `function`，那么我们也试试写一个支持同样参数的 C/C++ Addons 出来试试吧 😊

- C/C++ 代码 `src/args-callback.c`
```c
#include <stdio.h>
#include <node_api.h>
#include <assert.h>

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

```

- javascript `test/args-callback.js`
```js
const addon = require('../build/Release/args_callback.node');

const arg = { name: 'anan', age: 29 };
const cb = function (name, age) {
  console.log(`The name is ${name}`, `\nAge is ${age}`);
  console.log(this === global); // true
};

addon(arg, cb); // 如果导出的是 function
// addon.call(arg, cb); // 如果导出的是 exports 对象，并且 call 挂载在 exports 上
```

- 运行一下 `完美`
```bash
$ node test/args-callback.js
The name is anan
Age is 29
true
```

乍一看来我们 “实现了” **对象参数及回调函数** 了呢 😝，都知道 `http.request` 是异步的，所以不会阻塞 js 线程，那么我们试试模拟下 **异步**

- C/C++ 改造
```c
// windows.h 里面提供了 Sleep
#include <windows.h>
...
// 模拟等待 1 秒后执行
Sleep(1000);
napi_call_function(...);
```

- javascript 改造
```js
// 0.1 秒之后执行
setTimeout(() => console.log('Timeout is executed.'), 100);
// 调用 Addons
addon({ name: 'anan', age: 29 }, function (name, age) {...}); 
```
按我们一贯使用 node.js 异步的思维，这段程序应该会先输出 `Timeout is executed.` 然后再输出 `name, age的值`
- 运行一下
```bash
$ node test/args-callback.js
The name is anan
Age is 29
true
Timeout is executed.
```
哎吗呀！打脸了 😭，为啥子会酱紫呢？
其实 Addons 中的 C/C++ 代码执行共用的我们 node.js 设计的 **单进程单线程** 模型。
Addons 中使用了 `Sleep(1000)` 那么就真的是会把我们的线程休眠`1秒`哦 (机智的你是不是已经发现了，可以用 C/C++ Addons 在 node.js 中实现和其他语言一样的 sleep 呢)。
那么话说回来，这个问题肿莫办捏？
下一篇进阶文章 **Nodejs C/C++ Addons 非阻塞多线程回调** 我们来解决下这个问题.😜

