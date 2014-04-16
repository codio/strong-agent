if ('STRONGAGENT' in global) {
  module.exports = global.STRONGAGENT;
  return;
}

global.nodeflyConfig = require('./config');

var fs      = require('fs');
var util    = require('util');
var events  = require('events');
var os      = require('os');
var semver  = require('semver');

var Timer   = require('./timer');
var addon   = require('./addon');
var proxy   = require('./proxy');
var sender  = require('./sender');
var counts  = require('./counts');
var info    = require('./info');
var metrics = require('./metrics');
var transport = require('./transport');
var loop    = require('./loop');
var errors  = require('./errors');
var moduleDetector = require('./module-detector');

// Profilers
var cpuProf = require('./profilers/cpu');
var memProf = require('./profilers/memory');

var tiers = require('./tiers');
var loopbackTiers = require('./loopbackTiers');

var package = require('../package.json');

/**
 * Cascading config loader
 *
 * Search order:
 *   arguments
 *   process.env
 *   ./strongloop.json
 *   ./package.json
 *   ~/strongloop.json
 *
 * @param   {string} [key]      [API Key]
 * @param   {string} [appName]  [Name to identify app with in dashboard]
 * @returns {object || boolean} [Returns config data, or false if none found]
 */

function ensureConfig (key, appName) {
  var home = process.env.HOME || process.env.HOMEPATH || process.env.USERPROFILE
    , cwd = process.cwd()
    , env = process.env
    , nfjson
    , pkgjson
    , userjson;

  // Load configs from strongloop.json and package.json
  try { nfjson = require(cwd + '/strongloop.json'); } catch (e) { nfjson = {}; }
  try { pkgjson = require(cwd + '/package.json'); } catch (e) { pkgjson = {}; }
  try { userjson = require(home + '/strongloop.json'); } catch (e) { userjson = {}; }

  var config = {
    key: key ||
         env.SL_KEY ||
         env.STRONGLOOP_KEY ||
         env.NODEFLY_APPLICATION_KEY ||
         nfjson.userKey ||
         pkgjson.strongAgentKey ||
         userjson.key ||  // Bug-for-bug backwards compatibility...
         userjson.userKey,
    appName: appName ||
             env.SL_APP_NAME ||
             nfjson.appName ||
             pkgjson.name ||
             userjson.appName,
    proxy: env.STRONGLOOP_PROXY || nfjson.proxy || userjson.proxy,
    endpoint: nfjson.endpoint || userjson.endpoint,
  };

  // Only return config object if we found valid properties.
  if (config.key && config.appName) {
    return config;
  }

  return false;
}

// Constructor cannot take arguments, addon.hide() won't forward them.
var Agent = function() {
  this.debug = true;
  this.cpuinfo = require('./cpuinfo');
  events.EventEmitter.call(this);
};

if (addon) {
  // Make instances undetectable.
  Agent = addon.hide(Agent);
}

util.inherits(Agent, events.EventEmitter);

module.exports = new Agent;

Object.defineProperty(global, 'STRONGAGENT', { value: module.exports });

Agent.prototype.profile = function (key, appName, options) {
  var self = this;

  if(this.initialized) {
    console.error('strong-agent profiling has already started');
    return;
  }

  if (options == null) {
    options = {};
  }

  this.quiet = !!options.quiet;

  if (!process.hrtime) {
    this.info(
      'strong-agent not profiling, node does not support process.hrtime().');
    return;
  }

  var config = ensureConfig(key, appName);
  if (!config) {
    if (!this.quiet) {
      console.warn([
        'strong-agent not profiling, configuration not found.',
        'Generate configuration with:',
        '    npm install -g strong-cli',
        '    slc strongops',
        'See http://docs.strongloop.com/strong-agent for more information.'
      ].join('\n'));
    }
    return;
  }

  this.key = config.key;

  if (config.appName instanceof Array) {
    this.appName  = config.appName.shift();
    this.hostname = config.appName.join(':');
  } else {
    this.appName  = config.appName;
    this.hostname = os.hostname();
  }


  this.initialized = true;
  this.info('strong-agent v%s profiling app \'%s\' pid \'%d\'',
    package.version, this.appName, process.pid);
  this.info('strong-agent dashboard is at https://strongops.strongloop.com');

  proxy.init();
  sender.init();
  counts.init();
  info.init();
  metrics.init();
  tiers.init();
  loopbackTiers.init();
  loop.init();
  errors.init();

  var loopbackPath = 'loopback';

  var loopbackVersion = moduleDetector.detectModule(loopbackPath);

  this.transport = transport.init({
    agent: this,
    agentVersion: package.version,
    port: options.port || config.port,
    host: options.host || config.host,
    proxy: options.proxy || config.proxy,
    endpoint: options.endpoint || config.endpoint,
    loopbackVersion: loopbackVersion
  });
  this.transport.connect();

  this.prepareProbes();
  this.prepareProfilers();
  this.prepareClusterControls();
};

Agent.prototype.stop = function() {
  // FIXME(bnoordhuis) This should stop the timer in lib/sender.js.
  this.transport.disconnect();
};

Agent.prototype.prepareProbes = function () {
  var probes = {},
    wrapping_probes = {};
  var probe_files = fs.readdirSync(__dirname + '/probes'),
    wrapper_files = fs.readdirSync(__dirname + '/wrapping_probes');

  probe_files.forEach(function (file) {
    var m = file.match(/^(.*)+\.js$/);
    if (m && m.length == 2) probes[m[1]] = true;
  });

  wrapper_files.forEach(function (file) {
    var m = file.match(/^(.*)+\.js$/);
    if (m && m.length == 2) wrapping_probes[m[1]] = true;
  })

  // Monkey-wat?
  var original_require = module.__proto__.require;
  module.__proto__.require = function(name) {
    var args = Array.prototype.slice.call(arguments),
      target_module = original_require.apply(this, args);

    if (target_module == null) return target_module;
    if (args.length == 1 && !target_module.__required__) {
      if (wrapping_probes[name]) {
        target_module.__required__ = true;
        target_module = require('./wrapping_probes/' + name)(target_module);
      } else if (probes[name]) {
        target_module.__required__ = true;
        require('./probes/' + name)(target_module);
      }
    }

    return target_module;
  }
};

Agent.prototype.prepareProfilers = function () {
  var self = this;
  memProf.init();

  // // Allow instance profiling events to be triggered from server
  this.transport.on('memory:start', function () {
    if (memProf.enabled) {
      console.log('memory profiler has already been started');
      return;
    }
    if (memProf.start()) {
      console.log('strong-agent starting memory profiler');
      self.transport.send('profile:start', 'memory');

      self.transport.once('memory:stop', function () {
        console.log('strong-agent stopping memory profiler');
        memProf.stop();
        self.transport.send('profile:stop', 'memory');
      });
    }
  });

  // Allow cpu profiling events to be triggered from server
  this.transport.on('cpu:start', function () {
    if (cpuProf.enabled) {
      console.log('cpu profiler has already been started');
      return;
    }
    if (cpuProf.start()) {
      console.log('strong-agent starting cpu profiler');
      self.transport.send('profile:start', 'cpu');
      self.transport.once('cpu:stop', function (rowid) {
        var data = cpuProf.stop();
        console.log('strong-agent sending cpu profiler result', rowid);

        // we don't need to send profile:stop because the profileRun event
        // already updates that row to "done"
        self.transport.send('profileRun', rowid, data);
      });
    }
  });
};

// First call will have null control, and clustering configuration will be set
// to {enabled: false}. Later, the strong-cluster-control probe may call this
// with s-c-c as the argument.
Agent.prototype.prepareClusterControls = function (control) {
  var self = this;

  if (self._strongClusterControl) {
    // Ignore multiple initialization
    return;
  }

  var clusterInfo = {
    enabled: false
  };

  if (control) {
    var cluster = require('cluster');
    var version = control.VERSION;
    if (version != null && semver.gte(version, '0.2.0')) {
      self._strongClusterControl = control;

      clusterInfo.enabled = !!control._running;
      clusterInfo.isMaster = cluster.isMaster;
      clusterInfo.isWorker = cluster.isWorker;

      if (cluster.isMaster) {
        this.info('strong-agent using strong-cluster-control v%s', version);

        // Repeating loop is so we don't lose sync on disconnect, should be a
        // better way.
        Timer.repeat(5000, updateClusterStatus);

        // On any state change which effects status, update it (for faster
        // response on these infrequent events, we don't want to wait 5 seconds
        // to know a worker has died).
        cluster.on('fork', updateClusterStatus);
        cluster.on('exit', updateClusterStatus);
        cluster.on('disconnect', updateClusterStatus);
        control.on('setSize', updateClusterStatus);
        control.on('resize', updateClusterStatus);
        control.on('restart', updateClusterStatus);
        control.on('startRestart', updateClusterStatus);
        control.on('start', updateClusterStatus);
        control.on('stop', updateClusterStatus);

        self.transport.on('cluster:resize', resizeCluster);
        self.transport.on('cluster:restart-all', restartCluster);
        self.transport.on('cluster:terminate', terminateWorker);
        self.transport.on('cluster:shutdown', shutdownWorker);
      }
    }
    else {
      this.info(
        'strong-agent cannot use strong-cluster-control %s,',
        'please update to >= 0.2.0');
    }
  }

  // XXX(sam) It would be possible to display state of a cluster master (workers
  // and change in worker status) even if control wasn't running.
  sendClusterStatus();

  return;

  // XXX(sam) I think all the (!control || !clusterInfo.isMaster) guards below
  // are unnecessary (though harmless).
  function updateClusterStatus() {
    if (!control || !clusterInfo.isMaster) {
      return;
    }
    var status = control.status();
    clusterInfo.setSize = control.options.size;
    clusterInfo.size = status.workers.length;
    clusterInfo.workers = status.workers;
    clusterInfo.restarting = control._restartIds;
    clusterInfo.cpus = control.CPUS;
    clusterInfo.enabled = !!control._running;
    sendClusterStatus();
  }

  function sendClusterStatus() {
    self.transport.send('cluster:status', clusterInfo);
  }

  function resizeCluster(size) {
    if (!control || !clusterInfo.isMaster) return;
    control.setSize(size);
  }

  function restartCluster() {
    if (!control || !clusterInfo.isMaster) return;
    control.restart();
  }

  function shutdownWorker(id) {
    if (!control || !clusterInfo.isMaster) return;
    control.shutdown(id);
  }

  function terminateWorker(id) {
    if (!control || !clusterInfo.isMaster) return;
    control.terminate(id);
  }
};

Agent.prototype.metric = function (scope, name, value, unit, op, persist) {
  if(!this.initialized) return;
  metrics.add(scope, name, value, unit, op, persist);
};

Agent.prototype.info = function () {
  if (!this.quiet) {
    console.log.apply(null, arguments);
  }
};

Agent.prototype.log = function (msg) {
  if (this.debug && msg) console.log('strong-agent:', msg);
};


Agent.prototype.error = function (e) {
  if (this.debug && e) console.error('strong-agent error:', e, e.stack);
};


Agent.prototype.dump = function (obj) {
  if (this.debug) console.log(util.inspect(obj, false, 10, true));
};


Agent.prototype.message = function (msg) {
  util.log("\033[1;31mstrong-agent:\033[0m " + msg);
};
