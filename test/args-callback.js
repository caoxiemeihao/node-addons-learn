const addon = require('../build/Release/args_callback.node');

setTimeout(() => console.log('Timeout is executed.'), 100);

const arg = { name: 'anan', age: 29 };
const cb = function (name, age) {
  console.log(`The name is ${name}`, `\nAge is ${age}`);
  console.log(this === global); // true
};

addon(arg, cb); // 如果导出的是 function
// addon.call(arg, cb); // 如果导出的是 exports 对象，并且 call 挂载在 exports 上
