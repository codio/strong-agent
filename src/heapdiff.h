// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_HEAPDIFF_H_
#define AGENT_SRC_HEAPDIFF_H_

#include "v8.h"
#include "v8-profiler.h"
#include <stdint.h>

#include <map>
#include <set>
#include <vector>

namespace strongloop {
namespace agent {
namespace heapdiff {

// Wrap heap nodes in a small wrapper class that caches the node identifier.
// v8::HeapGraphNode::GetId() is extraordinary slow because it looks up the
// current isolate in thread-local storage with pthread_getspecific().
// In benchmarks where the id is not cached, about 25% of CPU time is
// attributable to v8::HeapGraphNode::GetId().
class HeapGraphNodeWrap {
 public:
  explicit HeapGraphNodeWrap(const v8::HeapGraphNode* node);
  const v8::HeapGraphNode* node() const;
  v8::SnapshotObjectId id() const;
  bool operator<(const HeapGraphNodeWrap& that) const;
 private:
  const v8::HeapGraphNode* node_;
  v8::SnapshotObjectId id_;
};

class Key {
 public:
  explicit Key(v8::Handle<v8::String> string);
  // Copy constructor has move semantics, afterwards that.data() returns NULL.
  Key(const Key& that);
  ~Key();
  bool operator<(const Key& that) const;
  v8::Handle<v8::String> handle() const;
  const uint16_t* data() const;
  unsigned size() const;
  uint32_t hash() const;
 private:
  static const unsigned kMaxSize = 4096;
  v8::Handle<v8::String> handle_;
  mutable uint16_t* data_;
  mutable unsigned size_;
  mutable uint32_t hash_;
  uint16_t buffer_[32];
  void operator=(const Key&);
};

class Score {
 public:
  Score();
  int count() const;
  int size() const;
  void Plus(const HeapGraphNodeWrap& wrapper);
  void Minus(const HeapGraphNodeWrap& wrapper);
 private:
  int count_;
  int size_;
};

typedef std::map<Key, Score> HeapGraphNodeMap;
typedef std::set<HeapGraphNodeWrap> HeapGraphNodeSet;
typedef std::vector<HeapGraphNodeWrap> HeapGraphNodeVector;

// TODO(bnoordhuis) This should be in a util.h header.
uint32_t JenkinsHash(const uint8_t* data, unsigned size);

void AddHeapGraphNodeToSet(const v8::HeapGraphNode* node,
                           HeapGraphNodeSet* set);

// Returns an object that looks something like this:
//
//  [ { type: 'Timeout', total: 1, size: 136 },
//    { type: 'Timer', total: 2, size: 64 },
//    { type: 'Array', total: 1, size: 32 } ] }
//
// |type| is the object class name, |total| the number of instances that were
// created between the two snapshots and |size| is the aggregated self size,
// not the aggregated retained size!  The retained size is much more expensive
// to calculate.
//
// When |total| and |size| are negative, more instances have been reaped by
// the garbage collector than were created by the application.  |total| and
// |size| are always paired: if one is negative, then so is the other.
v8::Local<v8::Object> Summarize(v8::Isolate* isolate,
                                const v8::HeapSnapshot* start_snapshot,
                                const v8::HeapSnapshot* end_snapshot);

}  // namespace heapdiff
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_HEAPDIFF_H_
