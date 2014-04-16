var Timer = require('./timer');
var measured = require('measured');

module.exports = function (tiersName) {
  var singleton;

  function BaseTiers () {
    this.stats = measured.createCollection(tiersName);
    this.config = global.nodeflyConfig;
    this.agent = global.STRONGAGENT;
    this.start();
  }

  // Entry point
  BaseTiers.init = function () {
    singleton = new BaseTiers;
    return singleton;
  };

  // Compatibility hack
  BaseTiers.sample = function (code, time) {
    singleton && singleton.sample(code, time);
  };

  // Put real sampler on contructed object
  BaseTiers.prototype.sample = function (code, time) {
    this.stats.histogram(code).update(time.ms);
  };

  // Expose this on the Tiers constructor, so we can stub it out
  BaseTiers.prototype.start = function () {
    var self = this;
    Timer.repeat(self.config.tiersInterval, function () {
      var data = self.stats.toJSON();
      self.agent.emit(tiersName, data);
      Object.keys(data[tiersName]).forEach(function (key) {
        self.stats.histogram(key).reset();
      });
    });
  };

  return BaseTiers;
};
