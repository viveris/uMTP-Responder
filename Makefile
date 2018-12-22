
override CFLAGS += -I./inc -lpthread -Wall -O3

all: output_dir umtprd

umtprd: obj/umtprd.o obj/mtp.o obj/mtp_datasets.o obj/mtp_helpers.o \
		obj/mtp_support_def.o obj/mtp_constant_strings.o obj/fs_handles_db.o \
		obj/usb_gadget.o obj/logs_out.o obj/usbstring.o obj/mtp_cfg.o obj/inotify.o
	${CC} -o $@    $^ $(LDFLAGS) -lpthread

obj/umtprd.o: src/umtprd.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/mtp.o: src/mtp.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/mtp_datasets.o: src/mtp_datasets.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/mtp_helpers.o: src/mtp_helpers.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/mtp_support_def.o: src/mtp_support_def.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/mtp_constant_strings.o: src/mtp_constant_strings.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/mtp_cfg.o: src/mtp_cfg.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/fs_handles_db.o: src/fs_handles_db.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/usb_gadget.o: src/usb_gadget.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/logs_out.o: src/logs_out.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/usbstring.o: src/usbstring.c
	${CC} -o $@ $^ -c $(CFLAGS)

obj/inotify.o: src/inotify.c
	${CC} -o $@ $^ -c $(CFLAGS)

output_dir:
	@mkdir -p obj

clean:
	rm -Rf  *.o  .*.o  .*.o.* *.ko  .*.ko  *.mod.* .*.mod.* .*.cmd umtprd obj

