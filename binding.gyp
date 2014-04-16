# Copyright (c) 2014, StrongLoop Inc.
#
# This software is covered by the StrongLoop License.  See StrongLoop-LICENSE
# in the top-level directory or visit http://strongloop.com/license.

{
  'targets': [
    {
      'target_name': 'strong-agent',
      'cflags': [
        '-fvisibility=hidden',
        '-fno-exceptions',
        '-fno-rtti',
        '-Wall',
        '-Wextra',
      ],
      # Need to repeat the compiler flags in xcode-specific lingo,
      # gyp on mac ignores the cflags field.
      'xcode_settings': {
        'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',
        'GCC_ENABLE_CPP_RTTI': 'NO',
        # -Wno-invalid-offsetof is only necessary for gcc 4.2,
        # it prints bogus warnings for POD types.
        'GCC_WARN_ABOUT_INVALID_OFFSETOF_MACRO': 'NO',
        # -fvisibility=hidden
        'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',
        'WARNING_CFLAGS': ['-Wall', '-Wextra'],
      },
      'sources': [
        'src/extras-v0-10.h',
        'src/extras-v0-12.h',
        'src/gcinfo-baton-inl.h',
        'src/gcinfo-baton.h',
        'src/gcinfo-v0-10.h',
        'src/gcinfo-v0-12.h',
        'src/heapdiff-inl.h',
        'src/heapdiff-v0-10.h',
        'src/heapdiff-v0-12.h',
        'src/heapdiff.h',
        'src/profiler-v0-10.h',
        'src/profiler-v0-12.h',
        'src/strong-agent.cc',
        'src/strong-agent.h',
      ],
    }
  ]
}
