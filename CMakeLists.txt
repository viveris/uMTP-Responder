cmake_minimum_required(VERSION 3.5)

project(utmp-responder LANGUAGES C)

include(GNUInstallDirs)

option(DEBUG_PRINT "Enable the debug output" OFF)
option(OLD_FUNCTIONFS_DESCRIPTORS "build with old-style FunctionFS descriptors support for old 3.15 kernels" OFF)
option(USE_SYSLOG "syslog support" ON)
option(USE_SYSTEMD "systemd support (if available)" ON)


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

if( DEBUG_PRINT)
    target_compile_definitions(umtprd PUBLIC DEBUG)
endif()

target_include_directories(umtprd PUBLIC inc/ ${SYSTEMD_INCLUDE_DIRS})
target_compile_options(umtprd PUBLIC ${SYSTEMD_CFLAGS_OTHER})
target_link_libraries(umtprd PUBLIC Threads::Threads ${SYSTEMD_LIBRARIES})

install(TARGETS umtprd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(PROGRAMS conf/umtprd-ffs.sh DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(PROGRAMS conf/umtprd_gfs.sh DESTINATION ${CMAKE_INSTALL_BINDIR})


