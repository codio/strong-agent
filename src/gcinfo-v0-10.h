// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_GCINFO_V0_10_H_
#define AGENT_SRC_GCINFO_V0_10_H_

#include "gcinfo-baton.h"
#include "gcinfo-baton-inl.h"
#include "queue.h"
#include "strong-agent.h"

namespace strongloop {
namespace agent {
namespace gcinfo {

using v8::Arguments;
using v8::Context;
using v8::Function;
using v8::FunctionTemplate;
using v8::GCCallbackFlags;
using v8::GCType;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Undefined;
using v8::V8;
using v8::Value;

Persistent<Function> on_gc_callback;
uv_idle_t idle_handle;

BATON_QUEUE_STATIC_INITIALIZER();

void OnIdle(uv_idle_t*, int) {
  HandleScope handle_scope;
  Local<Object> global_object;
  uv_idle_stop(&idle_handle);
  while (Baton* baton = Baton::Pop()) {
    if (on_gc_callback.IsEmpty() == false) {
      Local<Value> argv[] = {
        Integer::NewFromUnsigned(baton->heap_statistics()->used_heap_size()),
        String::New(baton->type_string()),
        String::New(baton->flags_string()),
        Integer::New(baton->type()),
        Integer::New(baton->flags())
      };
      if (global_object.IsEmpty() == true) {
        global_object = Context::GetCurrent()->Global();
      }
      on_gc_callback->Call(global_object, SL_ARRAY_SIZE(argv), argv);
    }
    baton->Dispose();
  }
}

void AfterGC(GCType type, GCCallbackFlags flags) {
  Baton::New(Isolate::GetCurrent(), type, flags);
  uv_idle_start(&idle_handle, OnIdle);
}

Handle<Value> OnGC(const Arguments& args) {
  HandleScope handle_scope;
  if (on_gc_callback.IsEmpty() == false) {
    on_gc_callback.Dispose();
    on_gc_callback.Clear();
    V8::RemoveGCEpilogueCallback(AfterGC);
  }
  if (args[0]->IsFunction() == true) {
    on_gc_callback = Persistent<Function>::New(args[0].As<Function>());
    V8::AddGCEpilogueCallback(AfterGC);
  }
  return Undefined();
}

void Initialize(Isolate* isolate, Handle<Object> target) {
  uv_idle_init(uv_default_loop(), &idle_handle);
  uv_unref(reinterpret_cast<uv_handle_t*>(&idle_handle));
  target->Set(FixedString(isolate, "onGC"),
              FunctionTemplate::New(OnGC)->GetFunction());
}

}  // namespace gcinfo
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_GCINFO_V0_10_H_
