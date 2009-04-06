.SUFFIXES: .c .o

TARGET = symlink

all: $(TARGET)

$(TARGET): symlink.o
	$(CC) $(CFLAGS) -o $(TARGET) symlink.c

clean:
	-rm -f *.o $(TARGET)
