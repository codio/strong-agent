// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_HEAPDIFF_INL_H_
#define AGENT_SRC_HEAPDIFF_INL_H_

#include "heapdiff.h"
#include <string.h>

#include <algorithm>
#include <iterator>

namespace strongloop {
namespace agent {
namespace heapdiff {

HeapGraphNodeWrap::HeapGraphNodeWrap(const v8::HeapGraphNode* node)
  : node_(node),
    id_(node->GetId()) {
}

const v8::HeapGraphNode* HeapGraphNodeWrap::node() const {
  return node_;
}

v8::SnapshotObjectId HeapGraphNodeWrap::id() const {
  return id_;
}

bool HeapGraphNodeWrap::operator<(const HeapGraphNodeWrap& that) const {
  return id() < that.id();
}

Key::Key(v8::Handle<v8::String> handle)
  : handle_(handle),
    data_(NULL),
    size_(0),
    hash_(0) {
  uint16_t* data = buffer_;
  unsigned maxsize = SL_ARRAY_SIZE(buffer_);
  unsigned size = 0;
  // HINT_MANY_WRITES_EXPECTED flattens cons strings before writing on the
  // assumption that we'll be processing the same cons strings repeatedly.
  // Seems like a reasonable assumption to make because there will normally
  // be many objects with the same class name.  Class names are usually flat
  // strings to start with so it might be a wash but the hint is unlikely to
  // hurt.
  const int options =
      v8::String::HINT_MANY_WRITES_EXPECTED | v8::String::NO_NULL_TERMINATION;
  for (;;) {
    // The choice for String::Write() is intentional.  String::WriteAscii() and
    // particularly String::WriteUtf8() are tremendously slow in comparison.
    size += handle->Write(data + size, size, maxsize - size, options);
    if (size < maxsize) {
      break;
    }
    unsigned const new_maxsize = 2 * maxsize;
    if (new_maxsize >= kMaxSize) {
      if (data != buffer_) {
        delete[] data;
      }
      return;
    }
    uint16_t* const new_data = new uint16_t[new_maxsize];
    ::memcpy(new_data, data, maxsize * sizeof(*data));
    if (data != buffer_) {
      delete[] data;
    }
    data = new_data;
    maxsize = new_maxsize;
  }
  data_ = data;
  size_ = size;
  hash_ = JenkinsHash(reinterpret_cast<const uint8_t*>(data), size);
}

Key::~Key() {
  if (data_ != buffer_) {
    delete[] data_;
  }
}

Key::Key(const Key& that)
  : handle_(that.handle_),
    data_(that.data_),
    size_(that.size_),
    hash_(that.hash_) {
  if (that.data_ == that.buffer_) {
    data_ = static_cast<uint16_t*>(::memcpy(buffer_, that.buffer_, that.size_));
  }
  that.data_ = NULL;
  that.size_ = 0;
  that.hash_ = 0;
}

bool Key::operator<(const Key& that) const {
  if (hash() < that.hash()) {
    return true;
  }
  if (hash() > that.hash()) {
    return false;
  }
  if (size() < that.size()) {
    return true;
  }
  if (size() > that.size()) {
    return false;
  }
  return ::memcmp(data(), that.data(), size()) < 0;
}

v8::Handle<v8::String> Key::handle() const {
  return handle_;
}

const uint16_t* Key::data() const {
  return data_;
}

unsigned Key::size() const {
  return size_;
}

uint32_t Key::hash() const {
  return hash_;
}

Score::Score() : count_(0), size_(0) {
}

int Score::count() const {
  return count_;
}

int Score::size() const {
  return size_;
}

void Score::Plus(const HeapGraphNodeWrap& wrap) {
  const v8::HeapGraphNode* node = wrap.node();
  count_ += 1, size_ += node->GetSelfSize();
}

void Score::Minus(const HeapGraphNodeWrap& wrap) {
  const v8::HeapGraphNode* node = wrap.node();
  count_ -= 1, size_ -= node->GetSelfSize();
}

uint32_t JenkinsHash(const uint8_t* data, unsigned size) {
  uint32_t hash = 0;
  for (unsigned index = 0; index < size; index += 1) {
    hash += data[index];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}

void AddHeapGraphNodeToSet(const v8::HeapGraphNode* node,
                           HeapGraphNodeSet* set) {
  // Heap numbers are numbers that don't fit in a SMI (a tagged pointer),
  // either because they're fractional or too large.  I'm not 100% sure
  // it's okay to filter them out because excessive heap number allocation
  // is a somewhat frequent source of performance issues.  On the other hand,
  // we're serializing the heap snapshot and sending it over the wire.
  // Including heap numbers would balloon the size of the payload.
  // Maybe we should simply keep track of the _number_ of heap numbers.
  if (node->GetType() == v8::HeapGraphNode::kHeapNumber) {
    return;
  }
  if (set->insert(HeapGraphNodeWrap(node)).second == false) {
    // Already in set.  We've processed this node before so there's no need
    // to iterate over its children.
    return;
  }
  const int children_count = node->GetChildrenCount();
  for (int index = 0; index < children_count; index += 1) {
    const v8::HeapGraphEdge* edge = node->GetChild(index);
    const v8::HeapGraphEdge::Type type = edge->GetType();
    // Filter out uninteresting edge types.
    //
    //  - Internal links are cons strings slices, relocation data, etc.
    //  - Shortcuts are predominantly the glue objects for functions bound
    //    with Function#bind().
    //  - Weak references (almost?) always point to internal oddbals that
    //    cannot be inspected.
    //
    // Hidden links need to be followed!  While they are usually backlinks for
    // retained size calculations, in V8 3.14 they also interact with eval().
    // It should be safe to skip them with V8 3.22 and newer but I'm reluctant
    // to introduce multiple code paths for something that has relatively
    // little overhead.  See also test/test-addon-heapdiff-eval.js.
    if (type != v8::HeapGraphEdge::kInternal &&
        type != v8::HeapGraphEdge::kShortcut &&
        type != v8::HeapGraphEdge::kWeak) {
      AddHeapGraphNodeToSet(edge->GetToNode(), set);
    }
  }
}

template <void (Score::*Method)(const HeapGraphNodeWrap&)>
struct SummarizeHelper {
  typedef ptrdiff_t difference_type;
  typedef std::output_iterator_tag iterator_category;
  typedef Score* pointer;
  typedef Score& reference;
  typedef Score value_type;
  explicit SummarizeHelper(HeapGraphNodeMap* map) : map_(map) {
  }
  void operator++() {
  }
  SummarizeHelper& operator*() {
    return *this;
  }
  void operator=(const HeapGraphNodeWrap& wrap) {
    const v8::HeapGraphNode* node = wrap.node();
    if (node->GetType() != v8::HeapGraphNode::kObject) {
      return;
    }
    Key key(node->GetName());
    if (key.data() == NULL) {
      return;  // Bailed out because string is too big.
    }
    Score& score = (*map_)[key];
    (score.*Method)(wrap);
  }
  HeapGraphNodeMap* map_;
};

v8::Local<v8::Object> Summarize(v8::Isolate* isolate,
                                const v8::HeapSnapshot* start_snapshot,
                                const v8::HeapSnapshot* end_snapshot) {
  HeapGraphNodeSet start_objects;
  HeapGraphNodeSet end_objects;
  AddHeapGraphNodeToSet(start_snapshot->GetRoot(), &start_objects);
  AddHeapGraphNodeToSet(end_snapshot->GetRoot(), &end_objects);

  HeapGraphNodeMap summary;
  HeapGraphNodeVector added_objects;
  HeapGraphNodeVector removed_objects;
  std::set_difference(end_objects.begin(),
                      end_objects.end(),
                      start_objects.begin(),
                      start_objects.end(),
                      SummarizeHelper<&Score::Plus>(&summary));
  std::set_difference(start_objects.begin(),
                      start_objects.end(),
                      end_objects.begin(),
                      end_objects.end(),
                      SummarizeHelper<&Score::Minus>(&summary));

  v8::Local<v8::String> type_string = FixedString(isolate, "type");
  v8::Local<v8::String> total_string = FixedString(isolate, "total");
  v8::Local<v8::String> size_string = FixedString(isolate, "size");

  uint32_t index = 0;
#if SL_NODE_VERSION == 12
  v8::Local<v8::Array> result = v8::Array::New(isolate);
#elif SL_NODE_VERSION == 10
  v8::Local<v8::Array> result = v8::Array::New();
#endif
  for (HeapGraphNodeMap::const_iterator it = summary.begin(),
       end = summary.end(); it != end; ++it) {
    const Key& key = it->first;
    const Score& score = it->second;
#if SL_NODE_VERSION == 12
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    object->Set(total_string, v8::Integer::New(isolate, score.count()));
    object->Set(size_string, v8::Integer::New(isolate, score.size()));
#elif SL_NODE_VERSION == 10
    v8::Local<v8::Object> object = v8::Object::New();
    object->Set(total_string, v8::Integer::New(score.count()));
    object->Set(size_string, v8::Integer::New(score.size()));
#endif
    object->Set(type_string, key.handle());
    result->Set(index, object);
    index += 1;
  }

  return result;
}

}  // namespace heapdiff
}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_HEAPDIFF_INL_H_
