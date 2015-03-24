all: primes

LDLIBS		= -lm -lpthread
CFLAGS		= -DREENTERANT -O3 -g3 -Wall -c -fmessage-length=0

primes: main_mt.o primes.o
	$(CC) main_mt.o primes.o -o $@ $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o$@

clean:
	rm *.o primes
