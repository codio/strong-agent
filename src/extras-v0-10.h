// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_EXTRAS_V0_10_H_
#define AGENT_SRC_EXTRAS_V0_10_H_

#include "strong-agent.h"

namespace strongloop {
namespace agent {
namespace extras {

using v8::Arguments;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Value;

Handle<Value> Forward(const Arguments& args) {
  HandleScope handle_scope;
  Local<Function> constructor = args.Data().As<Function>();
  Local<Value> result = constructor->Call(args.This(), 0, NULL);
  return handle_scope.Close(result);
}

Handle<Value> Hide(const Arguments& args) {
  HandleScope handle_scope;
  Local<FunctionTemplate> function_template = FunctionTemplate::New();
  function_template->SetCallHandler(Forward, args[0]);
  function_template->InstanceTemplate()->MarkAsUndetectable();
  return handle_scope.Close(function_template->GetFunction());
}

void Initialize(Isolate* isolate, Handle<Object> target) {
  target->Set(FixedString(isolate, "hide"),
              FunctionTemplate::New(Hide)->GetFunction());
}

}  // namespace extras
}  // namespace agent
}  // namespace strongloop


#endif  // AGENT_SRC_EXTRAS_V0_10_H_
