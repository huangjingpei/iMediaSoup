# NOTE: Inspired by
#   https://chromium.googlesource.com/chromium/src/+/master/third_party/usrsctp/BUILD.gn
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'dependencies': [
      # We need our own openssl dependency (required if SCTP_USE_OPENSSL_SHA1
      # is defined).
      #'../openssl/openssl.gyp:openssl',
    ],
    'defines': [
      #'SCTP_PROCESS_LEVEL_LOCKS',
      #'SCTP_SIMPLE_ALLOCATOR',
      #'SCTP_USE_OPENSSL_SHA1',
      #'__Userspace__',
      # 'SCTP_DEBUG', # Uncomment for SCTP debugging.
    ],
    'direct_dependent_settings': {
      'include_dirs': [
        #'usrsctp/usrsctplib',
        #'usrsctp/usrsctplib/netinet',
      ],
    },
    'include_dirs': [
      #'usrsctp/usrsctplib',
      # 'usrsctp/usrsctplib/netinet', # Not needed (it seems).
    ],
    'cflags': [
      #'-UINET',
      #'-UINET6',
    ],
    'xcode_settings': {
      'OTHER_CFLAGS': [
        '-UINET',
        '-UINET6',
      ],
    },
    'conditions': [
      ['OS in "linux android"', {
        'defines': [
          '__Userspace_os_Linux',
          '_GNU_SOURCE',
        ],
      }],
      ['OS in "mac ios"', {
        'defines': [
          'HAVE_SA_LEN',
          'HAVE_SCONN_LEN',
          '__APPLE_USE_RFC_2292',
          '__Userspace_os_Darwin',
        ],
        'xcode_settings': {
          'WARNING_CFLAGS': [
            '-Wall',
            '-Wextra',
            '-Wno-unused-parameter',
            '-Wstrict-prototypes',
          ],
          'OTHER_CFLAGS': [
            '-U__APPLE__',
            '-Wno-deprecated-declarations',
            # atomic_init in user_atomic.h is a static function in a header.
            '-Wno-unused-function',
          ],
        }
      }],
      ['OS=="win"', {
        'defines': [
          '__Userspace_os_Windows'
        ],
      }],
      ['OS!="win"', {
        'defines': [
          #'NON_WINDOWS_DEFINE'
        ],
      }],
      [ 'sctp_debug == "true"', {
        #'defines': [ 'SCTP_DEBUG' ]
      }]
    ],
  },
  'targets': [
    {
      'target_name': 'sdp',
      'type': 'static_library',
      'sources': [
        'SdpInfo.h',
        'SdpInfo.cpp',
        'StringUtil.h',
        'StringUtil.cpp',
        'RtpHeaders.h',
      ],
    },
  ], # targets
}
