var agent;

// Skip the express wrap, if we don't have domain support
var domain;
try { domain = require('domain'); }
catch (e) {
  exports.init = function () {
    global.STRONGAGENT.info('Error reporting is not available in this version of node');
  };
  exports.wrapExpress = function () {};
  return;
}

exports.init = function () {
  agent = global.STRONGAGENT;
  var d = domain.create();
  d.once('error', function(err) {
    d.exit();

    // Just rethrow the exception when node is exiting.  Trying to report it
    // to the collector now won't work because the event loop is dead.
    if (process._exiting) {
      return rethrow();
    }

    agent.emit('reportError', {
      ts: Date.now()
      , type: 'top-level'
      , stack: err.stack
    }, rethrow);

    // Make sure the error gets thrown even if the connection times out.
    // It's bad if the collector misses out on the error event but it's
    // infinitely worse to keep the application hanging in limbo.
    setTimeout(rethrow, 250);

    function rethrow() {
      if (rethrow.thrown) return;
      rethrow.thrown = true;
      // Re-emit the error on the domain object because throwing it will make
      // v8::Message::GetScriptResourceName() point to this file.  The current
      // approach is only marginally less broken because the origin will now
      // point to events.js in node.js core instead of the actual throw site.
      // We might be able to improve on the current situation by (ab)using V8's
      // debug facilities for exceptions but that will require a significant
      // amount of C++ code and may have a non-trivial performance impact.
      d.emit('error', err);
    }
  });
  d.enter();
};

exports.wrapExpress = function (app) {
  if (!app._router) {
    // express 4.x has no app._router, so we won't catch middleware errors
    return;
  }

  var oldRoute = app._router.route;
  var first = true;

  // Hack to ensure the error handler middleware is always the last in the stack
  app.use(function (req, res, next) {
    if (first) {
      first = false;
      app.use(function (err, req, res, next) {
        if (err.tracked) return next(err);
        agent.emit('reportError', {
          ts: Date.now()
          , type: 'express'
          , command: req.command
          , stack: err.stack
        }, function () {
          err.tracked = true;
          next(err);
        });
      });
    }

    next();
  });

  app._router.route = function (method, path, callback) {
    // Wrap the next thing in a domain
    oldRoute.call(this, method, path, function (req, res, next) {
      req.command = req.method + ' ' + path;
      var d = req.domain = domain.create();
      d.add(req);
      d.add(res);
      d.on('error', next);
      d.run(next);
    });

    // Do actual route call
    oldRoute.apply(this, arguments);
  }
};
