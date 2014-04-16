// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_GCINFO_BATON_INL_H_
#define AGENT_SRC_GCINFO_BATON_INL_H_

#include "strong-agent.h"
#include "gcinfo-baton.h"
#include "queue.h"

namespace strongloop {
namespace agent {
namespace gcinfo {

Baton* Baton::Pop() {
  if (QUEUE_EMPTY(&baton_queue)) {
    return static_cast<Baton*>(0);
  }
  QUEUE* q = QUEUE_HEAD(&baton_queue);
  QUEUE_REMOVE(q);
  return QUEUE_DATA(q, Baton, baton_queue_);
}

Baton* Baton::New(v8::Isolate* isolate,
                  v8::GCType type,
                  v8::GCCallbackFlags flags) {
  Baton* baton = new Baton(isolate, type, flags);
  QUEUE_INSERT_TAIL(&baton_queue, &baton->baton_queue_);
  return baton;
}

Baton::Baton(v8::Isolate* isolate, v8::GCType type, v8::GCCallbackFlags flags)
    : type_(type), flags_(flags) {
#if SL_NODE_VERSION == 10
  Use(isolate);
  v8::V8::GetHeapStatistics(&heap_statistics_);
#elif SL_NODE_VERSION == 12
  isolate->GetHeapStatistics(&heap_statistics_);
#endif
  QUEUE_INIT(&baton_queue_);
}

Baton::~Baton() {
}

v8::GCType Baton::type() const {
  return type_;
}

v8::GCCallbackFlags Baton::flags() const {
  return flags_;
}

v8::HeapStatistics* Baton::heap_statistics() const {
  // HeapStatistics is a getters-only class but its getters aren't marked const.
  return const_cast<v8::HeapStatistics*>(&heap_statistics_);
}

const char* Baton::type_string() const {
  switch (type()) {
#define V(name) case v8::name: return #name
    V(kGCTypeAll);
    V(kGCTypeScavenge);
    V(kGCTypeMarkSweepCompact);
#undef V
  }
  return "UnknownType";
}

const char* Baton::flags_string() const {
  switch (flags()) {
#define V(name) case v8::name: return #name
    V(kNoGCCallbackFlags);
    V(kGCCallbackFlagCompacted);
#if SL_NODE_VERSION == 12
    V(kGCCallbackFlagForced);
    V(kGCCallbackFlagConstructRetainedObjectInfos);
#endif
#undef V
  }
  return "UnknownFlags";
}

void Baton::Dispose() {
  delete this;
}

}  // namespace gcinfo
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_GCINFO_BATON_INL_H_
