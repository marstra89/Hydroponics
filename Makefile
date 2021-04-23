  
CC=gcc
CFLAGS=-I.
DEPS = 
OBJ = Hydroponics
EXTRA_LIBS=-lwiringPi

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Hydroponics: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(EXTRA_LIBS)

.PHONY: clean

clean:
	rm -f Hydroponics $(OBJ) 
  
