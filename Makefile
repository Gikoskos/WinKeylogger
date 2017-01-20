CC = gcc
CFLAGS = -static-libgcc -std=c99 -Wall
LINKER =
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
EXE = WinKeylogger.exe


$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LINKER)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	@del -f *.o $(EXE)
