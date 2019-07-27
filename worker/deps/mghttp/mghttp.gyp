{
  'targets':
  [
    {
      'target_name': 'mghttp',
      'type': 'static_library',
      'sources':
      [
        'mongoose.c'
      ],
      'direct_dependent_settings':
      {
        'include_dirs':
        [
          '.'
        ]
      },
      'conditions':
      [
        [ 'OS != "win"', {
          'cflags': [ '-Wall' ]
        }],
        [ 'OS == "mac"', {
          'xcode_settings':
          {
            'WARNING_CFLAGS': [ '-Wall' ]
          }
        }]
      ]
    }
  ]
}
