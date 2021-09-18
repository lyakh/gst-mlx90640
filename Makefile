t1 := gstmlx90640
t2 := gstmlx90640bar
api := MLX90640_API

ARM_TC := arm-linux-gnueabihf
X86_TC := x86_64-linux-gnu

TC := $(X86_TC)

CFLAGS := -Wall -Werror -fPIC -pthread -I/usr/lib/$(TC)/glib-2.0/include -I${HOME}/.local/include/gstreamer-1.0 -I/usr/include/orc-0.4 -I/usr/include/glib-2.0
CFLAGS += -I/usr/lib/$(TC)/glib-2.0/include

t1_CFLAGS := $(CFLAGS) -I${HOME}/usr/src/packages/mlx90640/mlx90640-library/headers
t1_CFLAGS += -DSYS_PATH=\"${HOME}/usr/src/packages/mlx90640/nvm.bin\"

LDFLAGS := -L${HOME}/.local/lib/$(TC) -shared
LIBS := -lgstvideo-1.0 -lgstbase-1.0 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0

all:		$(t1).so $(t2).so

.PHONY:		all

$(api).o:	$(api).c $(api).h
		$(CC) $(t1_CFLAGS) -c -o $@ $<

$(t1).o:	$(t1).c $(t1).h $(api).h
		$(CC) $(t1_CFLAGS) -c -o $@ $<

$(t2).o:	$(t2).c $(t2).h $(t1).h
		$(CC) $(CFLAGS) -c -o $@ $<

t1_OBJS := $(t1).o
#t1_OBJS += $(api).o

t2_OBJS := $(t2).o

$(t1).so:	$(t1_OBJS)
		$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(t2).so:	$(t2_OBJS)
		$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
