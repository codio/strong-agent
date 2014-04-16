var agent;
var Timer = require('./timer');
var topFunctions = require('./topFunctions');

var infoBuffer;
var metricsBuffer = [];
var tiersBuffer = [];
var loopBuffer = [];
var loopbackTiersBuffers = [];

exports.init = function() {
  agent = global.STRONGAGENT;

  agent.on('info', function(info) {
    infoBuffer = info;
  });

  agent.on('metric', function(metric) {
    metricsBuffer.push(metric);
  });

  agent.on('tiers', function(stats) {
    tiersBuffer.push(stats);
  });

  agent.on('loopback_tiers', function (stats) {
    tiersBuffer.push(stats);
  });

  agent.on('callCounts', function (counts) {
    agent.transport.update(counts);
  });

  agent.on('loop', function(loop) {
    loopBuffer.push(loop);
  });

  agent.on('instances', function (stats) {
    agent.transport.instances(stats);
  });

  topFunctions.on('update', function(update) {
    agent.transport.topCalls({ appHash: agent.appHash, update: update });
  });

  agent.on('reportError', function (error, callback) {
    agent.transport.reportError(error, callback);
  });

  Timer.repeat(1000, function() {
    if (agent.transport.disconnected()) {
      return;
    }
    try {
      sendInfo();
      sendMetrics();
      sendTiers();
      sendLoopbackTiers();
      sendLoop();
    }
    catch(e) {
      agent.error(e);
    }
  });
};


var sendInfo = function() {
  if (!infoBuffer) {
    return;
  }

  agent.transport.update(infoBuffer);
  infoBuffer = undefined;
};


var sendMetrics = function() {
  if (metricsBuffer.length == 0) {
    return;
  }

  metricsBuffer.forEach(function(metric) {
    agent.transport.update(metric);
  });

  metricsBuffer = [];
};


var sendTiers = function() {
  if (tiersBuffer.length == 0) {
    return;
  }

  tiersBuffer.forEach(function(stats) {
    agent.transport.update(stats);
  });

  tiersBuffer = [];
};


var sendLoopbackTiers = function () {
  if (loopbackTiersBuffers.length === 0) {
    return;
  }

  loopbackTiersBuffers.forEach(function (stats) {
    agent.transport.update(stats);
  });

  loopbackTiersBuffers = [];
};


var sendLoop = function() {
    if (loopBuffer.length == 0) {
      return;
    }

    loopBuffer.forEach(function(loop) {
      agent.transport.update(loop);
    });

    loopBuffer = [];
};
