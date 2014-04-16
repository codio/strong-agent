var os = require('os');
var Timer = require('./timer');
var addon = require('./addon');
var platform = os.platform();

var config = global.nodeflyConfig;

var osScope = os.hostname();
var processScope = osScope + ' - Process[' + process.pid + ']';

var agent;
var lastCpuTime;
var last_cpu_util;
var gcstats = [];

exports.init = function() {
  agent = global.STRONGAGENT;

  if (!addon) {
    agent.info('strong-agent could not load optional native add-on');
    return;
  }

  addon.onGC(function(stats){

    gcstats.push(stats);

    var len   = gcstats.length;
    var total = 0;

    gcstats.forEach(function(stat){
      total += stat;
    })

    var baseline = total / len / 1000000;

    agent.metric(processScope, 'GC Full. V8 heap used', baseline, 'MB');
    collectHeap(baseline);

    if (len>10) {
      gcstats = gcstats.splice(1,len);
    }

  });

  Timer.repeat(config.collectInterval, function() {
    try {
      collect();
      connectionInfo();
      collectHeap();
    }
    catch(e) { agent.error(e); }
  });

  osScope = os.hostname();
  processScope = osScope + ' - Process[' + process.pid + ']';

  collect();
};

function collectHeap(gcFull) {
  try {
    var mem = process.memoryUsage();
    var rss = mem.rss / 1000000;
    var heapUsed = mem.heapUsed / 1000000;
    var heapData = [ heapUsed, rss, gcFull];

    agent.metric(processScope, 'Heap Data', heapData, 'MB');
  }
  catch(err) {
    agent.error(err);
  }
}

function connectionInfo(){
  if (agent.server_obj) {
    var tp = agent.server_obj.connCount / (config.collectInterval / 1000);
    agent.server_obj.connCount = 0;

    if (agent.server_obj.getConnections) {
      agent.server_obj.getConnections(function(err, conns) {
        if (err === null) {
          agent.metric(processScope, 'Connections', [conns,tp], '');
        }
      });
    }
    else {
      var conns = agent.server_obj.connections || agent.server_obj._connections || null;
      agent.metric(processScope, 'Connections', [conns,tp], '');
    }

  }
}


var collect = function() {
  require('./cpuinfo').cpuutil(function(percent_proc,percent_user,percent_syst){
    agent.metric(processScope, 'CPU util',       percent_proc, '%');
    agent.metric(processScope, 'CPU util stime', percent_syst, '%');
    agent.metric(processScope, 'CPU util utime', percent_user, '%');
  });
};
