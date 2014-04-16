// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_UVMON_V0_10_H_
#define AGENT_SRC_UVMON_V0_10_H_

#include "strong-agent.h"

namespace strongloop {
namespace agent {
namespace uvmon {

using v8::Handle;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::kExternalUnsignedIntArray;

uv_check_t check_handle;
uint32_t statistics[3];
uint32_t& slowest = statistics[0];
uint32_t& sum = statistics[1];
uint32_t& ticks = statistics[2];

void OnCheck(uv_check_t* handle, int) {
  const uv_loop_t* const loop = handle->loop;
  const uint64_t now = uv_hrtime() / static_cast<uint64_t>(1e6);
  const uint32_t delta = now <= loop->time ? 0 : (now - loop->time);
  if (delta > slowest) {
    slowest = delta;
  }
  ticks += 1;
  sum += delta;
}

void Initialize(Isolate* isolate, Handle<Object> target) {
  uv_check_init(uv_default_loop(), &check_handle);
  uv_check_start(&check_handle, OnCheck);
  uv_unref(reinterpret_cast<uv_handle_t*>(&check_handle));
  Local<Object> event_loop_statistics = Object::New();
  event_loop_statistics->SetIndexedPropertiesToExternalArrayData(
      statistics, kExternalUnsignedIntArray, SL_ARRAY_SIZE(statistics));
  target->Set(FixedString(isolate, "eventLoopStatistics"),
              event_loop_statistics);
}

}  // namespace uvmon
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_UVMON_V0_10_H_
