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


all: wildmac_nowrap wildmac_wrap wildmac_delay

wildmac_nowrap: $(OBJECTS)
	${CC} -o $@ wildmac_sim.o no_wrap.o ${CFLAGS}

wildmac_wrap: $(OBJECTS)
	${CC} -o $@ wildmac_sim.o wrap.o ${CFLAGS}

wildmac_delay: $(OBJECTS)
	${CC} -o $@ wildmac_sim.o delay_next.o ${CFLAGS}

%.o: %.c
	${CC} -c $< ${CFLAGS}

run_simulation: all
	./wildmac_nowrap > no_wrap.data
	./wildmac_wrap > wrap.data
	./wildmac_delay > delay.data
	gnuplot cdf.gp
	epstopdf cdf.eps
	rm cdf.eps

clean:
	rm -f wildmac_nowrap *.o 

