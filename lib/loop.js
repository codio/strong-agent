function debug (format, args) {
  if (/uvmon/.test(process.env.NODEFLY_DEBUG) ) {
    console.log.apply(console, ['UVMON: ' + format].concat(args || []));
  }
}

var agent;
var Timer = require('./timer');
var addon = require('./addon');
var proxy = require('./proxy');

var config = global.nodeflyConfig;

exports.init = function() {
  agent = global.STRONGAGENT;
  if (!addon) {
    agent.info('strong-agent could not load optional native add-on');
    return;
  }
  start();
}

function nodeflyNoOp() {}

function checkNextTick(obj, args) {
  proxy.callback(args, -1, nodeflyNoOp);
}

function checkTimers(obj, args){
  // callback for any setTimeout or setInterval
  proxy.callback(args, -1, nodeflyNoOp);
}


function start() {
  debug('starting uvmon');

  var statistics = addon.eventLoopStatistics;
  Timer.repeat(config.loopInterval, function() {
    var slowest = statistics[0];
    var sum = statistics[1];
    var ticks = statistics[2];

    statistics[0] = 0;
    statistics[1] = 0;
    statistics[2] = 0;

    agent.emit('loop', {
      // XXX(bnoordhuis) Backwards compatible field names.
      loop: { count: ticks, slowest_ms: slowest, sum_ms: sum },
    });

    // we're also going to shoehorn it into the metric data to make our life easier
    agent.metric(null, 'queue', [slowest, sum / ticks]);

    if (process.env.NODEFLY_DEBUG && /uvmon/.test(process.env.NODEFLY_DEBUG)) {
      console.error('UVMON: %s', JSON.stringify({
        count: ticks, slowest_ms: slowest, sum_ms: sum
      }));
    }
  });

  proxy.before(process, [ 'nextTick' ], checkNextTick);
  proxy.before(global, [ 'setTimeout', 'setInterval' ], checkTimers);

}
