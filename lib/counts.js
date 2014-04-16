var agent;

var Timer = require('./timer');
var stats = require('measured').createCollection('callCounts');

exports.interval = 60e3;

exports.init = function() {
  agent = global.STRONGAGENT;
  start();
};

exports.sample = function(code) {
  var meter = stats.meter(code);
  meter.mark();
  meter.unref();
};

function start() {
  Timer.repeat(exports.interval, function () {
    var data = stats.toJSON();
    stats.constructor();  // Reset counter.
    agent.emit('callCounts', data);
  });
}
