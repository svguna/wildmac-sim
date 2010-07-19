UNAME := $(shell uname)
CFLAGS = -Wall


ifeq ($(UNAME), Linux)
LDFLAGS = -lgsl -lgslcblas
INCDIRS =
endif

ifeq ($(UNAME), Darwin)
LDFLAGS = -L/opt/local/lib -lgsl
INCDIRS = -I/opt/local/include
endif

CFLAGS += ${LDFLAGS} ${INCDIRS}


all:
	${CC} -o ruth_sim ruth_simulator.c ${CFLAGS}
	${CC} -o uconnect_sim uconnect_sim.c ${CFLAGS}

clean:
	rm -f ruth_sim uconnect_sim

