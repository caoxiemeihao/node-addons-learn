> 1、注：此教程默认你具备一定的 C/C++ 基础语法知识 
> 2、如：指针、结构体等 
> 3、本教程以 windows 操作系统为例，假设你会简单的使用 Visual Studio

### 系列文章

1. [C/C++ Addons 入门 Hello world!](https://www.jianshu.com/p/6b0d60672e04)
2. [C/C++ Addons 对象参数及回调函数](https://www.jianshu.com/p/210ab7c53732)
3. C/C++ Addons 非阻塞多线程回调
4. C/C++ Addons windows 下 .dll 动态链接库 **实用篇**

> [完整代码](https://github.com/caoxiemeihao/node-addons-learn)

### 前置准备

##### 工具
- `node-gyp`
  * Google 出品的跨平台构建工具，初衷是用来打包 chromium 的
  * `gyp` 即 `generate your package`，将你的 `C/++` 代码编译成 `node.js` 可识别的文件
  * 类似 `webpack` 将 `vue、jsx` 等方言编译成为浏览器可识别文件
  * 也可以用 cMake.js 做同样的事情
- `python`
  * 因为 `node-gyp` 是用 `python` 写的
  * 不止一个博客中说只能用 `python2.7` - **骗人的**🤬
  * 官方的说明 [Python v2.7, v3.5, v3.6, v3.7, or v3.8](https://github.com/nodejs/node-gyp#readme)
- `Visual Studio2017`
  * C/C++ 在 `windows` 下依赖 VS - **个人推荐**
  * 官方给出了木有 VS 的方案，[windows-build-tools](https://github.com/felixrieseberg/windows-build-tools) 但这没法用最重要语法提示、报错功能 - **不推荐**

##### 姿势

> 目前一共有三种方式可以编写 node.js 扩展，本文以官方推荐的 `N-API` 为例 

- [N-API](https://nodejs.org/dist/latest-v12.x/docs/api/n-api.html) 
  * node.js 由官方维护的 `node.js` 扩展 api
  * 纯 C 语法不依赖 `node.js` 版本，`node.js` 更新后基于 `N-API` 写的插件照样用，官方的解释是底层调用的 `node.js` 稳定版的二进制接口
- [node-addon-api](https://github.com/nodejs/node-addon-api)
  - `N-API` 的 C++ 包装版本(有对象，更美好😝)，目前 (Release 2.0.0) 并未完全的包装 `N-API` 的所有 api
  * [官方 demo](https://github.com/nodejs/node/tree/master/test/addons)
- [nan](https://github.com/nodejs/nan)
  * `N-API` 没出来之前主要的插件开发方式
  * “虽然”依赖 `node.js` 版本，但是维护团队很卖力，帮忙做好了每个版本的编译所以就 **不依赖** `node.js` 版本了 👍
- [原生 C/C++](https://nodejs.org/dist/latest-v12.x/docs/api/addons.html)
  - 极度复杂，需要用一些 `v8` api、源码
  - 依赖 `node.js` 版本，所以很难用 👎


### 起步

1. 安装依赖
```bash
yarn add -D node-gyp # 就这一个依赖就够了
```
- 个人很喜欢安装到项目里面，而不是 `yarn add -g node-gyp`
- package.js 配置 `scripts`
```json
{
  "scripts": {
    "configure": "node-gyp configure",
    "build": "node-gyp build",
    "clean": "node-gyp clean",
    "rebuild": "node-gyp rebuild"
  }
}
```
- `configure` 会根据 `binding.gyp` 在 `build` 文件夹下生成当前平台的 C/C++ 工程 - **第一步执行这个**
![image](https://upload-images.jianshu.io/upload_images/6263326-43f82ab34ea13439.jpg?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
> ps: 下面的命令干的活都交给 VS 咯，不要去折腾命令咯(除非你没有VS)
- `build`(*可选*) 如果你不想用 VS，只是编译已有的 C/C++ 程序，那么这条命令可以代替 VS 帮你构建 - **需要 windows-build-tools**
- `clean`(*可选*) 把 `build` 目录清除
- `rebuild`(*可选*) 依次执行 `clean、configure、build` 三条命令

2. 新建 binding.gyp
```gyp
{
  "targets": [
    {
      "target_name": "hello",
      "sources": [ "src/hello.c" ],
    }
  ]
}
```
- 构建配置文件，语法同 js 版本的 json。等价于 webpack.config.js
- `targets` 下面的每一项都可以理解为一个 `node插件`，等价于 `webpack` 打包 `bundle.js`
- `target_name` 即 `require([target_name])`
- `sources` C/C++ 源码目录
- [更多配置参考](https://www.cnblogs.com/x_wukong/p/4829598.html)

3. 生成目标平台项目
```bash
$ yarn configure
```

4. 启动 `Visual Studio`
![image](https://upload-images.jianshu.io/upload_images/6263326-a0129a8fed22c356.jpg?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
![image](https://upload-images.jianshu.io/upload_images/6263326-0ab4b4a7ec3191d5.jpg?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

### 编写扩展

- 一些 API 说明
```
  - `napi_status` 枚举
    * 调用任意 N-API 后返回值类型
  - `napi_extended_error_info` 结构体
    * 表示调用 N-API 后发生的错误对象
  - `napi_env` 结构体
    * 告诉 N-API 当前执行上下文，在调用 Addons 时自动(Init)传入
    * 调用任意多个、或嵌套 N-API 时候需要一直传递下去，不允许重用
  - `napi_callback_info` 
    * 用于 Addons 获取 js 调用时候传入的上下文信息，如参数
  - `napi_value` 不透明指针
    * N-API 提供的在 C、js 中间的一种数据类型
    * 任意的 js 数据类型都可以赋值给 napi_value，然后通过 N-API 提供的方法再把 napi_value 转成 C 语言的类型，反之亦然
  - `napi_threadsafe_function` 不透明指针
    * 代表一个 js 的 function，在多线程模式下通过 `napi_call_threadsafe_function` 调用实现异步 😁

  # 函数
  - `napi_create_string_utf8`
    * 创建 napi 类型的 string
    * 相当于 const str = 'Hello world!'
  - `napi_get_property`
    * 从 napi 类型的对象中取值
    * 相当于对 json = { name: 'anan', age: 29 } 取值: console.log(json.name, json.age)
  - `napi_get_cb_info`
    * 用于获取 js 的回调函数
  - `napi_call_function`
    * Addons 调用 js 回调
  - `napi_create_function`
    * 创建 js 函数
  - `napi_get_global`
    * 在 Addons 中获取 js 的 global 对象

```

- C/C++ `src/hello.c`

```c
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

```

> ps: 编写好C代码后，Ctrl+Shift+b (VS编译快捷键)

- javascript `index.js`
```js
// const addon = require('./build/Debug/hello.node'); // 如果 VS 编译模式是 Debug
const addon = require('./build/Release/hello.node'); // 如果 VS 编译模式是 Release

console.log(addon.hello('world!'));
```

- 运行
```bash
$ node index.js
Hello world!
```
`Boom Shakalaka`