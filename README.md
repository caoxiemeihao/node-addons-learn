#### [完整文章在简书 https://www.jianshu.com/u/ad3b2e000ea2](https://www.jianshu.com/u/ad3b2e000ea2)

---

## Addons 实例
- Hello world
  * src/hello.c
  * test/hello.js
- 对象参数及回调函数
  * src/args-callback.c
  * test/args-callback.js
- 非阻塞多线程回调
  * src/thread-fs.c
  * test/thread-fs.js
- windows 下 .dll 动态链接库调用
  * src/call-dll.c
  * test/call-cll.js

## 用到的 API 介绍

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


### 函数
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
