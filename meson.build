project('uMTP-Responder', 'c',
  default_options : ['warning_level=1'])

with_usb_fs_support = get_option('usb-fs-support')
if with_usb_fs_support
  add_project_arguments('-DCONFIG_USB_FS_SUPPORT=1', language: 'c')
endif

with_usb_hs_support = get_option('usb-hs-support')
if with_usb_hs_support
  add_project_arguments('-DCONFIG_USB_HS_SUPPORT=1', language: 'c')
endif

with_usb_ss_support = get_option('usb-ss-support')
if with_usb_ss_support
  add_project_arguments('-DCONFIG_USB_SS_SUPPORT=1', language: 'c')
endif

if not (with_usb_fs_support or with_usb_hs_support or with_usb_ss_support)
  error('At least one USB mode support must be enabled (usb-fs-support, usb-hs-support, usb-ss-support)!')
endif

with_debug = get_option('print-debug')
if with_debug
  add_project_arguments('-DDEBUG', language: 'c')
endif

use_syslog = get_option('use-syslog')
if use_syslog
  add_project_arguments('-DUSE_SYSLOG', language: 'c')
endif

with_systemd_notify = get_option('systemd-notify')
if with_systemd_notify
  add_project_arguments('-DSYSTEMD_NOTIFY', language: 'c')
endif

subdir('src')

umtprd_include_dirs  = include_directories('inc')

threads_dep = dependency('threads')
libsystemd_dep = dependency('libsystemd', required: with_systemd_notify)

umtprd_deps = [threads_dep, libsystemd_dep]

executable('umtprd',
           umtprd_sources,
           include_directories : umtprd_include_dirs,
           dependencies : umtprd_deps,
           install : true)
