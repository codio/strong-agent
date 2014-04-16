// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#include "strong-agent.h"

#if SL_NODE_VERSION == 10
# include "extras-v0-10.h"
# include "gcinfo-v0-10.h"
# include "heapdiff-v0-10.h"
# include "profiler-v0-10.h"
# include "uvmon-v0-10.h"
#elif SL_NODE_VERSION == 12
# include "extras-v0-12.h"
# include "gcinfo-v0-12.h"
# include "heapdiff-v0-12.h"
# include "profiler-v0-12.h"
# include "uvmon-v0-12.h"
#endif

namespace strongloop {
namespace agent {

void Initialize(v8::Handle<v8::Object> binding) {
#if SL_NODE_VERSION == 10
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
#elif SL_NODE_VERSION == 12
  v8::Isolate* isolate = binding->CreationContext()->GetIsolate();
#endif
  extras::Initialize(isolate, binding);
  gcinfo::Initialize(isolate, binding);
  heapdiff::Initialize(isolate, binding);
  profiler::Initialize(isolate, binding);
  uvmon::Initialize(isolate, binding);
}

// See https://github.com/joyent/node/pull/7240.  Need to make the module
// definition externally visible when compiling with -fvisibility=hidden.
// Doesn't apply to v0.11, it uses a constructor to register the module.
#if defined(__GNUC__) && SL_NODE_VERSION == 10
extern "C" __attribute__((visibility("default")))
node::node_module_struct strong_agent_module;
#endif

NODE_MODULE(strong_agent, Initialize)

}  // namespace agent
}  // namespace strongloop
