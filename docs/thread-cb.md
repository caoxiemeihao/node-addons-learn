### 系列文章

1. [C/C++ Addons 入门 Hello world!](https://www.jianshu.com/p/6b0d60672e04)
2. [C/C++ Addons 对象参数及回调函数](https://www.jianshu.com/p/210ab7c53732)
3. [C/C++ Addons 非阻塞多线程回调]()
4. C/C++ Addons windows 下 .dll 动态链接库 **实用篇**

> [完整代码](https://github.com/caoxiemeihao/node-addons-learn)

- 这篇文章是解决上一篇文章碰到的 Sleep() 后线程 `阻塞(休眠)` 问题探索的解决方案

**node.js 异步理解** *(参考深入浅出 node.js)*
- 简单的交代下：
所谓 node.js 异步事件环，底层使用了类似 `while(true)` 的方式开了一个死循环，然后在每次循环的时候去异步队列中取出一个或多个待执行的任务执行。
待执行任务来自 `node.js` 提供的异步接口如：`setTimeout` 或 `fs`、`http` 模块提供的异步方法等，这里不一一列出。
任何任务都需要线程去执行，既然主线程不执行他们那么这些非阻塞的任务是被怎样执行的呢？其实本质上 `node.js` 底层还是使用了**多线程**。
当然 `windows` 和 `*nix` 平台实现略有不同，`windows` 底层由操作系统接管， `*nix` 底层是 `node.js` 自己实现的多线程任务池，这里不展开细讲。

![image](https://raw.githubusercontent.com/caoxiemeihao/node-addons-learn/master/screenshot/thread.png)

### 代码实现

- C/C++ `src/thread-cb.c`
```c
#include <stdio.h>
#include <windows.h>
#include <assert.h>
// 开启 N-API 实验性功能，如：多线程 napi_threadsafe_function
#define NAPI_EXPERIMENTAL
#include <node_api.h>

typedef struct{
	napi_async_work work;          // 保存线程任务的
	napi_threadsafe_function tsfn; // 保存回调函数的
}Addon;

/** 调试报错用的 */
static void catch_err(napi_env env, napi_status status) {
	if (status != napi_ok) {
		const napi_extended_error_info* error_info = NULL;
		napi_get_last_error_info(env, &error_info);
		printf("%s\n", error_info->error_message);
		exit(0);
	}
}

/** 调用 js-callback 用的 */
static void call_js(napi_env env, napi_value js_cb, void* context, void* data) {
	(void)context; // 用不到它
	(void)data;    // 用不到它
	
	if (env != NULL) {
		napi_value undefined, js_prime;

		napi_get_undefined(env, &undefined); // 创建一个 js 的 undefined
		catch_err(env, napi_call_function(env,
			undefined, // js 回调的 this 对象
			js_cb,     // js 回调函数句柄
			0,         // js 回调函数接受参数个数
			NULL,      // js 回调函数参数数组
			NULL));    // js 回调函数中如果有 retrun，将会被 result 接受到，NULL 代表忽略
	}
}

/** 执行线程 */
static void execute_work(napi_env env, void* data) {
	Addon* addon = (Addon*)data;

	// 拿到 js-callback 函数
	catch_err(env, napi_acquire_threadsafe_function(addon->tsfn));

	// 延迟四秒执行
	Sleep(4000);

	// 调用 js-callback 函数
	catch_err(env, napi_call_threadsafe_function(
		addon->tsfn,          // js-callback 函数
		NULL,                 // call_js 的第四个参数
		napi_tsfn_blocking)); // 阻塞模式调用

	// 释放句柄
	catch_err(env, napi_release_threadsafe_function(addon->tsfn, napi_tsfn_release));
}

/** 线程执行完成 */
static void work_complete(napi_env env, napi_status status, void* data) {
	Addon* addon = (Addon*)data;

	// 释放句柄
	catch_err(env, napi_release_threadsafe_function(addon->tsfn, napi_tsfn_release));

	// 回收任务
	catch_err(env, napi_delete_async_work(env, addon->work));

	addon->work = NULL;
	addon->tsfn = NULL;
}

/**
 * start_thread 启动线程
 * 关于 static 关键字用不用都行的，官方的例子有用到 static
 * 以我 js 的能力我猜的可能是开多个线程下，可以公用一个函数，节约内存开销 (欢迎大神来讨论 😭)
 */
static napi_value start_thread(napi_env env, napi_callback_info info) {
	size_t argc = 1;       // js 传进来的参数个数
	napi_value js_cb;      // js 传进来的回调函数
	napi_value work_name;  // 给线程起个名字
	Addon* addon;          // “实例化” 结构体 (个人理解是取出了传进来的 js-cabllback 地址指针，期待大神来讨论 😭)
	napi_status sts;       // 程序执行状态
	
	sts = napi_get_cb_info(
		env,               // 执行上下文，可以理解为 js 那个 “事件环”
		info,              // 上下文信息
		&argc,             // 收到参数的个数
		&js_cb,            // 接收 js 参数
		NULL,              // 接收 js 的 this 对象
		(void**)(&addon)   // 取得 js 传进来的 callback 的指针地址
	);
	catch_err(env, sts);

	// 打酱油的 ^_^
	assert(addon->work == NULL && "Only one work item must exist at a time");

	// 创建线程名字
	catch_err(env, napi_create_string_utf8(env,
		"N-API Thread-safe Call from Async Work Item",
		NAPI_AUTO_LENGTH,
		&work_name));

	// 把 js function 变成任意线程都可以执行的函数
	// 酱紫我们就可以在开出来的子线程中调用它咯
	sts = napi_create_threadsafe_function(env,

		// 其他线程的 js 函数
		// call_js 的第二个参数
		// 也就是我们 addon.start(function) 传进来的 function
		js_cb,

		// 可能传递给一些异步任务async_hooks钩子传递初始化数据 (期待大神讨论 😊)
		// 个人理解 N-API 中的 async 指的就是多线程任务
		// 一个线程任务，在 N-API 中由 async work 调用
		NULL,

		work_name,       // 给线程起个名字，给 async_hooks 钩子提供一个标识符
		0,               // (官网直译)最大线程队列数量，0 代表没限制
		1,               // (官网直译)初始化线程数量，其中包括主线程
		NULL,            // (官网直译)线程之间可以传递数据(官网直译)
		NULL,            // (官网直译)线程之间可以传递函数，函数注销时候被调用
		NULL,            // (官网直译)附加给函数的执行上下文，应该就是 
		call_js,         // call_js 的第三个参数
		&(addon->tsfn)); // js 传进来的函数，可以理解为真实的 js 函数所在内存地址 (期待大神讨论 😊)
	catch_err(env, sts);

	// 负责执行上面创建的函数
	sts = napi_create_async_work(env,
		NULL,            // 可能传递 async_hooks 一些初始化数据
		work_name,       // 给线程起个名字，给 async_hooks 钩子提供一个标识符
		execute_work,    // 线程执行时候执行的函数 (与主线程并行执行)
		work_complete,   // 线程执行完时候的回调
		addon,           // 既 execute_work、work_complete 中的 void* data
		&(addon->work)); // 线程句柄
	catch_err(env, sts);

	// 将线程放到待执行队列中
	sts = napi_queue_async_work(env,
		// 要执行线程的句柄
		addon->work);
	catch_err(env, sts);

	return NULL; // 这个貌似是返回给 js-callback 的返回值
}

napi_value init(napi_env env, napi_value exports) {
	// 这里等价于 const obj = new Object();
	// 这回知道面向对象是咋来的了吧 😁
	// 类的本质就是“结构体”演化而来的，new(开辟堆内存空间) 关键字是 malloc(申请内存空间) 变种过来的
	Addon* addon = (Addon*)malloc(sizeof(*addon));

	// 等价于 obj.work = null;
	addon->work = NULL;

	// 个人 js 水平有限，Object 类研究的不深
	// 可以说，精通 Object 类的小伙伴可以自己想想咯，反正给对象挂一个属性、函数需要的东东，都在这里了
	// 相等于 const fun = () => {}, attr = 'Hello';
	napi_property_descriptor desc = {
		"start",       // 属性名称
		NULL,          // -- 没想明白
		start_thread,  // 函数体
		NULL,          // 属性 getter
		NULL,          // 属性 setter
		NULL,          // 属性描述符
		napi_default,
		addon          // (官网直译)也可以写 NULL，调用 getter 时候返回的数据
	};
	// 相当于 const obj = { fun, attr };
	napi_define_properties(env, exports, 1, &desc); // 将属性挂载到 exports 上面
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)


// 配合另一种导出模块用的
//static void addon_getting_unloaded(napi_env env, void* data, void* hint) {
//	Addon* addon = (Addon*)data;
//	assert(addon->work == NULL && "No work in progress at module unload");
//	free(addon);
//}

// 另一种导出模块的宏定义(有空再研究🙃)
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

```

- javascript `test/.js`
```js
const addon = require('../build/Release/thread_cb.node');

let second = 0;
let cb_exucted = false;

addon.start(function() {
  cb_exucted = true;
  console.log(`Asynchronous callback exectued.`);
});

const t = setInterval(() => {
  if (cb_exucted) {
    clearTimeout(t);
  }
  console.log(`After ${++second} seconds.`);
}, 1000);
```

- 运行
```bash
$ node test/thread-cb.js
After 1 seconds.
After 2 seconds.
After 3 seconds.
Asynchronous callback exectued.
After 4 seconds.
```

**OK! 木有任何问题，我们亲爱的 js 还是你熟悉的那个非阻塞模式运行 😋**

> 写在最后：
> 虽然这是教程的第三篇，但这篇是最难的了，我英语又不好看官方 N-API 文档好费劲的 😭
> 其次个人觉得吧现在地表上面的操作系统就 `windwos`、`*nix` 两款了，还都是 C 语言打造的，所以好多时候你再厉害的语言(java,php,python...)底层和 C 及 C 造的操作系统都有着千丝万缕的联系
> 所以说，虽然个人作为一个前端er，还是很推荐了解下 C 及 C 的产物操作系统，其实操作系统可以简单的理解是用 C 写的 **工具集合** 帮你操作音频、视频、网络、磁盘、合理调度 CPU、合适开辟内存等等 😂
