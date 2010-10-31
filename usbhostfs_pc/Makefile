OUTPUT=usbhostfs_pc
OBJS=main.o 
LIBS=-lpthread
CFLAGS=-Wall -ggdb -DPC_SIDE -D_FILE_OFFSET_BITS=64 -I. -O2 -I../usbhostfs -Wno-format
LDFLAGS=-L.

PREFIX=$(shell psp-config --pspdev-path 2> /dev/null)

ifdef BUILD_BIGENDIAN
CFLAGS += -DBUILD_BIGENDIAN
endif

ifdef NO_UID_CHECK
CFLAGS += -DNO_UID_CHECK
endif

ifdef WINUSB_DEVICE
OBJS += device_winusb.o
LIBS += -lsetupapi
else
OBJS += device_libusb.o
LIBS += -lusb
endif

ifdef READLINE_SHELL
CFLAGS += -DREADLINE_SHELL
LIBS += -lreadline -lcurses
endif

ifdef BUILD_WIN32
CFLAGS  += -I../windows/lib -Wno-format
LDFLAGS += -L../windows/lib
OUTPUT := $(OUTPUT).exe
endif

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LIBS)

install: $(OUTPUT)
	@echo "Installing $(OUTPUT)..."
	@if ( test $(PREFIX) ); then { mkdir -p $(PREFIX)/bin && cp $(OUTPUT) $(PREFIX)/bin; } else { echo "Error: psp-config not found!"; exit 1; } fi
	@echo "Done!"

clean:
	rm -f $(OUTPUT) *.o
