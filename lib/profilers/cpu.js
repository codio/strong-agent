'use strict';

var addon = require('../addon');

exports.start = function() {
  if (addon) {
    addon.startCpuProfiling();
    exports.enabled = true;
    return true;
  } else {
    console.log('[strong-agent] Could not load optional native add-on.');
    return false;
  }
};

exports.stop = function() {
  exports.enabled = false;
  return addon && addon.stopCpuProfiling();
};
