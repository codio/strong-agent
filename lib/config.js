var env = process.env.SL_ENV || 'prod';

var http = {
  host: process.env.STRONGLOOP_COLLECTOR,
  port: process.env.STRONGLOOP_COLLECTOR_PORT,
};

var https = {
  host: process.env.STRONGLOOP_COLLECTOR_HTTPS,
  port: process.env.STRONGLOOP_COLLECTOR_HTTPS_PORT,
};

var cfg = {
  prod: {
    collectInterval: 60 * 1000,
    metricsInterval: 60 * 1000,
    tiersInterval: 60 * 1000,
    loopInterval: 60 * 1000,
    collector: {
      http: {
        host: http.host || 'collector.strongloop.com',
        port: http.port || 80,
      },
      https: {
        host: https.host || 'collector.strongloop.com',
        port: https.port || 443,
      },
    }
  },
  staging: {
    collectInterval: 60 * 1000,
    metricsInterval: 60 * 1000,
    tiersInterval: 60 * 1000,
    loopInterval: 60 * 1000,
    collector: {
      http: {
        host: http.host || 'collector-staging.strongloop.com',
        port: http.port || 80,
      },
      https: {
        host: https.host || 'collector-staging.strongloop.com',
        port: https.port || 443,
      },
    }
  },
  dev: {
    collectInterval: 15 * 1000,
    metricsInterval: 15 * 1000,
    tiersInterval: 15 * 1000,
    loopInterval: 15 * 1000,
    collector: {
      http: {
        host: http.host || '127.0.0.1',
        port: http.port || 8080,
      },
      https: {
        host: https.host || '127.0.0.1',
        port: https.port || 8443,
      },
    }
  },
  test: {
    collectInterval: 1 * 1000,
    metricsInterval: 1 * 1000,
    tiersInterval: 1 * 1000,
    loopInterval: 1 * 1000,
    collector: {
      http: {
        host: http.host || '127.0.0.1',
        port: http.port || 8080,
      },
      https: {
        host: https.host || '127.0.0.1',
        port: https.port || 8443,
      },
    }
  },
};

module.exports = cfg[env] || cfg.prod;
