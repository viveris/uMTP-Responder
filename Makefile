CFLAGS += -I./inc -lpthread -Wall

sources := $(wildcard src/*.c)
objects := $(sources:src/%.c=obj/%.o)

ops_sources := $(wildcard src/mtp_operations/*.c)
ops_objects := $(ops_sources:src/mtp_operations/%.c=obj/%.o)

ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -g -DDEBUG
else
	CFLAGS += -O3
	LDFLAGS += -s
endif

ifeq ($(SYSTEMD), 1)
	CFLAGS += -DSYSTEMD_NOTIFY
	LDFLAGS += -lsystemd
endif

ifeq ($(USE_SYSLOG), 1)
	CFLAGS += -DUSE_SYSLOG
endif

ifeq ($(OLD_FUNCTIONFS_DESCRIPTORS), 1)
	CFLAGS += -DOLD_FUNCTIONFS_DESCRIPTORS
endif

all: umtprd

umtprd: $(objects) $(ops_objects)
	${CC} -o $@    $^ $(LDFLAGS) -lpthread

$(objects): obj/%.o: src/%.c | output_dir
	${CC} -o $@ $^ -c $(CPPFLAGS) $(CFLAGS)

$(ops_objects): obj/%.o: src/mtp_operations/%.c | output_dir
	${CC} -o $@ $^ -c $(CPPFLAGS) $(CFLAGS)

output_dir:
	@mkdir -p obj

clean:
	rm -Rf  *.o  .*.o  .*.o.* *.ko  .*.ko  *.mod.* .*.mod.* .*.cmd umtprd obj

help:
	@echo uMTP-Responder build help :
	@echo
	@echo Normal build :
	@echo "make"
	@echo
	@echo Cross compiled build :
	@echo "make CC=armv6j-hardfloat-linux-gnueabi-gcc"
	@echo
	@echo Syslog support enabled build :
	@echo "make USE_SYSLOG=1"
	@echo
	@echo systemd notify support enabled build :
	@echo "make SYSTEMD=1"
	@echo
	@echo build with old-style FunctionFS descriptors support for old 3.15 kernels :
	@echo "make OLD_FUNCTIONFS_DESCRIPTORS=1"
	@echo
	@echo Debug build :
	@echo "make DEBUG=1"
	@echo
	@echo You can combine most of these options.
