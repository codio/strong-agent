// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_HEAPDIFF_V0_12_H_
#define AGENT_SRC_HEAPDIFF_V0_12_H_

#include "heapdiff.h"
#include "heapdiff-inl.h"
#include "strong-agent.h"
#include "v8-profiler.h"

namespace strongloop {
namespace agent {
namespace heapdiff {

using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::HeapSnapshot;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

const HeapSnapshot* start_snapshot;

void StartHeapDiff(const FunctionCallbackInfo<Value>& args) {
  if (start_snapshot == NULL) {
    Isolate* isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
    start_snapshot =
        isolate->GetHeapProfiler()->TakeHeapSnapshot(String::Empty(isolate));
  }
}

void StopHeapDiff(const FunctionCallbackInfo<Value>& args) {
  if (start_snapshot == NULL) {
    return;
  }

  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);

  if (args[0]->IsTrue()) {
    const HeapSnapshot* end_snapshot =
        isolate->GetHeapProfiler()->TakeHeapSnapshot(String::Empty(isolate));
    Local<Object> result = Summarize(isolate, start_snapshot, end_snapshot);
    args.GetReturnValue().Set(result);
  }

  isolate->GetHeapProfiler()->DeleteAllHeapSnapshots();
  start_snapshot = NULL;
}

void Initialize(Isolate* isolate, Handle<Object> binding) {
  binding->Set(FixedString(isolate, "startHeapDiff"),
               FunctionTemplate::New(isolate, StartHeapDiff)->GetFunction());
  binding->Set(FixedString(isolate, "stopHeapDiff"),
               FunctionTemplate::New(isolate, StopHeapDiff)->GetFunction());
}

}  // namespace heapdiff
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_HEAPDIFF_V0_12_H_
