cmake_minimum_required(VERSION 3.5)

project(utmp-responder LANGUAGES C)

include(GNUInstallDirs)

option(DEBUG_PRINT "Enable the debug output" OFF)
option(OLD_FUNCTIONFS_DESCRIPTORS "build with old-style FunctionFS descriptors support for old 3.15 kernels" OFF)
option(USE_SYSLOG "syslog support" ON)
option(USE_SYSTEMD "systemd support (if available)" ON)
option(SUPER_SPEED "USB super-speed support" OFF)
option(HIGH_SPEED "USB high-speed support" ON)
option(FULL_SPEED "USB full-speed support" ON)

set(FILE_BUFFER_SZ "1024" CACHE FILEPATH "size of the file read buffer in kB. (Must be a power of 2)")
set(USB_TX_BUFFER_SZ "64" CACHE FILEPATH "size of the USB transmit buffer in kB")
set(USB_RX_BUFFER_SZ "64" CACHE FILEPATH "size of the USB receive buffer in kB")


FILE(GLOB SRC src/*.c src/mtp_operations/*.c)
add_executable(umtprd ${SRC})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

if (USE_SYSTEMD)
    pkg_check_modules(SYSTEMD "systemd")
    if(SYSTEMD_FOUND)
        target_compile_definitions(umtprd PUBLIC SYSTEMD_NOTIFY)
        if (NOT SYSTEMD_LIBRARIES)
            set(SYSTEMD_LIBRARIES -lsystemd)
        endif()
    endif()
endif()

if( USE_SYSLOG)
    target_compile_definitions(umtprd PUBLIC USE_SYSLOG)
endif()

if( SUPER_SPEED)
    target_compile_definitions(umtprd PUBLIC CONFIG_USB_SS_SUPPORT=1)
endif()
if( HIGH_SPEED)
    target_compile_definitions(umtprd PUBLIC CONFIG_USB_HS_SUPPORT=1)
endif()
if( FULL_SPEED)
    target_compile_definitions(umtprd PUBLIC CONFIG_USB_FS_SUPPORT=1)
endif()

math(EXPR FILE_BUFFER_SZ  "${FILE_BUFFER_SZ}*1024")
math(EXPR USB_TX_BUFFER_SZ  "${USB_TX_BUFFER_SZ}*1024")
math(EXPR USB_RX_BUFFER_SZ  "${USB_RX_BUFFER_SZ}*1024")
target_compile_definitions(umtprd PUBLIC CONFIG_READ_FILE_BUFFER_SIZE=${FILE_BUFFER_SZ})
target_compile_definitions(umtprd PUBLIC CONFIG_MAX_TX_USB_BUFFER_SIZE=${USB_TX_BUFFER_SZ})
target_compile_definitions(umtprd PUBLIC CONFIG_MAX_RX_USB_BUFFER_SIZE=${USB_RX_BUFFER_SZ})

if( DEBUG_PRINT)
    target_compile_definitions(umtprd PUBLIC DEBUG)
endif()

target_include_directories(umtprd PUBLIC inc/ ${SYSTEMD_INCLUDE_DIRS})
target_compile_options(umtprd PUBLIC ${SYSTEMD_CFLAGS_OTHER})
target_link_libraries(umtprd PUBLIC Threads::Threads ${SYSTEMD_LIBRARIES})

install(TARGETS umtprd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(PROGRAMS conf/umtprd-ffs.sh DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(PROGRAMS conf/umtprd_gfs.sh DESTINATION ${CMAKE_INSTALL_BINDIR})


