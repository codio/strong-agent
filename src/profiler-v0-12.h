// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_PROFILER_V0_12_H_
#define AGENT_SRC_PROFILER_V0_12_H_

#include "strong-agent.h"
#include "v8-profiler.h"
#include <string.h>

namespace strongloop {
namespace agent {
namespace profiler {

using v8::Array;
using v8::CpuProfile;
using v8::CpuProfileNode;
using v8::CpuProfiler;
using v8::EscapableHandleScope;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Value;

// Call with a valid HandleScope.
Local<Object> ToObject(Isolate* isolate, const CpuProfileNode* node) {
  // Use a helper that caches the property strings.
  struct ToObjectHelper {
    explicit ToObjectHelper(Isolate* isolate) : isolate_(isolate) {
      // Only collect information that exists both in v0.10 and v0.11.
      bailout_reason_sym_ = FixedString(isolate, "bailoutReason");
      children_sym_ = FixedString(isolate, "children");
      column_number_sym_ = FixedString(isolate, "columnNumber");
      function_name_sym_ = FixedString(isolate, "functionName");
      hit_count_sym_ = FixedString(isolate, "hitCount");
      line_number_sym_ = FixedString(isolate, "lineNumber");
      script_name_sym_ = FixedString(isolate, "scriptName");
    }

    bool IsConstructed() const {
      return bailout_reason_sym_.IsEmpty() == false &&
             children_sym_.IsEmpty() == false &&
             column_number_sym_.IsEmpty() == false &&
             function_name_sym_.IsEmpty() == false &&
             hit_count_sym_.IsEmpty() == false &&
             line_number_sym_.IsEmpty() == false &&
             script_name_sym_.IsEmpty() == false;
    }

    Local<Object> ToObject(const CpuProfileNode* node) const {
      EscapableHandleScope handle_scope(isolate_);
      // Guard against out-of-memory situations, they're not unlikely when
      // the DAG is big.
      Local<Object> o = Object::New(isolate_);
      if (o.IsEmpty()) return Local<Object>();

      // These two cannot really fail but the extra checks don't hurt.
      Handle<String> script_name_val = node->GetScriptResourceName();
      if (script_name_val.IsEmpty()) return Local<Object>();
      // Filter out empty strings.  Samples from native code don't have
      // script names associated with them.
      if (script_name_val->Length() > 0) {
        o->Set(script_name_sym_, script_name_val);
      }

      Handle<String> function_name_val = node->GetFunctionName();
      if (function_name_val.IsEmpty()) return Local<Object>();
      // Filter out anonymous function names, they are plentiful in most code
      // but uninteresting and we can save quite a bit of bandwidth this way.
      // The string comparison is done in a somewhat roundabout way for
      // performance reasons; writing out the string like this is a little
      // faster than using Equals() or String::AsciiValue.
      static const uint8_t anonymous_function[] = "(anonymous function)";
      uint8_t write_buffer[sizeof(anonymous_function) - 1] = { 0 };
      const bool check = function_name_val->Length() == sizeof(write_buffer);
      if (check) {
        function_name_val->WriteOneByte(write_buffer,
                                        0,
                                        sizeof(write_buffer),
                                        String::NO_NULL_TERMINATION);
      }
      if (check == false ||
          memcmp(anonymous_function, write_buffer, sizeof(write_buffer)) != 0) {
        o->Set(function_name_sym_, function_name_val);
      }

      // The hit count is frequently zero, meaning the function was in the
      // call tree on the stack somewhere but not actually sampled by the
      // profiler.  A zero hit count implies that this node is not a leaf
      // node, the actual sample is in one of its descendants.
      const unsigned int hit_count = node->GetHitCount();
      if (hit_count > 0) {
        Local<Integer> hit_count_val =
            Integer::NewFromUnsigned(isolate_, hit_count);
        if (hit_count_val.IsEmpty()) return Local<Object>();
        o->Set(hit_count_sym_, hit_count_val);
      }

      // TODO(bnoordhuis) There is only a limited number of bailout reasons.
      // Collect them in a tree-like structure that caches the String handles.
      const char* const bailout_reason = node->GetBailoutReason();
      if (bailout_reason != NULL &&
          bailout_reason[0] != '\0' &&
          ::strcmp(bailout_reason, "no reason") != 0) {
        const uint8_t* const bytes =
            reinterpret_cast<const uint8_t*>(bailout_reason);
        Local<String> bailout_reason_val =
            String::NewFromOneByte(isolate_, bytes);
        if (bailout_reason_val.IsEmpty()) return Local<Object>();
        o->Set(bailout_reason_sym_, bailout_reason_val);
      }

      // Note: Line and column numbers start at 1.  Skip setting the property
      // when there is no line or column number information available for this
      // function, saves bandwidth when sending the profile data over the
      // network.
      const int line_number = node->GetLineNumber();
      if (line_number != CpuProfileNode::kNoLineNumberInfo) {
        Local<Integer> line_number_val = Integer::New(isolate_, line_number);
        if (line_number_val.IsEmpty()) return Local<Object>();
        o->Set(line_number_sym_, line_number_val);
      }

      const int column_number = node->GetColumnNumber();
      if (column_number != CpuProfileNode::kNoColumnNumberInfo) {
        Local<Integer> column_number_val =
            Integer::New(isolate_, column_number);
        if (column_number_val.IsEmpty()) return Local<Object>();
        o->Set(column_number_sym_, column_number_val);
      }

      // Don't create the "children" property for leaf nodes, saves memory
      // and bandwidth.
      const int children_count = node->GetChildrenCount();
      if (children_count > 0) {
        Local<Array> children = Array::New(isolate_, children_count);
        if (children.IsEmpty()) return Local<Object>();
        for (int index = 0; index < children_count; ++index) {
          Local<Object> child = this->ToObject(node->GetChild(index));
          if (child.IsEmpty()) return Local<Object>();
          children->Set(index, child);
        }
        o->Set(children_sym_, children);
      }

      return handle_scope.Escape(o);
    }

    Isolate* isolate_;
    Local<String> bailout_reason_sym_;
    Local<String> children_sym_;
    Local<String> column_number_sym_;
    Local<String> function_name_sym_;
    Local<String> hit_count_sym_;
    Local<String> line_number_sym_;
    Local<String> script_name_sym_;
  };

  EscapableHandleScope handle_scope(isolate);
  ToObjectHelper helper(isolate);
  if (helper.IsConstructed() == false) {
    return Local<Object>();  // Out of memory.
  }
  return handle_scope.Escape(helper.ToObject(node));
}

void StartCpuProfiling(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  isolate->GetCpuProfiler()->StartCpuProfiling(String::Empty(isolate));
}

void StopCpuProfiling(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  CpuProfiler* profiler = isolate->GetCpuProfiler();
  const CpuProfile* profile =
      profiler->StopCpuProfiling(String::Empty(isolate));
  if (profile == NULL) {
    return;  // Not started or preempted by another profiler.
  }
  Local<Object> top_root = ToObject(isolate, profile->GetTopDownRoot());
  // See https://code.google.com/p/v8/issues/detail?id=3213.
  const_cast<CpuProfile*>(profile)->Delete();
  args.GetReturnValue().Set(top_root);
}

void Initialize(Isolate* isolate, Handle<Object> o) {
  o->Set(FixedString(isolate, "startCpuProfiling"),
         FunctionTemplate::New(isolate, StartCpuProfiling)->GetFunction());
  o->Set(FixedString(isolate, "stopCpuProfiling"),
         FunctionTemplate::New(isolate, StopCpuProfiling)->GetFunction());
}

}  // namespace profiler
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_PROFILER_V0_12_H_
