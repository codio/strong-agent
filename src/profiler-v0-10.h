// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_PROFILER_V0_10_H_
#define AGENT_SRC_PROFILER_V0_10_H_

#include "strong-agent.h"
#include "v8-profiler.h"

namespace strongloop {
namespace agent {
namespace profiler {

using v8::Arguments;
using v8::Array;
using v8::CpuProfile;
using v8::CpuProfileNode;
using v8::CpuProfiler;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Undefined;
using v8::Value;

Local<Object> ToObject(Isolate* isolate, const CpuProfileNode* node) {
  // Use a helper that caches the property strings.
  struct ToObjectHelper {
    explicit ToObjectHelper(Isolate* isolate) : isolate_(isolate) {
      call_uid_sym_ = FixedString(isolate, "callUid");
      children_count_sym_ = FixedString(isolate, "childrenCount");
      children_sym_ = FixedString(isolate, "children");
      function_name_sym_ = FixedString(isolate, "functionName");
      line_number_sym_ = FixedString(isolate, "lineNumber");
      script_name_sym_ = FixedString(isolate, "scriptName");
      self_samples_count_sym_ = FixedString(isolate, "selfSamplesCount");
      self_time_sym_ = FixedString(isolate, "selfTime");
      total_samples_count_sym_ = FixedString(isolate, "totalSamplesCount");
      total_time_sym_ = FixedString(isolate, "totalTime");
    }

    bool IsConstructed() const {
      return call_uid_sym_.IsEmpty() == false &&
             children_count_sym_.IsEmpty() == false &&
             children_sym_.IsEmpty() == false &&
             function_name_sym_.IsEmpty() == false &&
             line_number_sym_.IsEmpty() == false &&
             script_name_sym_.IsEmpty() == false &&
             self_samples_count_sym_.IsEmpty() == false &&
             self_time_sym_.IsEmpty() == false &&
             total_samples_count_sym_.IsEmpty() == false &&
             total_time_sym_.IsEmpty() == false;
    }

    Local<Object> ToObject(const CpuProfileNode* node) const {
      HandleScope handle_scope;
      const int children_count = node->GetChildrenCount();

      // Guard against out-of-memory situations, they're not unlikely when
      // the DAG is big.
      Local<Object> o = Object::New();
      if (o.IsEmpty()) return Local<Object>();

      Local<Number> self_samples_count_val =
          Number::New(node->GetSelfSamplesCount());
      if (self_samples_count_val.IsEmpty()) return Local<Object>();

      Local<Number> self_time_val = Number::New(node->GetSelfTime());
      if (self_time_val.IsEmpty()) return Local<Object>();

      Local<Number> total_samples_count_val =
          Number::New(node->GetTotalSamplesCount());
      if (total_samples_count_val.IsEmpty()) return Local<Object>();

      Local<Number> total_time_val = Number::New(node->GetTotalTime());
      if (total_time_val.IsEmpty()) return Local<Object>();

      Local<Integer> call_uid_val =
          Integer::NewFromUnsigned(node->GetCallUid(), isolate_);
      if (call_uid_val.IsEmpty()) return Local<Object>();

      Local<Integer> children_count_val =
          Integer::New(children_count, isolate_);
      if (children_count_val.IsEmpty()) return Local<Object>();

      Local<Integer> line_number_val =
          Integer::New(node->GetLineNumber(), isolate_);
      if (line_number_val.IsEmpty()) return Local<Object>();

      // The next two cannot really fail but the extra checks don't hurt.
      Handle<String> script_name_val = node->GetScriptResourceName();
      if (script_name_val.IsEmpty()) return Local<Object>();

      Handle<String> function_name_val = node->GetFunctionName();
      if (function_name_val.IsEmpty()) return Local<Object>();

      // Field order compatible with strong-cpu-profiler.
      o->Set(children_count_sym_, children_count_val);
      o->Set(call_uid_sym_, call_uid_val);
      o->Set(self_samples_count_sym_, self_samples_count_val);
      o->Set(total_samples_count_sym_, total_samples_count_val);
      o->Set(self_time_sym_, self_time_val);
      o->Set(total_time_sym_, total_time_val);
      o->Set(line_number_sym_, line_number_val);
      o->Set(script_name_sym_, script_name_val);
      o->Set(function_name_sym_, function_name_val);

      Local<Array> children = Array::New(children_count);
      if (children.IsEmpty()) return Local<Object>();
      for (int index = 0; index < children_count; ++index) {
        Local<Object> child = this->ToObject(node->GetChild(index));
        if (child.IsEmpty()) return Local<Object>();
        children->Set(index, child);
      }
      o->Set(children_sym_, children);

      return handle_scope.Close(o);
    }

    Isolate* isolate_;
    Local<String> call_uid_sym_;
    Local<String> children_count_sym_;
    Local<String> children_sym_;
    Local<String> function_name_sym_;
    Local<String> line_number_sym_;
    Local<String> script_name_sym_;
    Local<String> self_samples_count_sym_;
    Local<String> self_time_sym_;
    Local<String> total_samples_count_sym_;
    Local<String> total_time_sym_;
  };

  HandleScope handle_scope;
  ToObjectHelper helper(isolate);
  if (helper.IsConstructed() == false) {
    return Local<Object>();  // Out of memory.
  }
  return handle_scope.Close(helper.ToObject(node));
}

Handle<Value> StartCpuProfiling(const Arguments&) {
  HandleScope handle_scope;
  CpuProfiler::StartProfiling(String::Empty());
  return Undefined();
}

Handle<Value> StopCpuProfiling(const Arguments&) {
  HandleScope handle_scope;
  const CpuProfile* profile = CpuProfiler::StopProfiling(String::Empty());
  if (profile == NULL) {
    return Undefined();  // Not started or preempted by another profiler.
  }
  Local<Object> top_root = ToObject(Isolate::GetCurrent(),
                                    profile->GetTopDownRoot());
  CpuProfiler::DeleteAllProfiles();
  if (top_root.IsEmpty() == true) {
    return Undefined();  // Out of memory.
  }
  return handle_scope.Close(top_root);
}

void Initialize(Isolate* isolate, Handle<Object> o) {
  o->Set(FixedString(isolate, "startCpuProfiling"),
         FunctionTemplate::New(StartCpuProfiling)->GetFunction());
  o->Set(FixedString(isolate, "stopCpuProfiling"),
         FunctionTemplate::New(StopCpuProfiling)->GetFunction());
}

}  // namespace profiler
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_PROFILER_V0_10_H_
