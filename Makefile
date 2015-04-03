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

run_det: wildmac
	./wildmac 2000000 20250 250 49 > cdf_hp_20ms_1.0_sim.data

run_wildmac: wildmac
	./wildmac 2000000 20860 250 25 > cdf_hp_20ms_0.5_sim.data
	./wildmac 2000000 20180 250 31 > cdf_hp_20ms_0.6_sim.data
	./wildmac 2000000 20660 250 36 > cdf_hp_20ms_0.7_sim.data
	./wildmac 2000000 20610 250 42 > cdf_hp_20ms_0.8_sim.data
	./wildmac 1000000 20880 250 15 > cdf_hp_20ms_0.9_sim.data

run_tmote: wildmac
	./wildmac 2000000 86000 1000 6 > cdf_hp_20ms_0.5_tmote.data
	./wildmac 2000000 90000 1000 7 > cdf_hp_20ms_0.6_tmote.data
	./wildmac 2000000 106000 1000 7 > cdf_hp_20ms_0.7_tmote.data
	./wildmac 2000000 109000 1000 8 > cdf_hp_20ms_0.8_tmote.data
	./wildmac 1000000 64000 1000 5 > cdf_hp_20ms_0.9_tmote.data


cdf_hp_20ms_sim.pdf: run_wildmac
	gnuplot cdf_hp_20ms_sim.gp
	epstopdf cdf_hp_20ms_sim.eps

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

