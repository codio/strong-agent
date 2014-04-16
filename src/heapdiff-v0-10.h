// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_HEAPDIFF_V0_10_H_
#define AGENT_SRC_HEAPDIFF_V0_10_H_

#include "heapdiff.h"
#include "heapdiff-inl.h"
#include "strong-agent.h"
#include "v8-profiler.h"

namespace strongloop {
namespace agent {
namespace heapdiff {

using v8::Arguments;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::HeapProfiler;
using v8::HeapSnapshot;
using v8::Isolate;
using v8::Object;
using v8::String;
using v8::Undefined;
using v8::Value;

const HeapSnapshot* start_snapshot;

Handle<Value> StartHeapDiff(const Arguments&) {
  HandleScope handle_scope;
  if (start_snapshot == NULL) {
    start_snapshot = HeapProfiler::TakeSnapshot(String::Empty());
  }
  return Undefined();
}

Handle<Value> StopHeapDiff(const Arguments& args) {
  HandleScope handle_scope;

  if (start_snapshot == NULL) {
    return Undefined();
  }

  Handle<Value> result = Undefined();
  if (args[0]->IsTrue()) {
    const HeapSnapshot* end_snapshot =
        HeapProfiler::TakeSnapshot(String::Empty());
    result = Summarize(NULL, start_snapshot, end_snapshot);
  }

  HeapProfiler::DeleteAllSnapshots();
  start_snapshot = NULL;
  return handle_scope.Close(result);
}

void Initialize(Isolate* isolate, Handle<Object> binding) {
  binding->Set(FixedString(isolate, "startHeapDiff"),
               FunctionTemplate::New(StartHeapDiff)->GetFunction());
  binding->Set(FixedString(isolate, "stopHeapDiff"),
               FunctionTemplate::New(StopHeapDiff)->GetFunction());
}

}  // namespace heapdiff
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_HEAPDIFF_V0_10_H_
