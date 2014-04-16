function debug (format, args) {
  if (/memprof/.test(process.env.NODEFLY_DEBUG) ) {
    console.log.apply(console, ['MEMORY PROFILER: ' + format].concat(args || []));
  }
}

// Load dependencies
var Timer = require('../timer');

function Instances () {
  this.addon = require('../addon');
  this.agent = global.STRONGAGENT;
  this.enabled = false;
  this.instances = [];
  this.timer = null;

  // NOTE: Can not be prototype function. Difficult to bind and use with off()
  var self = this;
  this._step = function () {
    debug('instance monitoring step');
    var state = self.addon.stopHeapDiff(true);
    self.agent.emit('instances', { type: 'Instances', state: state });
    self.addon.startHeapDiff();
  };
}
module.exports = new Instances;

Instances.prototype.init = function () {
  this.agent = global.STRONGAGENT;
};

Instances.prototype.toggle = function () {
  this.enabled ? this.stop() : this.start();
};

Instances.prototype.start = function () {
  if (!this.addon) {
    this.agent.info('strong-agent could not load heap monitoring add-on');
    return false;
  }
  debug('instance monitoring started');
  this.instances = [];
  this.addon.startHeapDiff();
  this.timer = Timer.repeat(15 * 1000, this._step);
  this._step();
  this.enabled = true;
  return true;
};

Instances.prototype.stop = function () {
  if (!this.addon) return;
  debug('instance monitoring stopped');
  if (this.timer) {
    this.addon.stopHeapDiff(false);
    clearInterval(this.timer);
    this.timer = null;
  }
  this.enabled = false;
};
