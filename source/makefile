################################################################################
# wizd makefile
#						$Revision: 1.17 $
#						$Date: 2005/01/27 10:36:04 $
################################################################################
.SUFFIXES: .c .o .obj

CC = gcc
CFLAGS = -pipe -O2 -Wall
CFLAGS += -g	# debug symbols
CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -D_LARGEFILE_SOURCE
### -- If you dont want to use JPEG resize feature, comment it out.
CFLAGS += -DRESIZE_JPEG
CFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib -ljpeg
### --
LDFLAGS += -s	# strip symbols

#CFLAGS += -pg	# for gprof

# not tested... if you wanna use them, config it by yourself
#WINCC = i386-cygwin32-gcc
WINCC = mingw32-gcc
WINCFLAGS = -pipe -O2 -Wall
WINCFLAGS += -s	# -s means strip :)
WINCFLAGS += -D_FILE_OFFSET_BITS=64
WINCFLAGS += -DUTF8_INPUT_ENABLE
WINCFLAGS += -D_LARGEFILE_SOURCE
WINCFLAGS += -mcygwin
WINCFLAGS += -I/usr/local/include
WINLDFLAGS += -L/usr/local/lib -ljpeg

all:	wizd

wizd:	makefile		\
	wizd_main.o	\
	wizd_param.o	\
	wizd.h 		\
	wizd_listen.o	\
	wizd_http.o	\
	wizd_menu.o	\
	wizd_send_file.o	\
	wizd_send_svi.o	\
	wizd_send_vob.o	\
	wizd_proxy.o	\
	wizd_base64.o	\
	wizd_cgi.o	\
	wizd_aviread.o	\
	wizd_resize_jpeg.o	\
	wizd_detect.o	\
	wizd_tools.h	\
	wizd_tools.o	\
	wizd_skin.h	\
	wizd_skin.o	\
	nkfwrap/nkfwrap.o
	$(CC) $(CFLAGS)  -o wizd  		\
		wizd_main.o  	\
		wizd_param.o	\
		wizd_listen.o	\
		wizd_http.o	\
		wizd_menu.o	\
		wizd_send_file.o	\
		wizd_send_svi.o	\
		wizd_send_vob.o	\
		wizd_proxy.o	\
		wizd_base64.o	\
		wizd_cgi.o	\
		wizd_aviread.o	\
		wizd_resize_jpeg.o	\
		wizd_detect.o	\
		wizd_tools.o	\
		wizd_skin.o	\
		nkfwrap/nkfwrap.o		\
		$(LDFLAGS)

wizd_inetd:	makefile		\
	wizd_inetd.o	\
	wizd_param.o	\
	wizd.h 		\
	wizd_listen.o	\
	wizd_http.o	\
	wizd_menu.o	\
	wizd_send_file.o	\
	wizd_send_svi.o	\
	wizd_send_vob.o	\
	wizd_proxy.o	\
	wizd_base64.o	\
	wizd_cgi.o	\
	wizd_aviread.o	\
	wizd_resize_jpeg.o	\
	wizd_detect.o	\
	wizd_tools.h	\
	wizd_tools.o	\
	wizd_skin.h	\
	wizd_skin.o	\
	nkfwrap/nkfwrap.o
	$(CC) $(CFLAGS) -s -o wizd_inetd  		\
		wizd_inetd.o  	\
		wizd_param.o	\
		wizd_listen.o	\
		wizd_http.o	\
		wizd_menu.o	\
		wizd_send_file.o	\
		wizd_send_svi.o	\
		wizd_send_vob.o	\
		wizd_proxy.o	\
		wizd_base64.o	\
		wizd_cgi.o	\
		wizd_aviread.o	\
		wizd_resize_jpeg.o	\
		wizd_detect.o	\
		wizd_tools.o	\
		wizd_skin.o	\
		nkfwrap/nkfwrap.o		\
		$(LDFLAGS)

wizd.exe:	makefile		\
	wizd_main.obj	\
	wizd_param.obj	\
	wizd.h 		\
	wizd_listen.obj	\
	wizd_http.obj	\
	wizd_menu.obj	\
	wizd_send_file.obj	\
	wizd_send_svi.obj	\
	wizd_send_vob.obj	\
	wizd_proxy.obj	\
	wizd_base64.obj	\
	wizd_cgi.obj	\
	wizd_aviread.obj	\
	wizd_detect.obj	\
	wizd_tools.h	\
	wizd_tools.obj	\
	nkfwrap/nkfwrap.obj	
	$(WINCC) $(WINCFLAGS)  -o wizd.exe  		\
		wizd_main.obj  	\
		wizd_param.obj	\
		wizd_listen.obj	\
		wizd_http.obj	\
		wizd_menu.obj	\
		wizd_send_file.obj	\
		wizd_send_svi.obj	\
		wizd_send_vob.obj	\
		wizd_proxy.obj	\
		wizd_base64.obj	\
		wizd_cgi.obj	\
		wizd_aviread.obj	\
		wizd_detect.obj	\
		wizd_tools.obj	\
		nkfwrap/nkfwrap.obj		\
		$(LDFLAGS)

myavinfo: myavinfo.o wizd_aviread.o wizd_aviread.h
	gcc 	-pipe -O2 -Wall   		\
		-o myavinfo				\
		myavinfo.o wizd_aviread.o

nkfwrap/nkfwrap.o:
	cd nkfwrap; $(MAKE) all

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c.obj:
	$(WINCC) $(WINCFLAGS) -c $< -o $@


clean:
	cd nkfwrap; $(MAKE) clean
	rm -f wizd *.o

moreclean:
	cd nkfwrap; $(MAKE) moreclean
	rm -f wizd *.o
