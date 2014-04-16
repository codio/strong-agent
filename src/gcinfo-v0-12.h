// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_GCINFO_V0_12_H_
#define AGENT_SRC_GCINFO_V0_12_H_

#include "gcinfo-baton.h"
#include "gcinfo-baton-inl.h"
#include "queue.h"
#include "strong-agent.h"

namespace strongloop {
namespace agent {
namespace gcinfo {

using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
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
using v8::Value;

Persistent<Function> on_gc_callback;
uv_idle_t idle_handle;

BATON_QUEUE_STATIC_INITIALIZER();

void OnIdle(uv_idle_t*, int) {
  Isolate* isolate = Isolate::GetCurrent();  // FIXME(bnoordhuis)
  HandleScope handle_scope(isolate);
  Local<Object> global_object;
  uv_idle_stop(&idle_handle);
  while (Baton* baton = Baton::Pop()) {
    if (on_gc_callback.IsEmpty() == false) {
      const uint8_t* type_string =
          reinterpret_cast<const uint8_t*>(baton->type_string());
      const uint8_t* flags_string =
          reinterpret_cast<const uint8_t*>(baton->flags_string());
      Local<Value> argv[] = {
        Integer::NewFromUnsigned(isolate,
                                 baton->heap_statistics()->used_heap_size()),
        String::NewFromOneByte(isolate, type_string),
        String::NewFromOneByte(isolate, flags_string),
        Integer::New(isolate, baton->type()),
        Integer::New(isolate, baton->flags())
      };
      if (global_object.IsEmpty() == true) {
        global_object = isolate->GetCurrentContext()->Global();
      }
      Local<Function>::New(isolate, on_gc_callback)->Call(global_object,
                                                          SL_ARRAY_SIZE(argv),
                                                          argv);
    }
    baton->Dispose();
  }
}

void AfterGC(Isolate* isolate, GCType type, GCCallbackFlags flags) {
  Baton::New(isolate, type, flags);
  uv_idle_start(&idle_handle, OnIdle);
}

void OnGC(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  if (on_gc_callback.IsEmpty() == false) {
    on_gc_callback.Reset();
    isolate->RemoveGCEpilogueCallback(AfterGC);
  }
  if (args[0]->IsFunction() == true) {
    on_gc_callback.Reset(isolate, args[0].As<Function>());
    isolate->AddGCEpilogueCallback(AfterGC);
  }
}

void Initialize(Isolate* isolate, Handle<Object> target) {
  uv_idle_init(uv_default_loop(), &idle_handle);
  uv_unref(reinterpret_cast<uv_handle_t*>(&idle_handle));
  target->Set(FixedString(isolate, "onGC"),
              FunctionTemplate::New(isolate, OnGC)->GetFunction());
}

}  // namespace gcinfo
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_GCINFO_V0_12_H_
