function trace (format, args) {
  if (/cpu_verbose/.test(process.env.NODEFLY_DEBUG) ) {
    console.log.apply(console, ['CPU_VERBOSE: ' + format].concat(args || []));
  }
}

function debug (format, args) {
  if (/cpu/.test(process.env.NODEFLY_DEBUG) ) {
    console.log.apply(console, ['CPU: ' + format].concat(args || []));
  }
}


var proc = require('./proc');
var platform = require('os').platform();
var _ = require('underscore');
var util = require('util');

var last = {
  valid : false,
  all : 0,
  ptime : 0,
  utime : 0,
  stime : 0,
  uptime : 0,
  active : 0
};


function reportMetrics(proc, user, syst, onMetric) {
  // There's a weird bug, on OS X if not elsewhere, either from floating-point
  // calculations, or clock drift, or something that's leading to negative CPU
  // readings in some cases; -2.960594732333751e-15 and so on.
  // Pretty sure this only affects the system time, but we're going to round
  // them all because having a negative CPU % doesn't make sense.

  debug('reportMetrics  -  proc: %s, user: %s, syst: %s', [proc, user, syst]);

  if ((proc < 0) || !isFinite(proc)) { proc = 0; }
  if ((user < 0) || !isFinite(user)) { user = 0; }
  if ((syst < 0) || !isFinite(syst)) { syst = 0; }

  if (onMetric) {
    debug('actually reported  -  proc: %s, user: %s, syst: %s', [proc, user, syst]);
    onMetric(proc, user, syst);
  }
}


function calculateMetrics(utime, stime, ptime, all, onMetric) {
  debug('calculateMetrics  -  utime: %s, ptime: %s, stime: %s, all: %s', [utime, ptime, stime, all]);
  if (last.valid) {
    var tickDelta  = (all - last.all);

    if (tickDelta == 0) {
      // no ticks have elapsed, so these readings are all invalid; just return
      return;
    }

    var proc = (ptime - last.ptime) / tickDelta * 100;
    var user = (utime - last.utime) / tickDelta * 100;
    var syst = (stime - last.stime) / tickDelta * 100;

    reportMetrics(proc, user, syst, onMetric);
  }
  else {
    debug('last not valid, not reporting');
  }

  last.valid = true;
  last.all = all;
  last.ptime = ptime;
  last.utime = utime;
  last.stime = stime;
}


// used for OS X and FreeBSD
function parsePs(fields, pid, onMetric) {
  var ps = require('child_process').spawn('/bin/ps', ['-p', pid, '-o',fields]);
  if (ps.unref) {
    ps.unref();
  }
  var res = '';
  ps.stdout.on('data',function(data){
    res += data;
  });
  ps.on('close',function(){
    var m = res.match(/ELAPSED\s*(\d*):(\d*\.\d*)\s*(\d*):(\d*\.\d*)\s*(?:(\d*)-)?(?:(\d*):)?(\d*):(\d*)/);
    trace('ps is:\n%s', [res]);

    if (m) {
      m.shift(); // toss the full match

      var keys = [
        'uMinutes',
        'uSeconds',
        'pMinutes',
        'pSeconds',
        'rDays',
        'rHours',
        'rMinutes',
        'rSeconds'
      ];

      var data = _.reduce(m, function(memo, val) {
        memo[keys.shift()] = parseFloat(val) || 0;
        return memo;
      }, {});

      var utime = data.uMinutes * 60 + data.uSeconds;
      var ptime = data.pMinutes * 60 + data.pSeconds;
      var stime = ptime - utime;

      var all =
        ((data.rDays * 24 + data.rHours)
          * 60 + data.rMinutes)
          * 60 + data.rSeconds;

      calculateMetrics(utime, stime, ptime, all, onMetric);
    }
    else {
      debug('unparsable ps data');
    }
  });
}


exports.cpuutil = function (onMetric)
{
  var pid = process.pid;

  if (platform === 'linux') {
    debug('platform is linux');

    proc.stat(pid, function(err, stat) {
      if (err) {
        console.err('ERROR: could not run proc.stat for this pid\n', err);
        return;
      }
      trace('stat is\n', util.inspect(stat));
      calculateMetrics(stat.utime, stat.stime, stat.ptime, stat.all, onMetric);
    });

  }
  else if (platform === 'sunos' || platform === 'solaris') {
    debug('platform is sunos/solaris');

    proc.usage(pid, function(err, usage) {
      if (err) return;
      var utime = usage.utime;
      var stime = usage.stime;
      var ptime = utime + stime;
      var all = usage.rtime;

      calculateMetrics(utime, stime, ptime, all, onMetric);
    });

  }
  else if (platform === 'darwin') {
    debug('platform is darwin');

    parsePs('utime,time,etime', pid, onMetric);

  }
  else if (platform === 'freebsd') {
    debug('platform is freebsd');

    parsePs('usertime,time,etime', pid, onMetric);

  }
  else if (platform === 'win32') {
    debug('platform is win32');

    if (last.uptime) {
      var ps = require('child_process').exec('tasklist /v',
        function(err, stdout, stderr) {
        if (! err) {
          stdout.split('\n').forEach(function(item) {
            var items = item.split(/\s+/);
            var pid = parseInt(items[1]);
            if (pid === process.pid){
              var times = items[8].split(/[:.]/);

              var hour = parseInt(times[0]);
              var mins = parseInt(times[1]);
              var secs = parseInt(times[2]);

              // Total CPU Time of Process in Seconds
              var active = hour * 3600 + mins * 60 + secs;

              // Total Uptime of Process in Seconds
              var uptime = process.uptime();

              var uptimeDelta = uptime - last.uptime;
              var activeDelta = active - last.active;

              last.active = active;
              last.uptime = uptime;

              var usage = activeDelta / uptimeDelta;

              if (uptimeDelta > 0) {
                if ( usage > 1 ) {
                  // Spike Alert
                }
                reportMetrics(usage * 100, usage * 100, 0, onMetric);
              }
            }
          })
        }
      });
    } else {
      last.uptime = process.uptime();
    }

  } // if win32
}
