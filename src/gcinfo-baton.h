// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_GCINFO_BATON_H_
#define AGENT_SRC_GCINFO_BATON_H_

#include "queue.h"
#include "strong-agent.h"

namespace strongloop {
namespace agent {
namespace gcinfo {

#define BATON_QUEUE_STATIC_INITIALIZER()                                      \
  QUEUE Baton::baton_queue = { &Baton::baton_queue, &Baton::baton_queue };

class Baton {
 public:
  static Baton* New(v8::Isolate* isolate,
                    v8::GCType type,
                    v8::GCCallbackFlags flags);
  static Baton* Pop();
  v8::GCType type() const;
  v8::GCCallbackFlags flags() const;
  const char* type_string() const;
  const char* flags_string() const;
  v8::HeapStatistics* heap_statistics() const;
  void Dispose();
 private:
  Baton(v8::Isolate* isolate, v8::GCType type, v8::GCCallbackFlags flags);
  // Only allow deletion through Baton::Dispose().
  ~Baton();
  // Forbid copy and assigment.
  Baton(const Baton&);
  void operator=(const Baton&);
  static QUEUE baton_queue;
  QUEUE baton_queue_;
  v8::GCType type_;
  v8::GCCallbackFlags flags_;
  v8::HeapStatistics heap_statistics_;
};

}  // namespace gcinfo
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_GCINFO_BATON_H_
