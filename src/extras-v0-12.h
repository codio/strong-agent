// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_EXTRAS_V0_12_H_
#define AGENT_SRC_EXTRAS_V0_12_H_

#include "strong-agent.h"

namespace strongloop {
namespace agent {
namespace extras {

using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Value;

void Forward(const FunctionCallbackInfo<Value>& args) {
  HandleScope handle_scope(args.GetIsolate());
  Local<Function> constructor = args.Data().As<Function>();
  Local<Value> result = constructor->Call(args.This(), 0, NULL);
  args.GetReturnValue().Set(result);
}

void Hide(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  Local<FunctionTemplate> function_template = FunctionTemplate::New(isolate);
  function_template->SetCallHandler(Forward, args[0]);
  function_template->InstanceTemplate()->MarkAsUndetectable();
  args.GetReturnValue().Set(function_template->GetFunction());
}

void Initialize(Isolate* isolate, Handle<Object> binding) {
  binding->Set(FixedString(isolate, "hide"),
               FunctionTemplate::New(isolate, Hide)->GetFunction());
}

}  // namespace extras
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_EXTRAS_V0_12_H_
