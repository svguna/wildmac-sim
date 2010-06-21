linux:

	gcc -o ruth_sim ruth_simulator.c -Wall -lgsl -lgslcblas
	gcc -o uconnect_sim uconnect_sim.c -Wall -lgsl -lgslcblas

mac:

	gcc -o ruth_sim ruth_simulator.c -Wall -I/opt/local/include -L/opt/local/lib -lgsl
	gcc -o uconnect_sim uconnect_sim.c -Wall -I/opt/local/include -L/opt/local/lib -lgsl

clean:

	rm -f ruth_sim
	rm -f uconnect_sim

