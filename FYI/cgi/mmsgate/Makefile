.SUFFIXES: .c .o

TARGET = mmsgate.cgi

all: $(TARGET)

$(TARGET): mmsgate.o
	$(CC) $(CFLAGS) -o $(TARGET) mmsgate.c

clean:
	-rm -f *.o $(TARGET)
