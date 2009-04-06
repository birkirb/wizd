.SUFFIXES: .c .o

CFLAGS += -I/usr/local/include
LIBS += -L/usr/local/lib -ldvdread

all: dvd_titleinfo dvd_playtitle

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

dvd_titleinfo: dvd_titleinfo.o
	$(CC) dvd_titleinfo.o -o dvd_titleinfo $(LIBS)

dvd_playtitle: dvd_playtitle.o
	$(CC) dvd_playtitle.o -o dvd_playtitle $(LIBS)

clean:
	-rm -f *.o dvd_titleinfo dvd_playtitle
