# node.js C/C++ 扩展入门

> 注：此教程默认你具备一定的 C/C++ 基础语法知识
> 如：指针、结构体等
> 本教程以 windows 操作系统为例，假设你会简单的使用 Visual Studio

### 前置准备

##### 工具
- `node-gyp`
  * Google 出品的跨平台构建工具，初衷是用来打包 chromium 的
  * `gyp` 即 `generate your package`，将你的 `C/++` 代码编译成 `node.js` 可识别的文件
  * 和 `webpack` 将 `vue、jsx` 等方言编译成成为浏览器可识别的文件功能类似
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
![](https://raw.githubusercontent.com/caoxiemeihao/node-addons-hello/master/screenshot/build-folder.jpg)
![](https://raw.githubusercontent.com/caoxiemeihao/node-addons-hello/master/screenshot/vs-launch.jpg)
![](https://raw.githubusercontent.com/caoxiemeihao/node-addons-hello/master/screenshot/vs-struct.jpg)
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

### 编写扩展

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

## 未完待续
- [完整代码](https://github.com/caoxiemeihao/node-addons-hello)
- [顶尖电子称 AclasSDK 对接]
