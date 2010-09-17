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


all: wildmac wildmac_nowrap wildmac_wrap wildmac_delay wildmac_split_nowrap wildmac_rsplit_nowrap wildmac_split_wrap wildmac_rsplit_wrap wildmac_split_delay wildmac_rsplit_delay

wildmac: wildmac_sim_phase.o default.o
	${CC} -o $@ $^ ${CFLAGS}

wildmac_nowrap: wildmac_sim.o no_wrap.o 
	${CC} -o $@ $^ ${CFLAGS}

wildmac_wrap: wildmac_sim.o wrap.o 
	${CC} -o $@ $^ ${CFLAGS}

wildmac_delay: wildmac_sim.o delay_next.o 
	${CC} -o $@ $^ ${CFLAGS}

wildmac_split_nowrap: wildmac_sim_split.o no_wrap_split.o 
	${CC} -o $@ $^ ${CFLAGS}

wildmac_rsplit_nowrap: wildmac_sim_rsplit.o no_wrap_split.o 
	${CC} -o $@ $^ ${CFLAGS}

wildmac_split_wrap: wildmac_sim_split.o wrap_split.o 
	${CC} -o $@ $^ ${CFLAGS}

wildmac_rsplit_wrap: wildmac_sim_rsplit.o wrap_split.o 
	${CC} -o $@ $^ ${CFLAGS}

wildmac_split_delay: wildmac_sim_split.o delay_next_split.o 
	${CC} -o $@ $^ ${CFLAGS}

wildmac_rsplit_delay: wildmac_sim_rsplit.o delay_next_split.o 
	${CC} -o $@ $^ ${CFLAGS}

%.o: %.c
	${CC} -c $< ${CFLAGS}

run_wildmac: wildmac
	./wildmac 1000000 11460 250 42 > wildmac.data
	gnuplot cdf-wildmac.gp
	epstopdf cdf-wildmac.eps
	rm cdf-wildmac.eps

run_all_simulation: all
	./wildmac_nowrap > no_wrap.data
	./wildmac_wrap > wrap.data
	./wildmac_delay > delay.data
	./wildmac_split_nowrap > no_wrap_split.data
	./wildmac_rsplit_nowrap > no_wrap_rsplit.data
	./wildmac_split_wrap > wrap_split.data
	./wildmac_rsplit_wrap > wrap_rsplit.data
	./wildmac_split_delay > delay_split.data
	./wildmac_rsplit_delay > delay_rsplit.data
	gnuplot cdf.gp
	epstopdf cdf.eps
	rm cdf.eps

clean:
	rm -f wildmac_nowrap wildmac_wrap wildmac_delay wildmac_split_nowrap wildmac_rsplit_nowrap wildmac_split_wrap wildmac_rsplit_wrap wildmac_split_delay wildmac_rsplit_delay *.o 

