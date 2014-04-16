// Copyright (c) 2014, StrongLoop Inc.
//
// This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
// in the top-level directory or visit http://strongloop.com/license.

#ifndef AGENT_SRC_STRONG_AGENT_H_
#define AGENT_SRC_STRONG_AGENT_H_

// Can't use `#pragma GCC diagnostic push/pop`, not supported by gcc 4.2.
#if defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wunused-parameter"
# if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 8
#  pragma GCC diagnostic ignored "-Wunused-local-typedefs"
# endif
#endif

#include "node_version.h"
#include "node.h"
#include "uv.h"
#include "v8.h"

#if defined(__GNUC__)
# if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 8
#  pragma GCC diagnostic warning "-Wunused-local-typedefs"
# endif
# pragma GCC diagnostic warning "-Wunused-parameter"
#endif

namespace strongloop {
namespace agent {

// For squelching warnings about unused parameters/variables.
template <typename T>
void Use(const T&) {
}

#define SL_ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

// Assumption: the release after v0.12 will be v1.0, as prophesied.
#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION >= 11
# define SL_NODE_VERSION 12  // v0.12
#elif NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION == 10
# define SL_NODE_VERSION 10  // v0.10
#else
# error "Unsupported node.js version."
#endif

template <size_t N>
v8::Local<v8::String> FixedString(v8::Isolate* isolate, const char (&s)[N]) {
#if SL_NODE_VERSION == 10
  Use(isolate);
  return v8::String::New(s, N - 1);
#elif SL_NODE_VERSION == 12
  return v8::String::NewFromOneByte(isolate,
                                    reinterpret_cast<const uint8_t*>(s),
                                    v8::String::kNormalString,
                                    N - 1);
#endif
}

namespace extras { void Initialize(v8::Isolate*, v8::Local<v8::Object>); }
namespace gcinfo { void Initialize(v8::Isolate*, v8::Local<v8::Object>); }
namespace heapdiff { void Initialize(v8::Isolate*, v8::Local<v8::Object>); }
namespace profiler { void Initialize(v8::Isolate*, v8::Local<v8::Object>); }
namespace uvmon { void Initialize(v8::Isolate*, v8::Local<v8::Object>); }

}  // namespace agent
}  // namespace strongloop

#endif  // AGENT_SRC_STRONG_AGENT_H_
