LDLIBS		= -lm -lpthread
COMMONCFLAGS	= -DREENTERANT -O3 -g3 -Wall -c -fmessage-length=0
WINFLAGS	= -D__CLEANUP_C -DPTW32_STATIC_LIB
CFLAGS		= $(COMMONCFLAGS)

ifdef WINDIR
	CFLAGS = $(COMMONCFLAGS) $(WINFLAGS)
endif

all: primes

primes: main_mt.o primes.o
	$(CC) main_mt.o primes.o -o $@ $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o$@

clean:
	rm *.o primes primes.exe
