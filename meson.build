project('uMTP-Responder', 'c',
    version: '1.6.8',
)

cc = meson.get_compiler('c')

add_project_arguments(['-Wall', '-I./inc'], language : 'c')
add_project_link_arguments(['-lpthread', '-lrt'], language : ['c', 'cpp'])

# Debug configuration
buildtype = get_option('buildtype')
if buildtype == 'debug'
	add_project_arguments('-DDEBUG', language : 'c')
endif

# Systemd support
systemd = get_option('systemd')
if systemd
	add_project_arguments('-DSYSTEMD_NOTIFY', language : 'c')
	add_project_link_arguments(['-lsystemd'], language : ['c', 'cpp'])
endif

# Syslog support
use_syslog = get_option('use_syslog')
if use_syslog
	add_project_arguments('-DUSE_SYSLOG', language : 'c')
endif

# Old-style FunctionFS descriptors
old_functionfs_descriptors = get_option('old_functionfs_descriptors')
if old_functionfs_descriptors
	add_project_arguments('-DOLD_FUNCTIONFS_DESCRIPTORS', language : 'c')
endif

# USB speed level support
usb_fs_support = get_option('usb_fs_support')
if usb_fs_support
	add_project_arguments('-DCONFIG_USB_FS_SUPPORT', language : 'c')
endif
usb_hs_support = get_option('usb_hs_support')
if usb_hs_support
	add_project_arguments('-DCONFIG_USB_HS_SUPPORT', language : 'c')
endif
usb_ss_support = get_option('usb_ss_support')
if usb_ss_support
	add_project_arguments('-DCONFIG_USB_SS_SUPPORT', language : 'c')
endif


sources = [
	'src/fs_handles_db.c',
	'src/inotify.c',
	'src/logs_out.c',
	'src/msgqueue.c',
	'src/mtp.c',
	'src/mtp_cfg.c',
	'src/mtp_constant_strings.c',
	'src/mtp_datasets.c',
	'src/mtp_helpers.c',
	'src/mtp_operations/mtp_op_begineditobject.c',
	'src/mtp_operations/mtp_op_closesession.c',
	'src/mtp_operations/mtp_op_deleteobject.c',
	'src/mtp_operations/mtp_op_endeditobject.c',
	'src/mtp_operations/mtp_op_getdeviceinfos.c',
	'src/mtp_operations/mtp_op_getdevicepropdesc.c',
	'src/mtp_operations/mtp_op_getdevicepropvalue.c',
	'src/mtp_operations/mtp_op_getobject.c',
	'src/mtp_operations/mtp_op_getobjecthandles.c',
	'src/mtp_operations/mtp_op_getobjectinfo.c',
	'src/mtp_operations/mtp_op_getobjectpropdesc.c',
	'src/mtp_operations/mtp_op_getobjectproplist.c',
	'src/mtp_operations/mtp_op_getobjectpropssupported.c',
	'src/mtp_operations/mtp_op_getobjectpropvalue.c',
	'src/mtp_operations/mtp_op_getobjectreferences.c',
	'src/mtp_operations/mtp_op_getpartialobject.c',
	'src/mtp_operations/mtp_op_getstorageids.c',
	'src/mtp_operations/mtp_op_getstorageinfo.c',
	'src/mtp_operations/mtp_op_opensession.c',
	'src/mtp_operations/mtp_op_sendobject.c',
	'src/mtp_operations/mtp_op_sendobjectinfo.c',
	'src/mtp_operations/mtp_op_setobjectpropvalue.c',
	'src/mtp_operations/mtp_ops_helpers.c',
	'src/mtp_operations/mtp_op_truncateobject.c',
	'src/mtp_properties.c',
	'src/mtp_support_def.c',
	'src/umtprd.c',
	'src/usb_gadget.c',
	'src/usbstring.c',
]

executable('umtprd', sources, include_directories : 'inc')
