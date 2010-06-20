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
#define METRIC 0			// How to calculate the detection values. 0: Gives the instant of time for the first detection, 1: Gives the distance between two consecutive detections
#define EPOCH_DELAY 5		//Number of epochs that pass before starting the detection. The state from previous epoch is taking into account to calculate the detection
#define TIME_DELAY 0		//If set, for each iteration, a random value between [0, Epoch] is defined. The detection starts after this time has passed
#define MAX_EPOCHS 100		//Maximum number of epochs per iteration
#define MAX_ITERATIONS 100	//Maximumm number of iterations in certain iterative proccesses
#define SEGMENT_TIME 250	//Length in microseconds of each cell of the array that contains the CDF values
#define N_EPOCHS_SAVED 20	//Number of epochs stored in the array that contains the CDF values
#define CCA_POSITION 0		//Defines, for LPL, when the CCA check is made in each slot of size "beacon". 0: the CCA is made at the begining, 1 the CCA is made at the end
#define CCA_LENGTH 250		//Length of the CCA check

/*
 *Stores the information related to the current experiment
 */
struct t_conf {
	unsigned long n_sensors;					//Number of sensors
	unsigned long n_number_iterations;			//Number of iterations
	unsigned long epoch_t;						//Epoch length
	unsigned long beacon_t;						//Beacon length
	unsigned long listen_t;						//Listen length
	unsigned long min_num_det;					//Minimum number to accomplish per iteration
	int divide_actions;							//How the beacon and listen activities are organized. 0: Coupled (Beacon+Listen), 1: Decoupled No specific order, 2: Coupled (Listen+Beacon)
	int phase;									//Sets the phase between nodes. 0: No phase. 1: Phase defined between [0,T]
	int active_phase;							//Defines how the beacon and listen are generated. 0: The active phase starts in the interval [0, T-(L+S)] for each epoch, 1: the active phase starts in the interval [0,T] being possible to overlap with the next epoch, 2: Active phase starts within [0,T], but contained in the epoch, the overlapping part of the listen is set at the begining of the epoch
};


gsl_rng *r;										//Global variable used by the random number generator
struct t_conf experiment_conf;					//Stores the information related to the current experiment
unsigned long *delay;							//Stores the phase of each node
unsigned long *beacon_matrix;					//Stores the instant of time, relative to the current epoch, each node starts beaconing
unsigned long *listen_matrix;					//Stores the instant of time, relative to the current epoch, the node starts listening
unsigned long *listen_matrix_extra;				//Additional array that stores the listening times when they are divided inside the epoch
unsigned long *previous_beacon_matrix;			//Stores the instant of time, relative to the previous epoch, each node starts beaconing
unsigned long *previous_listen_matrix;			//Stores the instant of time, relative to the previous epoch, each node starts listening
unsigned long *previous_listen_matrix_extra;	//Additional array that stores the listening times when they are divided inside the epoch
char **detection_control;						//Stores the detection events for the current iteration.
unsigned long *cdf;								//Stores the CDF values related to the detection between nodes
unsigned long instant_contact;					//Stores the instant a detection is made
unsigned long detection_delay;					//In case TIME_DELAY is set to 1, it stores the time must pass before allowing the detection


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
	int i;

	delay = (unsigned long *) malloc (sizeof(unsigned long)*experiment_conf.n_sensors);
	detection_control = (char **)malloc(experiment_conf.n_sensors * (sizeof(char *)));
	for (i = 0; i < experiment_conf.n_sensors; i++ )
		detection_control[i] = (char *)malloc(experiment_conf.n_sensors * sizeof(char));

	cdf = (unsigned long *) malloc (sizeof(unsigned long)*((experiment_conf.epoch_t*N_EPOCHS_SAVED)/ SEGMENT_TIME ));
	memset(cdf,0, sizeof(unsigned long)*((experiment_conf.epoch_t*N_EPOCHS_SAVED)/ SEGMENT_TIME ));

}

/*
 * Initalizes the data structures that must be generated again for each iteration
 */

void initDataIteration(void)
{
	int i,j;

	for (i = 0; i < experiment_conf.n_sensors; i++ )
		for (j = 0; j < experiment_conf.n_sensors; j++ )
			detection_control[i][j] = '0';

	for (i = 0; i < experiment_conf.n_sensors; i++ )
	{
		if (experiment_conf.phase == FALSE)
			delay[i] = 0;
		else
		{
			//In case the nodes have a phase between them, the node 0 will have a phase value of 0
			if(i == 0) 
				delay[i] = 0;
			else
				delay[i] = (((double) gsl_rng_uniform (r) / 1 ) * (experiment_conf.epoch_t));
		}
	}

	if (TIME_DELAY == TRUE)
		detection_delay = (((double) gsl_rng_uniform (r) / 1 ) * (experiment_conf.epoch_t));
	else
		detection_delay = 0;
	
	instant_contact = 0;
	
}

/*
 * This functions returns the amount of time two segments overlap.
 *
 * init1: Instant of time the first segment starts
 * dur1:  Length of the first segment
 * init2: Instant of time the second segment starts
 * dur2:  Length of the second segment
 *
 */

unsigned long overlapCheck(unsigned long init1, unsigned long dur1, unsigned long init2, unsigned long dur2)
{
	unsigned long aux;

	if ( (init1 == ULONG_MAX) || (init2 == ULONG_MAX))
		return ULONG_MAX;

	//The segments are ordered, granting init2 >= init1
	if (init1>init2) {
		aux = init1;
		init1 = init2;
		init2 = aux;
		aux= dur1;
		dur1 = dur2;
		dur2 = aux;
	}
	
	if ((((init1+dur1) >= init2) && ((init1+dur1) <= (init2+dur2))) ||  (((init2+dur2) >= init1) && ((init2) <= (init1+dur1))))
	{
		if ( (init1+dur1) >= (init2+dur2) )
			aux = dur2;
		else
			aux = (init1+dur1) - init2;
	
		return aux;
	} 
	else
		return ULONG_MAX;
}

/*
 * Returns True if the number of detections made during the current iteration is equal or higher than the minimum required
 */

int checkDetectionState(void)
{
	unsigned long i,j, detections;

	for (i=0; i<experiment_conf.n_sensors; i++) {
		detections = 0;
		for (j=0; j<experiment_conf.n_sensors; j++) {

			if (detection_control[i][j] == '1' )
				detections++;

			if (detections >= experiment_conf.min_num_det)
				return TRUE;
		}
	}
	return FALSE;
}


/*
 * It checks the detection between two nodes, simulating the LPL behaviour. It performs a serie of CCA checks during the listening time, in intervals of length 
 * equal the beaconing length. 
 *
 * Beacon: Instant of time node A starts beaconing.
 * Listen: Instant of time node B starts listening.
 * Listen_length: Length of the listening activity of node B.
 * Epoch: Value of the current epoch.
 *
 */

int checkCcaOverlap(unsigned long beacon, unsigned long listen, unsigned long listen_length, int epoch)
{
	unsigned long ret, abs_cca_start, cca_start;
	int start, end, i, beacon_start_phase;
	
	if (listen_length < CCA_LENGTH)
		return FALSE;
	
	//If the available space only allows one CCA sampling, it is set at the begining or the end of the available time, depending when the CCA must be made
	if (listen_length < experiment_conf.beacon_t)
	{
		start= 0;
		end = 0;
		if (CCA_POSITION == 0)
			beacon_start_phase = 0;
		else
			beacon_start_phase = listen_length - CCA_LENGTH;
	}
	else
	{
		if (CCA_POSITION == 0)
		{
			start=0;
			end=(listen_length/experiment_conf.beacon_t)-1;
			beacon_start_phase=0;
		}
		else 
		{
			start=1;
			end=listen_length/experiment_conf.beacon_t;
			beacon_start_phase=-CCA_LENGTH;
		}
	}
		
	for (i=start; i<=end; i++) 
	{
		cca_start = listen+(experiment_conf.beacon_t * i)+beacon_start_phase;				//Calculate the instant of time the CCA check is made
		ret = overlapCheck(beacon, experiment_conf.beacon_t, cca_start, CCA_LENGTH);			//Check if the CCA check overlaps with the beacon
		if (ret == CCA_LENGTH)
		{
			abs_cca_start = cca_start + ((epoch - EPOCH_DELAY) * experiment_conf.epoch_t);	//Calculate the instant, in absolute time, of the detection
			if (abs_cca_start >= detection_delay)											//If the instant of detection is bigger than the detection delay, it is possible to register the detection
			{
				if ((instant_contact == 0) || (abs_cca_start < instant_contact))			//In case no contact has been made, or it is made before the one which is registered, it is stored.
					instant_contact = abs_cca_start; 
				return TRUE;
			}
		}
	}
	
	return FALSE;
}


/*
 * Returns the length of the listening activity in the current epoch for a specific node
 *
 * sensor: Current node
 * extra_listen: Defines the listen matrix being analized. FALSE: the standard listen matrix, TRUE: the additional listen matrix when the listening activity last more than the end of the epoch 
 * l: Listening matrix
 *
 */

unsigned long calcListenLength(int sensor, int extra_listen, unsigned long *l)
{
	if (experiment_conf.active_phase != 2)	//If the experiment configuration guarantees the listening activity will not be divided
		return experiment_conf.listen_t;
	else 
	{
		if (extra_listen == FALSE)			//If we are analyzing the standard listen matrix
		{
			if ( ((experiment_conf.epoch_t + delay[sensor]) - (l[sensor])) >= experiment_conf.listen_t)	//If the listen activity is contained within the epoch
				return experiment_conf.listen_t;
			else
				return ( (experiment_conf.epoch_t + delay[sensor]) - (l[sensor]));
				
		}
		else
			return ( (l[sensor] + experiment_conf.listen_t) - (experiment_conf.epoch_t + delay[sensor])  );
	}
}


/*
 * This functions checks, for the current and previous epochs, if a contact has been made between two nodes
 *
 * sensor: Index of the sensor which is being checked
 * extra_listen: It indicates which listen matrix has to be used. FALSE: standard listen matrix, TRUE: the additional listen matrix when the listening activity last more than the end of the epoch 
 * Epoch: Value of the current epoch.
 *
 */

void checkDetection(int sensor, int extra_listen, int epoch)
{	
	unsigned long listen, beacon, listen_length;
	unsigned long *prev_l, *current_l;
	int i,ret;
	
	if (extra_listen == FALSE) 
	{
		prev_l = previous_listen_matrix;
		current_l = listen_matrix;
	}
	else 
	{
		prev_l = previous_listen_matrix_extra;
		current_l = listen_matrix_extra;
	}

	
	//Checking if the listen activity of the current epoch overlaps with the beaconing of other node in the previous epoch
	if ( (previous_beacon_matrix != NULL) && (current_l[sensor] != ULONG_MAX))
	{
		listen = current_l[sensor] + experiment_conf.epoch_t;
		listen_length = calcListenLength(sensor, extra_listen, listen_matrix);
		for (i=0; i<experiment_conf.n_sensors; i++)			
			if (sensor != i)
			{
				beacon = previous_beacon_matrix[i];
				ret = checkCcaOverlap(beacon, listen, listen_length, epoch-1);
				if(ret == TRUE)
					detection_control[sensor][i] = '1';
				
			}
	}
	
	//Checking if the listen activity of the previous epoch overlaps with the beaconing of other node in the current epoch
	if ((prev_l != NULL) && (prev_l[sensor] !=  ULONG_MAX ))
	{
		listen = prev_l[sensor];
		listen_length = calcListenLength(sensor, extra_listen, previous_listen_matrix);
		for (i=0; i<experiment_conf.n_sensors; i++)			
			if (sensor != i)
			{
				beacon = beacon_matrix[i] + experiment_conf.epoch_t;
				ret = checkCcaOverlap(beacon, listen, listen_length, epoch - 1 );
				if(ret == TRUE)
					detection_control[sensor][i] = '1';
			}
	}
	
	//Checking if the listen activity of the current epoch overlaps with the beaconing of other node in the current epoch
	if (current_l[sensor] != ULONG_MAX)
	{
		listen = current_l[sensor];
		listen_length = calcListenLength(sensor, extra_listen, listen_matrix);
		for (i=0; i<experiment_conf.n_sensors; i++)			
			if (sensor != i)
			{
				beacon = beacon_matrix[i];
				ret = checkCcaOverlap(beacon, listen, listen_length, epoch);
				if(ret == TRUE)
					detection_control[sensor][i] = '1';
			}
	}
}


/*
 * Generates the Beacon and Listen values (in this order) for a specific sensor and the current epoch.
 *
 * sensor: Current sensor index
 * init_time: Lower bound within the epoch it is possible to start generating events
 * upper_limit: Upper bound within the epoch it is possible to start generating events
 * current_epoch: Current epoch value
 *
 */

void generateBeaconListenData(int sensor, unsigned long init_time, unsigned long upper_limit, int current_epoch)
{
	
		beacon_matrix[sensor] = init_time + (((double) gsl_rng_uniform (r) / 1 ) * (experiment_conf.epoch_t -init_time - upper_limit));
		beacon_matrix[sensor] = beacon_matrix[sensor] + delay[sensor];
	
		if ((beacon_matrix[sensor] + experiment_conf.beacon_t + experiment_conf.listen_t ) <= (experiment_conf.epoch_t + delay[sensor])) //if the listen activity does not overlap with the next epoch
			listen_matrix[sensor] = beacon_matrix[sensor] + experiment_conf.beacon_t;
		else
		{
			if ( experiment_conf.active_phase == FALSE )
				listen_matrix[sensor] = ULONG_MAX;
			else																			//if the listen activity overlaps with the next epoch, but it is allowed
				listen_matrix[sensor] = beacon_matrix[sensor] + experiment_conf.beacon_t;
		}
}


/*
 * Generates the Beacon and Listen values decoupled, and with no specific order, for a specific sensor and the current epoch. It first generates the value for the listen even and after for the beacon
 *
 * sensor: Current sensor index
 * init_time: Lower bound within the epoch it is possible to start generating events
 * upper_limit: Upper bound within the epoch it is possible to start generating events
 * current_epoch: Current epoch value
 *
 */

void generateBeaconListenDecoupled(int sensor, unsigned long init_time, unsigned long upper_limit, int current_epoch)
{
	unsigned long iterations, init_listen, end_listen;
	int regenerate;
	
	do
	{
		iterations = 0;
		do
		{
			if (iterations >=MAX_ITERATIONS) //Maximum amount of iterations to avoid an infinity loop
				break;
		
			regenerate=FALSE;
			listen_matrix[sensor] = init_time + (((double) gsl_rng_uniform (r) / 1 ) * (experiment_conf.epoch_t -init_time - upper_limit));
			if(upper_limit!=0) //If it is not allowed to overlap with the next epoch
			{
				//Checks if there is place left to generate the beacon
				if ( (listen_matrix[sensor] < experiment_conf.beacon_t) && ( ((experiment_conf.epoch_t - (listen_matrix[sensor] + experiment_conf.listen_t)) < experiment_conf.beacon_t )))
					regenerate = TRUE;
			}
			iterations++;
		}while(regenerate);

		if (iterations >=MAX_ITERATIONS) //In case we exceeded the maximum amount of iterations, we start again the process
			continue;
	
		listen_matrix[sensor] = listen_matrix[sensor] + delay[sensor];
	
		//Gets the lower and upper bound where the beacon can be generated
		if (listen_matrix[sensor] < experiment_conf.beacon_t)
			init_listen = listen_matrix[sensor] + experiment_conf.listen_t;
		else 
			init_listen = delay[sensor] + init_time;
	
		if (upper_limit != 0)
		{
			if (((experiment_conf.epoch_t + delay[sensor]) - (listen_matrix[sensor] + experiment_conf.listen_t)) < experiment_conf.beacon_t ) 
				end_listen = listen_matrix[sensor] - 1; 
			else
				end_listen = (delay[sensor] + experiment_conf.epoch_t) - experiment_conf.beacon_t; 
		}
		else
		{
			if ((listen_matrix[sensor] + experiment_conf.listen_t) > (experiment_conf.epoch_t + delay[sensor]))
				end_listen = listen_matrix[sensor] - 1;
			else
				end_listen =  experiment_conf.epoch_t + delay[sensor];
		}

		iterations = 0;
		
		do
		{
			if (iterations >=MAX_ITERATIONS)
				break;
			regenerate=FALSE;
			beacon_matrix[sensor] = init_listen + (((double) gsl_rng_uniform (r) / 1 ) * (end_listen-init_listen));
			//We check the beacon and listen do not overlap
			if (overlapCheck(beacon_matrix[sensor], experiment_conf.beacon_t, listen_matrix[sensor], experiment_conf.listen_t) != ULONG_MAX)
				regenerate = TRUE;
			iterations++;
		}while(regenerate);
	}while ((iterations >= MAX_ITERATIONS));
}


/*
 * Generates the Listen and Beacon values (in this order) for a specific sensor and the current epoch.
 *
 * sensor: Current sensor index
 * init_time: Lower bound within the epoch it is possible to start generating events
 * upper_limit: Upper bound within the epoch it is possible to start generating events
 * current_epoch: Current epoch value
 *
 */

void generateListenBeaconData(int sensor, unsigned long init_time, unsigned long upper_limit, int current_epoch)
{
	
	listen_matrix[sensor] = init_time + (((double) gsl_rng_uniform (r) / 1 ) * (experiment_conf.epoch_t -init_time - upper_limit));
	listen_matrix[sensor] = listen_matrix[sensor] + delay[sensor];
		
				
	if ((listen_matrix[sensor] + experiment_conf.listen_t + experiment_conf.beacon_t ) <= (experiment_conf.epoch_t + delay[sensor]))
		beacon_matrix[sensor] = listen_matrix[sensor] + experiment_conf.listen_t;
	else
	{
		if ( experiment_conf.active_phase == FALSE )
			beacon_matrix[sensor] = ULONG_MAX;
		else 
			beacon_matrix[sensor] = listen_matrix[sensor] + experiment_conf.listen_t;
			
	}
}


/*
 * Generates the Beacon and Listen values in the order specified by the experiment parameters. In case the listen activity overlaps with the next epoch, the overlapping segment goes to the begining of the epoch
 *
 * sensor: Current sensor index
 * init_time: Lower bound within the epoch it is possible to start generating events
 * upper_limit: Upper bound within the epoch it is possible to start generating events
 * current_epoch: Current epoch value
 *
 */

void generateActiveDataExtra(int sensor, unsigned long init_time, unsigned long upper_limit, int current_epoch)
{
	
	unsigned long extra_listen_length;
	
	listen_matrix[sensor] = init_time + (((double) gsl_rng_uniform (r) / 1 ) * (experiment_conf.epoch_t -init_time - upper_limit));
	listen_matrix[sensor] = listen_matrix[sensor] + delay[sensor];

	if ((listen_matrix[sensor] + experiment_conf.listen_t) > (experiment_conf.epoch_t + delay[sensor]))
	{
		listen_matrix_extra[sensor] = init_time + delay[sensor];
		extra_listen_length = (listen_matrix[sensor] + experiment_conf.listen_t)  - (experiment_conf.epoch_t + delay[sensor]);
	}
	else
		listen_matrix_extra[sensor] = ULONG_MAX;

	
	if (experiment_conf.divide_actions == 0) {
		beacon_matrix[sensor] = listen_matrix[sensor] - experiment_conf.beacon_t;
	}
	else if (experiment_conf.divide_actions == 2)
	{
		if (listen_matrix_extra[sensor] != ULONG_MAX)
			beacon_matrix[sensor] = listen_matrix_extra[sensor] + extra_listen_length;
		else {
			//if the beacon does not fit after the listen, is placed at the begining of the current epoch
			if ((listen_matrix[sensor] + experiment_conf.listen_t + experiment_conf.beacon_t) > (experiment_conf.epoch_t + delay[sensor]))
				beacon_matrix[sensor] = init_time + delay[sensor];
			else
				beacon_matrix[sensor] = listen_matrix[sensor] + experiment_conf.listen_t;
		}
	}
}


void freeMatrix(unsigned long *a, unsigned long *b, unsigned long *c)
{
	free(a);
	free(b);
	free(c);
}

int main(int argc, char** argv)
{
	int sensor, iteration, epoch, ret;
	unsigned long i, first_detection, second_detection;
	
	if (argc != 10) {
		printf("ERROR: Wrong number of parameters\nsimulation N_SENSORS NUMBER_ITERATIONS EPOCH_T BEACON_T LISTEN_T MIN_NUM_DETECT DIVIDE_ACTIONS PHASE ACTIVE_TIME - Time in microseconds\n");
		printf("argc %d\n", argc);
		exit(1);
	}

	experiment_conf.n_sensors = atoi(argv[1]);
	experiment_conf.n_number_iterations = atoi(argv[2]);
	experiment_conf.epoch_t = atof(argv[3]);
	experiment_conf.beacon_t = atof(argv[4]);
	experiment_conf.listen_t = atof(argv[5]);
	
	experiment_conf.min_num_det = atof(argv[6]);
	experiment_conf.divide_actions = atoi(argv[7]);
	experiment_conf.phase = atoi(argv[8]);
	experiment_conf.active_phase = atoi(argv[9]);

	
	if ( (experiment_conf.beacon_t > experiment_conf.listen_t) || (experiment_conf.epoch_t < (experiment_conf.listen_t + experiment_conf.beacon_t)) ) {
		printf("Times are not consistent...\n");
		printf("%lu %lu %lu %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d\n", experiment_conf.n_number_iterations, experiment_conf.n_sensors, experiment_conf.listen_t, experiment_conf.beacon_t, experiment_conf.min_num_det, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		exit(1);
	}

	if ( experiment_conf.min_num_det <= 0 ) {
		printf("Wrong definition of minimum detections...\n");
		printf("%lu %lu %lu %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d\n", experiment_conf.n_number_iterations, experiment_conf.n_sensors, experiment_conf.listen_t, experiment_conf.beacon_t, experiment_conf.min_num_det, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		exit(1);
	}

	if (experiment_conf.beacon_t < experiment_conf.beacon_t) {
		printf("ERROR: Beacon lenght not big enough to fit minimum detect sample\n");
		printf("%lu %lu %lu %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d\n", experiment_conf.n_number_iterations, experiment_conf.n_sensors, experiment_conf.listen_t, experiment_conf.beacon_t, experiment_conf.min_num_det, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		exit(1);
	}

	//Initialize the random number generator
	r = gsl_rng_alloc (gsl_rng_ranlxs2);
	gsl_rng_set (r, getSeed());

	initData();

	for (iteration=0; iteration < experiment_conf.n_number_iterations; iteration++) 
	{
		initDataIteration();
		first_detection = 0;
		
		for (epoch=0; epoch < MAX_EPOCHS; epoch++)
		{
			
			beacon_matrix = (unsigned long *)malloc(experiment_conf.n_sensors * sizeof(unsigned long));
			listen_matrix = (unsigned long *)malloc(experiment_conf.n_sensors * sizeof(unsigned long));
			listen_matrix_extra = (unsigned long *)malloc(experiment_conf.n_sensors * sizeof(unsigned long));
			
			for (sensor = 0; sensor < experiment_conf.n_sensors; sensor++ )
			{
				beacon_matrix[sensor] = ULONG_MAX;
				listen_matrix[sensor] = ULONG_MAX;
				listen_matrix_extra[sensor] = ULONG_MAX;
			
				//Generate the beacon and listen values for the current epoch for all the nodes
				if ( experiment_conf.active_phase == 2 )
				{
					
					switch (experiment_conf.divide_actions) 
					{
						case 0:
							generateActiveDataExtra(sensor, experiment_conf.beacon_t, 0, epoch);
							break;
						case 1:
							generateBeaconListenDecoupled(sensor, 0, experiment_conf.listen_t, epoch);
							break;
						case 2:
							generateActiveDataExtra(sensor, 0, 0, epoch);
							break;	
					}
				}
				else
				{	
			
					switch (experiment_conf.divide_actions) 
					{
						case 0:
							if ( experiment_conf.active_phase == FALSE )
								generateBeaconListenData(sensor, 0, experiment_conf.beacon_t+experiment_conf.listen_t, epoch);
							else 
							{
								if (previous_beacon_matrix == NULL) 
									generateBeaconListenData(sensor, 0, 0, epoch);
								else
								{
									if ((previous_listen_matrix[sensor] + experiment_conf.listen_t) > (experiment_conf.epoch_t + delay[sensor])) 
									{
										i = (previous_listen_matrix[sensor]+experiment_conf.listen_t - (experiment_conf.epoch_t + delay[sensor]) );
										generateBeaconListenData(sensor, i, 0, epoch);
									}
									else 
										generateBeaconListenData(sensor, 0, 0, epoch);
								}
							}
						break;
			
						case 1:
							if ( experiment_conf.active_phase == FALSE )
								generateBeaconListenDecoupled(sensor, 0, experiment_conf.listen_t, epoch);
							else 
							{
								if (previous_beacon_matrix == NULL)
									generateBeaconListenDecoupled(sensor, 0, 0, epoch);
								else
								{
									if ((previous_listen_matrix[sensor] + experiment_conf.listen_t) > (experiment_conf.epoch_t + delay[sensor])) 
									{
										i = (previous_listen_matrix[sensor]+experiment_conf.listen_t - (experiment_conf.epoch_t + delay[sensor]) );
										generateBeaconListenDecoupled(sensor, i, 0, epoch);
									}
									else if ((previous_beacon_matrix[sensor] + experiment_conf.beacon_t) > (experiment_conf.epoch_t + delay[sensor])) 
									{
										i = (previous_beacon_matrix[sensor]+experiment_conf.beacon_t - (experiment_conf.epoch_t + delay[sensor]) );
										generateBeaconListenDecoupled(sensor, i, 0, epoch);
									}
									else 
										generateBeaconListenDecoupled(sensor, 0, 0, epoch);
								}
							}
						break;
			
						case 2:
							if ( experiment_conf.active_phase == FALSE )
								generateListenBeaconData(sensor, 0, experiment_conf.beacon_t+experiment_conf.listen_t, epoch);
							else 
							{
								if (previous_beacon_matrix == NULL)
									generateListenBeaconData(sensor, 0, 0, epoch);
								else
								{
									if ((previous_beacon_matrix[sensor] + experiment_conf.beacon_t) > (experiment_conf.epoch_t + delay[sensor]))
									{
										i = (previous_beacon_matrix[sensor]+experiment_conf.beacon_t - (experiment_conf.epoch_t + delay[sensor]) );
										generateListenBeaconData(sensor, i, 0, epoch);
									}
									else 
										generateListenBeaconData(sensor, 0, 0, epoch);
								}
							}
							break;
					}
				}
			}
			
			
			//print's when the beacon and listen are generated decoupled, only shows for one node.
			/*
			printf("%lu 1 %lu\n",  beacon_matrix[0]+(epoch*experiment_conf.epoch_t) + (experiment_conf.beacon_t/2), experiment_conf.beacon_t);
			printf("%lu 1 %lu\n",  listen_matrix[0]+(epoch*experiment_conf.epoch_t) + (experiment_conf.listen_t/2), experiment_conf.listen_t);
			printf("%lu 2 %lu\n",  beacon_matrix[1]+(epoch*experiment_conf.epoch_t) + (experiment_conf.beacon_t/2), experiment_conf.beacon_t);
			printf("%lu 2 %lu\n",  listen_matrix[1]+(epoch*experiment_conf.epoch_t) + (experiment_conf.listen_t/2), experiment_conf.listen_t);
			*/
			
			//Calculate the detection between the nodes
			if (epoch >= EPOCH_DELAY) 
			{
				for (sensor=0; sensor < experiment_conf.n_sensors; sensor++)
				{
					checkDetection(sensor, FALSE, epoch);
					if ( experiment_conf.active_phase == 2 )
						checkDetection(sensor, TRUE, epoch);
				}
				ret = checkDetectionState();
			}
	
			if ( ((ret == TRUE) || (epoch >= (MAX_EPOCHS-1))) && (epoch >= EPOCH_DELAY))
			{
				//printf("#DETECTION IN %d EPOCHS ITERATION %d DELAY 1 %lu DELAY 2 %lu \n\n", (epoch-EPOCH_DELAY)+1, iteration, delay[0], delay[1]);
				if (METRIC == 0)
				{				
					cdf[instant_contact/ SEGMENT_TIME ]++;
					freeMatrix(previous_listen_matrix, previous_listen_matrix_extra, previous_beacon_matrix);
					previous_listen_matrix = previous_listen_matrix_extra = previous_beacon_matrix = NULL;
					break;
				}
				
				if ((METRIC == 1) && (first_detection != 0) && (instant_contact > 0) )
				{
					second_detection = instant_contact;
					if (second_detection > first_detection)
					{
						cdf[(second_detection - first_detection)/ SEGMENT_TIME ]++;
						freeMatrix(previous_listen_matrix, previous_listen_matrix_extra, previous_beacon_matrix);
						previous_listen_matrix = previous_listen_matrix_extra = previous_beacon_matrix = NULL;
						break;
					}
				}
				else if ((METRIC == 1) && (first_detection == 0))
				{
					first_detection = instant_contact;
					instant_contact = 0;
				}	
			}
			freeMatrix(previous_listen_matrix, previous_listen_matrix_extra, previous_beacon_matrix);
			previous_listen_matrix = listen_matrix;
			previous_listen_matrix_extra = listen_matrix_extra;
			previous_beacon_matrix = beacon_matrix;
		}
	}
	
	//Generate the CDF for the detection values
	for (i=1; i < ((experiment_conf.epoch_t*N_EPOCHS_SAVED) / SEGMENT_TIME ) ; i++)
		cdf[i] = cdf[i] + cdf[i-1];
	
	for (i=0; i < ((experiment_conf.epoch_t*N_EPOCHS_SAVED) / SEGMENT_TIME ) ; i++)
		printf("%lu %f\n", i * SEGMENT_TIME, (double) cdf[i]/experiment_conf.n_number_iterations );
	
	gsl_rng_free (r);
	exit(0);
}
