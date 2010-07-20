UNAME := $(shell uname)
CFLAGS = -Wall

SOURCES=$(shell ls *.c)
OBJECTS=$(SOURCES:.c=.o)

ifeq ($(UNAME), Linux)
LDFLAGS = -lgsl -lgslcblas
INCDIRS =
endif

ifeq ($(UNAME), Darwin)
LDFLAGS = -L/opt/local/lib -lgsl
INCDIRS = -I/opt/local/include
endif

CFLAGS += ${LDFLAGS} ${INCDIRS}


all: wildmac_nowrap

wildmac_nowrap: $(OBJECTS)
	${CC} -o $@ wildmac_sim.o no_wrap.o ${CFLAGS}

%.o: %.c
	${CC} -c $< ${CFLAGS}

clean:
	rm -f ruth_sim uconnect_sim

