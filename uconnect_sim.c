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
 * Returns the state of the slot provided, identified by its index. It is calculated as it is stated in the U-connect article
 * The Beaconing state is identified by 'B', the Listening state by 'L' and the idle state by '-'.
 *
 */

char getSlotState(unsigned long current_slot)
{

	char state;

	if ( (0<= (current_slot % hyperslot_size)) && ((current_slot % hyperslot_size) < ((sensor_conf.p+1)/2) ) )
		state = 'B';
	else if ((current_slot % sensor_conf.p) == 0 ) 
		state = 'L';
	else 
		state = '-';
	
	return state;
}


/*
 * Initalizes the data structures that will remain unchanged through all the experiment.
 */

void initData(void)
{
	delay = (unsigned long *) malloc (sizeof(unsigned long)*sensor_conf.n_sensors);
	cdf = (unsigned long *) malloc (sizeof(unsigned long)*(N_HYPERSLOTS*hyperslot_size));
	memset(cdf,0, sizeof(unsigned long)*(N_HYPERSLOTS *hyperslot_size));	
}


/*
 * Assings the phase value for each one of the nodes. The delay ranges between [0, hyperslot_length]. Here is assigned a continous value. In case the phase has been set as discrete, the discrete value will be calculated in the detection process
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
	unsigned long slot_nodeA, slot_nodeB;
	unsigned long time, in_phase;
	int slot_delay;
	
	time = delay[1];													//Phase of node B
	in_phase = time % CCA;												//Phase inside a single slot, used when the phase is continous
	slot_delay = delay[1]/CCA;											//Discrete value of the phase.
	
	for (slot_nodeA = (hyperslot_size*H_DELAY); slot_nodeA < ((N_HYPERSLOTS*hyperslot_size)+(hyperslot_size*H_DELAY)); slot_nodeA++)
	{
		if (slot_nodeA < (slot_delay+(hyperslot_size*H_DELAY)))			//The slots that do not overlap with the other node slots are not taken into account
			continue;
		
		slot_nodeB = slot_nodeA - slot_delay;							//Slot value after applying the phase
		
	
		if (slot_nodeB >= ((N_HYPERSLOTS*hyperslot_size)+(hyperslot_size*H_DELAY)))				//If were are out of the maximum amount of defined hyperslots
			return ULONG_MAX;
		
		if (slot_nodeB == 0)											//If it is the first slot, we move to the equivalent one in the next slot, in case we need to analyze the values from previous slots
			slot_nodeB=hyperslot_size;
		
		if (sensor_conf.contact == FALSE)								//If the detection is made when a Beacon slot from node A overlaps with a Listen slot from node B
		{
			if (((getSlotState(slot_nodeA) == 'B') && (getSlotState(slot_nodeB) == 'L')) ||  ((getSlotState(slot_nodeA) == 'L') && (getSlotState(slot_nodeB) == 'B')))
			{
				if ((in_phase == 0) || (sensor_conf.phase==2))			//In case there is no phase between the nodes, or the phase is discrete
						return slot_nodeA - (hyperslot_size*H_DELAY);
				
				if (getSlotState(slot_nodeA) == 'B')								
				{
					if(getSlotState(slot_nodeA + 1) == 'B')						//In case the phase is continous, and there is a positive value for the in_phase variable, there must a be an additional beacon slot following the current one to guarantee the detection
						return slot_nodeA - (hyperslot_size*H_DELAY);
				}
				else
				{
					if(getSlotState(slot_nodeB - 1) == 'B')						//In case the phase is continous, and there is a positive value for the in_phase variable, there must a be an additional beacon slot before the current one  to guarantee the detection
						return slot_nodeA - (hyperslot_size*H_DELAY);
				}
			}
		}
		else if ((getSlotState(slot_nodeA) != '-') && (getSlotState(slot_nodeB) != '-'))		//If the detection is made simply when two active slots overlap
		{
			if ((in_phase == 0) || (sensor_conf.phase==2))								//In case there is no phase between the nodes, or the phase is discrete				
				return slot_nodeA - (hyperslot_size*H_DELAY);
				
			if ((getSlotState(slot_nodeA + 1) != '-') || (getSlotState(slot_nodeB - 1) != '-'))	//In case there is a positive value for the in_phase variable, there must be two consecutive active slots
				return slot_nodeA - (hyperslot_size*H_DELAY);
		}
	}
	return ULONG_MAX;
}


int main(int argc, char** argv)
{
	unsigned long slot, iteration;
	
	if (argc != 7) {
		printf("ERROR: Wrong number of parameters\n");
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
