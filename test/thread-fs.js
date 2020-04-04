const addon = require('../build/Release/thread_fs.node');

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
