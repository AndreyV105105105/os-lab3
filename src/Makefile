CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -D_WIN32_WINNT=0x0A00
OBJS    = main.o gauss.o matrix.o bench.o common.o

ifeq ($(OS),Windows_NT)
    RM = del /Q /F
else
    RM = rm -f
endif

all: gauss.exe

gauss.exe: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c common.h matrix.h gauss.h bench.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-$(RM) *.o *.exe

.PHONY: all clean
