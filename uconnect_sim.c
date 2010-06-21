#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include <sys/time.h>
#include <gsl/gsl_rng.h>

#define TRUE 1
#define FALSE 0
#define SEGMENT_TIME 500					//Length in microseconds of each cell of the array that contains the CDF values
#define TIMES 100							//Used to define the size of the array in base to the epoch size, which will be multiplied by TIMES
#define N_HYPERSLOTS 5						//Number of hyperslots stored in the array that contains the CDF values
#define H_DELAY 0							//How many hyperslots must run before we start to perform detection
#define CCA 250								//Length of the CCA check, which defines the size of the U-connect slot as well

struct t_conf {
	unsigned long n_sensors;				//Number of sensors
	unsigned long n_number_iterations;		//Number of iterations
	unsigned long p;						//Prime number that defines the behaviour of U-Connect
	unsigned long min_num_det;				//Minimum number to accomplish per iteration
	int contact;							//How a node detect the present of another. 0: A listening slot must overlap with a beaconing slot, 1: Two active slots must overlap. It does not matter if is a beacon or listening slot.
	int phase;								//Sets the phase between nodes. 0: no phase, 1: continous phase, 2:discrete phase
};

gsl_rng *r;
char *hyperslot;							//Contains the node behaviour for any hyperslot, which will be similar for all nodes
struct t_conf sensor_conf;					//Stores the information related to the current experiment
unsigned long hyperslot_size;				//Hyperslot size, measured in slot
unsigned long *delay;						//Stores the phase of each node
unsigned long *cdf;							//Stores the CDF values related to the detection between nodes


/*
 * Initializes the seed used by the random number generator
 */

unsigned long int getSeed(void)
{
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	return cur_time.tv_usec;
}


/*
 * Initalizes the data structures that will remain unchanged through all the experiment
 */

void initData(void)
{
	unsigned long slot;
	
	delay = (unsigned long *) malloc (sizeof(unsigned long)*sensor_conf.n_sensors);
	hyperslot = (char *)malloc(N_HYPERSLOTS * hyperslot_size * sizeof(char));
	memset(hyperslot,0, sizeof(char)*(N_HYPERSLOTS*hyperslot_size));
		
	cdf = (unsigned long *) malloc (sizeof(unsigned long)*(N_HYPERSLOTS*hyperslot_size));
	memset(cdf,0, sizeof(unsigned long)*(N_HYPERSLOTS *hyperslot_size));
	
	for (slot = 0; slot < (N_HYPERSLOTS*hyperslot_size); slot++)
	{
		if ( (0<= (slot % hyperslot_size)) && ((slot % hyperslot_size) < ((sensor_conf.p+1)/2) ) )	//Assign the activity state in each slot based in U-connect behviour
			hyperslot[slot] = 'B';
		else if ((slot % sensor_conf.p) == 0 ) 
			hyperslot[slot] = 'L';
		else 
			hyperslot[slot] = '-';
	}
}

/*
 * Assing the phase value for each one of the nodes. The delay ranges between [0, hyperslot_length]. Here is assigned a continous value. In case the phase has been set as discrete, the discrete value will be calculated in the detection process
 */

void initPhase(void)
{
	int i;
	
	memset(delay,0, sensor_conf.n_sensors * sizeof(unsigned long));
	
	if (sensor_conf.phase == FALSE)
		return;
	
	for (i = 1; i < sensor_conf.n_sensors; i++ )
		delay[i] = (((double) gsl_rng_uniform (r) / 1 ) * (hyperslot_size * CCA));
}


/*
 *
 * Checks the detection state between the nodes being executed
 * Returns the slot the contact is made. If no contact is made, it returns ULONG_MAX
 * 
 */

unsigned long checkDetection(void)
{
	unsigned long slot, aux_slot;
	unsigned long time, in_phase;
	int slot_delay;
	
	time = delay[1];
	in_phase = time % CCA;		//Phase inside a single slot, used when the phase is continous
	slot_delay = delay[1]/CCA;	//Phase measured in slots.
	
	for (slot = (hyperslot_size*H_DELAY); slot < (N_HYPERSLOTS*hyperslot_size); slot++)
	{
		if (slot < slot_delay)
			continue;
		
		aux_slot = slot - slot_delay;
		
		if (aux_slot >= (N_HYPERSLOTS*hyperslot_size)) 
			return ULONG_MAX;
		
		if (aux_slot == 0)
			aux_slot=hyperslot_size;
		
		if (sensor_conf.contact == FALSE)
		{
			if (((hyperslot[slot] == 'B') && (hyperslot[aux_slot] == 'L')) ||  ((hyperslot[slot] == 'L') && (hyperslot[aux_slot] == 'B')))
			{
				if ((in_phase == 0) || (sensor_conf.phase==2)) 
					return slot - (hyperslot_size*H_DELAY);
			
				if (hyperslot[slot] == 'B')
				{
					if(hyperslot[slot+1] == 'B')
						return slot - (hyperslot_size*H_DELAY);
				}
				else
				{
					if(hyperslot[aux_slot-1] == 'B')
						return slot - (hyperslot_size*H_DELAY);
				}
			}
		}
		else if ((hyperslot[slot] != '-') && (hyperslot[aux_slot] != '-'))
		{
			if ((in_phase == 0) || (sensor_conf.phase==2))
				return slot - (hyperslot_size*H_DELAY);
			
			if ((hyperslot[slot+1] != '-') || (hyperslot[aux_slot-1] != '-')) 
				return slot - (hyperslot_size*H_DELAY);
		}
	}
	return ULONG_MAX;
}


int main(int argc, char** argv)
{
	unsigned long slot, iteration;
	
	if (argc != 7) {
		printf("ERROR:\n");
		exit(1);
	}
	
	sensor_conf.n_sensors = atoi(argv[1]);
	sensor_conf.n_number_iterations = atoi(argv[2]);
	sensor_conf.p = atof(argv[3]);
	sensor_conf.min_num_det = atof(argv[4]);
	sensor_conf.contact = atoi(argv[5]);
	sensor_conf.phase = atoi(argv[6]);
	
	hyperslot_size = sensor_conf.p * sensor_conf.p;
	r = gsl_rng_alloc (gsl_rng_ranlxs2);
	gsl_rng_set (r, getSeed());
	initData();

	for (iteration=0; iteration < sensor_conf.n_number_iterations ; iteration++) 
	{
		initPhase();
		slot = checkDetection();
			
		if (slot != ULONG_MAX)
			cdf[slot]++;
	}
	
	for (slot=1; slot < (2*hyperslot_size); slot++)
			cdf[slot] = cdf[slot] + cdf[slot-1];
	
	printf("0 0\n");
	for (slot=0; slot < (2*hyperslot_size) ; slot++) 
		printf("%lu %f\n", (slot + 1) * CCA, (double) cdf[slot]/sensor_conf.n_number_iterations );
	
	 exit(0);
}
