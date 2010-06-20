//gcc -o simulation simulation_v8.c -Wall -I/opt/local/include -L/opt/local/lib -lgsl

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include <sys/time.h>
#include "dyn_struct.h"
#include "time_energy.h"

#include <gsl/gsl_rng.h>
#include <gsl/gsl_statistics_ulong.h>
#include <gsl/gsl_statistics_double.h>

#include <gsl/gsl_sort_ulong.h>

#define TRUE 1
#define FALSE 0

#define MAX_EPOCHS 100
#define COLLISION_WAIT 400
#define SEPARATION_PROPORTION 20
#define SAMPLE_TIME 100
#define SLOT_TIME 500 //slot size used to calculate the number of cells of the array used to calculate the cumulative distriubted probability
#define TIMES 100 //used to definee the size of the array in base to the epoch size, which will be multiplied by TIMES
#define EPOCH_DELAY 5

struct t_conf {
	unsigned long n_sensors;
	unsigned long n_number_iterations;
	unsigned long epoch_t;
	unsigned long beacon_t;
	unsigned long listen_t;
	unsigned long min_num_det;
	int divide_actions;
	int phase;
};

struct t_results {
	unsigned long *detection_results;
	unsigned long *collision_results;
	double *energy_results;
};


gsl_rng *r;
struct t_conf sensor_conf;
struct t_results iteration_result;
unsigned long *delay;
unsigned long *beacon_matrix;
unsigned long *listen_matrix;
unsigned long *previous_beacon_matrix;
unsigned long *previous_listen_matrix;
char **detection_control;
unsigned long collisions;
unsigned long *order_info;
unsigned long *cdf; //stores the detection made until a certain time
unsigned long instant_contact;

void showData(void)
{
	unsigned long sensor;
	
	for(sensor=0; sensor < sensor_conf.n_sensors; sensor++)
	{
		if(previous_beacon_matrix != NULL)
		printf("Delay %lu - Previous Beacon %lu %lu - Previous Listen %lu\n", delay[sensor], previous_beacon_matrix[sensor], previous_beacon_matrix[sensor] + sensor_conf.beacon_t, previous_listen_matrix[sensor]);
		printf("Delay %lu - Current Beacon %lu %lu - Current Listen %lu\n", delay[sensor], beacon_matrix[sensor]+sensor_conf.epoch_t, beacon_matrix[sensor] + sensor_conf.beacon_t+sensor_conf.epoch_t, listen_matrix[sensor]+sensor_conf.epoch_t);
	}
}

unsigned long int getSeed(void)
{
	struct timeval cur_time;

	gettimeofday(&cur_time, NULL);

	return cur_time.tv_usec;
}

//we order everything on base of the actual epoch beaconing order, all the data in position X of each array belongs to the same node
void orderArray(void)
{
	unsigned long i,j,temp;

	for (i = 0; i < sensor_conf.n_sensors - 1; i++) {
		for (j = i + 1; j < sensor_conf.n_sensors; j++) {
			if (beacon_matrix[i] > beacon_matrix[j]) {
				temp = beacon_matrix[i];
				beacon_matrix[i] = beacon_matrix[j];
				beacon_matrix[j] = temp;

				temp = order_info[i];
				order_info[i] = order_info[j];
				order_info[j] = temp;

				temp = delay[i];
				delay[i] = delay[j];
				delay[j] = temp;

				if (previous_beacon_matrix != NULL) {
					temp = previous_beacon_matrix[i];
					previous_beacon_matrix[i] = previous_beacon_matrix[j];
					previous_beacon_matrix[j] = temp;

					temp = previous_listen_matrix[i];
					previous_listen_matrix[i] = previous_listen_matrix[j];
					previous_listen_matrix[j] = temp;
				}
			}
		}
	}
}

void calcStatValue(void)
{
	double mean, stand_dev, mean_coll, stand_dev_coll, mean_energ, stand_dev_energ, min_energ, max_energ;
	unsigned long min, max, min_coll, max_coll;

	mean = gsl_stats_ulong_mean (iteration_result.detection_results, 1, sensor_conf.n_number_iterations);
	stand_dev = gsl_stats_ulong_sd(iteration_result.detection_results, 1, sensor_conf.n_number_iterations);
	gsl_stats_ulong_minmax (&min, &max, iteration_result.detection_results, 1, sensor_conf.n_number_iterations);

	mean_coll = gsl_stats_ulong_mean (iteration_result.collision_results, 1, sensor_conf.n_number_iterations);
	stand_dev_coll = gsl_stats_ulong_sd(iteration_result.collision_results, 1, sensor_conf.n_number_iterations);
	gsl_stats_ulong_minmax (&min_coll, &max_coll, iteration_result.collision_results, 1, sensor_conf.n_number_iterations);
	/*
		mean_energ = gsl_stats_mean (energy_consump, 1, sensor_conf.n_current_iterations);
		stand_dev_energ = gsl_stats_sd(energy_consump, 1, sensor_conf.n_current_iterations);
		gsl_stats_minmax (&min_energ, &max_energ, energy_consump, 1, sensor_conf.n_current_iterations);
	*/
	//printf("N Iterations - N sensors - Listen - Beacon - Min detections - mean - stand_dev - max - min - mean_coll - stand_dev_coll - min_coll - max_coll - mean_energ - stand_dev_energ - min_energ - max_energ\n");
	printf("%lu %lu %lu %lu %lu %.2f %.2f %lu %lu %.2f %.2f %lu %lu %.2f %.2f %.2f %.2f\n", sensor_conf.n_number_iterations, sensor_conf.n_sensors, sensor_conf.listen_t, sensor_conf.beacon_t, sensor_conf.min_num_det, mean, stand_dev, max, min, mean_coll, stand_dev_coll, min_coll, max_coll, mean_energ, stand_dev_energ, min_energ, max_energ);
}

void initData(void)
{
	int i;

	iteration_result.detection_results = (unsigned long *) malloc (sizeof(unsigned long)*sensor_conf.n_number_iterations);
	iteration_result.collision_results = (unsigned long *) malloc (sizeof(unsigned long)*sensor_conf.n_number_iterations);
	iteration_result.energy_results = (double *) malloc (sizeof(double)*sensor_conf.n_number_iterations);

	memset(iteration_result.detection_results,0, sensor_conf.n_number_iterations * sizeof(unsigned long));
	memset(iteration_result.collision_results,0, sensor_conf.n_number_iterations * sizeof(unsigned long));
	memset(iteration_result.energy_results,0, sensor_conf.n_number_iterations * sizeof(double));

	delay = (unsigned long *) malloc (sizeof(unsigned long)*sensor_conf.n_sensors);
	detection_control = (char **)malloc(sensor_conf.n_sensors * (sizeof(char *)));
	for (i = 0; i < sensor_conf.n_sensors; i++ )
		detection_control[i] = (char *)malloc(sensor_conf.n_sensors * sizeof(char));

	cdf = (unsigned long *) malloc (sizeof(unsigned long)*((sensor_conf.epoch_t*TIMES)/ SLOT_TIME ));
	memset(cdf,0, sizeof(unsigned long)*((sensor_conf.epoch_t*TIMES)/ SLOT_TIME ));
}

void initDataIteration(void)
{
	int i,j;

	for (i = 0; i < sensor_conf.n_sensors; i++ )
		for (j = 0; j < sensor_conf.n_sensors; j++ )
			detection_control[i][j] = '0';

	for (i = 0; i < sensor_conf.n_sensors; i++ )
	{
		if (sensor_conf.phase == FALSE)
			delay[i] = 0;
		else
		{
			if(i == 0)
				delay[i] = 0;
			else
				delay[i] = (((double) gsl_rng_uniform (r) / 1 ) * (sensor_conf.epoch_t));
		}
			//printf("Delay %lu\n", delay[i]);
	}
	
	collisions = 0;
}

unsigned long overlapCheck(unsigned long init1, unsigned long dur1, unsigned long init2, unsigned long dur2)
{
	unsigned long aux;

	if ( (init1 == ULONG_MAX) || (init2 == ULONG_MAX))
		return ULONG_MAX;

	if (init1>init2) {
		aux = init1;
		init1 = init2;
		init2 = aux;
		aux= dur1;
		dur1 = dur2;
		dur2 = aux;
	} //it already grants than init2 >= init1
	if ((((init1+dur1) >= init2) && ((init1+dur1) <= (init2+dur2))) ||  (((init2+dur2) >= init1) && ((init2) <= (init1+dur1)))) {
		if ( (init1+dur1) >= (init2+dur2) )
			aux = dur2;
		else
			aux = (init1+dur1) - init2;
	
		if ((instant_contact == 0) || (init2<instant_contact))
			instant_contact = init2; //the instant of contact is defined by the event that happens the last
		
		return aux;
	} else
		return ULONG_MAX;
	}

int checkDetectionState(void)
{
	unsigned long i,j, detections;

	for (i=0; i<sensor_conf.n_sensors; i++) {
		detections = 0;
		for (j=0; j<sensor_conf.n_sensors; j++) {

			if ( (detection_control[order_info[i]][order_info[j]] == '1' ) || (detection_control[order_info[j]][order_info[i]] == '1' ) ) //if A->B then B->A
				detections++;

			if (detections >= sensor_conf.min_num_det)
				return TRUE;
		}
	}
	return FALSE;
}

void checkDetection(int sensor)
{
	unsigned long beacon, listen, ret, i;
	//first, checking if actual beacon is listen by previous listen or actual listen	
	if (beacon_matrix[sensor] != ULONG_MAX)
	{
		if (previous_listen_matrix != NULL)
		{
			beacon = beacon_matrix[sensor] + sensor_conf.epoch_t;
			for (i=0; i<sensor_conf.n_sensors; i++)			
				if (sensor != i)
				{				
					if (previous_listen_matrix[i] != ULONG_MAX) // if were not possible to generate listen
					{ 						
						
						if ( ((sensor_conf.epoch_t + delay[i]) - previous_listen_matrix[i]) < sensor_conf.listen_t)
						{
							listen = (sensor_conf.epoch_t + delay[i]) - previous_listen_matrix[i];
						}
							else
							listen = sensor_conf.listen_t;

						ret = overlapCheck(beacon, sensor_conf.beacon_t, previous_listen_matrix[i], listen);
						if ( (ret >= sensor_conf.beacon_t) && ( ret != ULONG_MAX ) )
							detection_control[order_info[sensor]][order_info[i]] = '1';
					}
				}
			}	
			
			for (i=0; i<sensor_conf.n_sensors; i++)			
				if (sensor != i)
				{
					beacon = beacon_matrix[sensor];						
					if (listen_matrix[i] != ULONG_MAX) // if null or were not possible to generate listen for this node in previous epoch
					{ 					
						if ( ((sensor_conf.epoch_t + delay[i]) - listen_matrix[i]) < sensor_conf.listen_t)
						{
							listen = (sensor_conf.epoch_t + delay[i])- listen_matrix[i];
						}
						else
							listen = sensor_conf.listen_t;
						
						ret = overlapCheck(beacon, sensor_conf.beacon_t, listen_matrix[i], listen);
					
						if ( (ret >= sensor_conf.beacon_t) && ( ret != ULONG_MAX ) )
							detection_control[order_info[sensor]][order_info[i]] = '1';
					}
				}
		}

		//second, checking if previous beacon is listen by actual listen
		if (previous_beacon_matrix != NULL)
		{
			beacon = previous_beacon_matrix[sensor];
			if(beacon != ULONG_MAX)
			for (i=0; i<sensor_conf.n_sensors; i++) 
			{
				if (sensor != i)
				{
					if (listen_matrix[i] != ULONG_MAX) 
					{					
						if ( ((sensor_conf.epoch_t + delay[i]) - listen_matrix[i]) < sensor_conf.listen_t)
						{
							listen = (sensor_conf.epoch_t + delay[i]) - listen_matrix[i];
						}
							else
							listen = sensor_conf.listen_t;

						ret = overlapCheck(beacon, sensor_conf.beacon_t, listen_matrix[i] + sensor_conf.epoch_t, listen);
						
						if ( (ret >= sensor_conf.beacon_t) && ( ret != ULONG_MAX ) )
							detection_control[order_info[sensor]][order_info[i]] = '1';
					}
				}
			}
		}
}

//returns the number of iterations there will be still collisions
unsigned long beaconCollision(int sensor)
{
	int i;
	unsigned long overlap;

	if (previous_beacon_matrix != NULL)
		for (i=0; i< sensor_conf.n_sensors;i++)
			if (i!=sensor)
			{
				overlap = overlapCheck(previous_beacon_matrix[i],sensor_conf.beacon_t, beacon_matrix[sensor] + sensor_conf.epoch_t, sensor_conf.beacon_t );
				if (overlap != ULONG_MAX)				
					return(overlap);
			}

	for (i=0; i< sensor_conf.n_sensors;i++) //they have been ordered by time, so need to check the one who are later in time
	{
		if (i!=sensor)
		{
			overlap = overlapCheck(beacon_matrix[i],sensor_conf.beacon_t, beacon_matrix[sensor], sensor_conf.beacon_t );
			if ( overlap != ULONG_MAX)
				return(overlap);			
		}
	}
	return FALSE;
}

//in this version, there is no collision avoidance, if there is a collision, it cannot transmit
void generateBeaconData(unsigned long init_time, unsigned long upper_limit, int current_epoch)
{
	int sensor;
	unsigned long separation;
	unsigned long node_coll;

	if (sensor_conf.divide_actions == 1)
		separation = (sensor_conf.epoch_t/SEPARATION_PROPORTION); //max_value
	else
		separation = 0;

	for (sensor=0; sensor < sensor_conf.n_sensors; sensor++) 
	{
		beacon_matrix[sensor] = init_time + (((double) gsl_rng_uniform (r) / 1 ) * (sensor_conf.epoch_t -init_time - upper_limit - separation));
		beacon_matrix[sensor] = beacon_matrix[sensor] + delay[sensor];
	}

	//gsl_sort_ulong (beacon_matrix, 1, sensor_conf.n_sensors); //order by time
	//orderArray();
	//the anti-node collision (waiting a time before retransmitting) has been deactivated. if there is a collision, it will be marked as non-generated beacon (ULONG-max)
	/*
	for (sensor=1; sensor < sensor_conf.n_sensors; sensor++) 
	{
		node_coll = beaconCollision(sensor);
		while ( node_coll != FALSE) 
		{
			collisions = collisions + (node_coll/COLLISION_WAIT) + 1;
			if ((beacon_matrix[sensor] + node_coll + COLLISION_WAIT + sensor_conf.beacon_t) > (sensor_conf.epoch_t + delay[sensor])) 
			{
				beacon_matrix[sensor] = ULONG_MAX;
				break;
			} 
			else
				beacon_matrix[sensor] = beacon_matrix[sensor] + node_coll + COLLISION_WAIT;
			
			node_coll = beaconCollision(sensor);
		}
		 
		}
	*/
	//we just generate full listens
	for (sensor=0; sensor < sensor_conf.n_sensors; sensor++) 
	{
		if (beacon_matrix[sensor] != ULONG_MAX)
		{
			if (sensor_conf.divide_actions == 1)
				separation = (((double) gsl_rng_uniform (r) / 1 ) * (sensor_conf.epoch_t/SEPARATION_PROPORTION));

			if ((beacon_matrix[sensor] + sensor_conf.beacon_t + separation + sensor_conf.listen_t ) <= (sensor_conf.epoch_t + delay[sensor]))
				listen_matrix[sensor] = beacon_matrix[sensor] + sensor_conf.beacon_t + separation;
			else
			{
				listen_matrix[sensor] = ULONG_MAX;
				printf("!!\n");
			}
		}
		else
		{
			listen_matrix[sensor] = ULONG_MAX;
			printf("!!\n");
		}
	}
	//showData();
}

void generateBeaconDataImproved(unsigned long init_time, unsigned long upper_limit, int current_epoch)
{
	int sensor;
	unsigned long node_coll;	

	for (sensor=0; sensor < sensor_conf.n_sensors; sensor++) {
		beacon_matrix[sensor] = init_time + (((double) gsl_rng_uniform (r) / 1 ) * (sensor_conf.epoch_t -init_time - upper_limit));
		beacon_matrix[sensor] = beacon_matrix[sensor] + delay[sensor];
	}
	
	orderArray();
	for (sensor=1; sensor < sensor_conf.n_sensors; sensor++) {
		if ( beaconCollision(sensor) == TRUE ) 
		{
			collisions++;			
			if ((beacon_matrix[sensor] + sensor_conf.listen_t ) <= (sensor_conf.epoch_t + delay[sensor]))			
				listen_matrix[sensor] = beacon_matrix[sensor] + SAMPLE_TIME;
			else
				listen_matrix[sensor] = ULONG_MAX;
			
			beacon_matrix[sensor] = ULONG_MAX;		
		}
	}
	for (sensor=0; sensor < sensor_conf.n_sensors; sensor++) {
		if ( (listen_matrix[sensor] != ULONG_MAX) && (beacon_matrix[sensor] == ULONG_MAX)) {
			if ((listen_matrix[sensor] + sensor_conf.listen_t + sensor_conf.beacon_t) < (sensor_conf.epoch_t + delay[sensor])) {
				beacon_matrix[sensor] = listen_matrix[sensor] + sensor_conf.listen_t;
				
				node_coll = beaconCollision(sensor);
				while ( node_coll != FALSE) 
				{
					collisions = collisions + (node_coll/COLLISION_WAIT) + 1;
					if ((beacon_matrix[sensor] + node_coll + COLLISION_WAIT + sensor_conf.beacon_t) > (sensor_conf.epoch_t + delay[sensor])) 
					{
						beacon_matrix[sensor] = ULONG_MAX;
						break;
					} 
					else
						beacon_matrix[sensor] = beacon_matrix[sensor] + node_coll + COLLISION_WAIT;
					
					node_coll = beaconCollision(sensor);
				}
				
				/*												
				while ( beaconCollision(sensor) == TRUE) 
					{
						collisions++;
					if ((beacon_matrix[sensor] + COLLISION_WAIT + sensor_conf.beacon_t) > (sensor_conf.epoch_t + delay[sensor])) {
						beacon_matrix[sensor] = ULONG_MAX;
						break;
					} else
						beacon_matrix[sensor] = beacon_matrix[sensor] + COLLISION_WAIT;
				}
				*/
			}
		} else if ( (listen_matrix[sensor] == ULONG_MAX) && (beacon_matrix[sensor] != ULONG_MAX)) {
			if ((beacon_matrix[sensor] + sensor_conf.beacon_t + sensor_conf.listen_t ) <= (sensor_conf.epoch_t + delay[sensor]))
				listen_matrix[sensor] = beacon_matrix[sensor] + sensor_conf.beacon_t;
		}

	}
	
	//showData();
}


void freeMatrix(unsigned long *a, unsigned long *b)
{
	free(a);
	free(b);
}

int main(int argc, char** argv)
{
	int sensor, iteration, epoch, ret;
	unsigned long i;
	
	if (argc != 9) {
		printf("ERROR: Not enough parameters\nsimulation N_SENSORS NUMBER_ITERATIONS EPOCH_T BEACON_T LISTEN_T MIN_NUM_DETECT DIVIDE_ACTIONS PHASE- Time in microseconds\n");
		exit(1);
	}

	sensor_conf.n_sensors = atoi(argv[1]);
	sensor_conf.n_number_iterations = atoi(argv[2]);
	sensor_conf.epoch_t = atof(argv[3]);
	sensor_conf.beacon_t = atof(argv[4]);
	sensor_conf.listen_t = atof(argv[5]);
	sensor_conf.min_num_det = atof(argv[6]);
	sensor_conf.divide_actions = atoi(argv[7]);
	sensor_conf.phase = atoi(argv[8]);
	
	if ( (sensor_conf.beacon_t > sensor_conf.listen_t) || (sensor_conf.epoch_t < (sensor_conf.listen_t + sensor_conf.beacon_t)) ) {
		printf("Times are not consistent...\n");
		printf("%lu %lu %lu %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d\n", sensor_conf.n_number_iterations, sensor_conf.n_sensors, sensor_conf.listen_t, sensor_conf.beacon_t, sensor_conf.min_num_det, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		exit(1);
	}

	if ( sensor_conf.min_num_det >= sensor_conf.n_sensors ) {
		printf("Wrong definition of minimum detections...\n");
		printf("%lu %lu %lu %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d\n", sensor_conf.n_number_iterations, sensor_conf.n_sensors, sensor_conf.listen_t, sensor_conf.beacon_t, sensor_conf.min_num_det, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		exit(1);
	}

	if (sensor_conf.beacon_t < sensor_conf.beacon_t) {
		printf("ERROR: Beacon lenght not big enough to fit minimum detect sample\n");
		printf("%lu %lu %lu %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d\n", sensor_conf.n_number_iterations, sensor_conf.n_sensors, sensor_conf.listen_t, sensor_conf.beacon_t, sensor_conf.min_num_det, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		exit(1);
	}

	r = gsl_rng_alloc (gsl_rng_ranlxs2);
	gsl_rng_set (r, getSeed());

	initData();
	order_info = (unsigned long *)malloc(sensor_conf.n_sensors * sizeof(unsigned long));

	for (iteration=0; iteration < sensor_conf.n_number_iterations; iteration++) {
		initDataIteration();
		for (sensor = 0; sensor < sensor_conf.n_sensors; sensor++ )
			order_info[sensor] = sensor;
		instant_contact = 0;
		
		for (epoch=0; epoch < MAX_EPOCHS; epoch++) {
			
			beacon_matrix = (unsigned long *)malloc(sensor_conf.n_sensors * sizeof(unsigned long));
			listen_matrix = (unsigned long *)malloc(sensor_conf.n_sensors * sizeof(unsigned long));

			for (sensor = 0; sensor < sensor_conf.n_sensors; sensor++ ) {
				beacon_matrix[sensor] = ULONG_MAX;
				listen_matrix[sensor] = ULONG_MAX;
			}
			switch (sensor_conf.divide_actions) {
			case 0:
			case 1:
				generateBeaconData(0, sensor_conf.beacon_t+sensor_conf.listen_t, epoch);
				break;
			case 2:
				generateBeaconDataImproved(0, sensor_conf.beacon_t+sensor_conf.listen_t, epoch);
				break;
			}
			for (sensor=0; sensor < sensor_conf.n_sensors; sensor++)
				checkDetection(sensor);
			if (previous_beacon_matrix != NULL) 
			{
				//printf("Nodo 1: %lu %lu-%lu , %lu %lu-%lu\n", previous_beacon_matrix[0]+((epoch-1)*sensor_conf.epoch_t), previous_listen_matrix[0]+((epoch-1)*sensor_conf.epoch_t), previous_listen_matrix[0]+((epoch-1)*sensor_conf.epoch_t) + sensor_conf.listen_t , beacon_matrix[0]+(epoch*sensor_conf.epoch_t), listen_matrix[0]+(epoch*sensor_conf.epoch_t), listen_matrix[0]+(epoch*sensor_conf.epoch_t) +sensor_conf.listen_t );
				//printf("Nodo 2: %lu %lu-%lu , %lu %lu-%lu\n", previous_beacon_matrix[1]+((epoch-1)*sensor_conf.epoch_t), previous_listen_matrix[1]+((epoch-1)*sensor_conf.epoch_t), previous_listen_matrix[1]+((epoch-1)*sensor_conf.epoch_t) + sensor_conf.listen_t, beacon_matrix[1]+(epoch*sensor_conf.epoch_t), listen_matrix[1]+(epoch*sensor_conf.epoch_t), listen_matrix[1]+(epoch*sensor_conf.epoch_t) +sensor_conf.listen_t);
			}
			else {
				//printf("Nodo 1: 0 0 , %lu %lu-%lu\n", beacon_matrix[0], listen_matrix[0], listen_matrix[0] + sensor_conf.listen_t );
				//printf("Nodo 2: 0 0 , %lu %lu-%lu\n", beacon_matrix[1], listen_matrix[1],listen_matrix[1] + sensor_conf.listen_t);
			}

			//printf("%lu 1 %lu\n",  beacon_matrix[0]+(epoch*sensor_conf.epoch_t) + ((sensor_conf.listen_t + sensor_conf.beacon_t)/2), (sensor_conf.listen_t + sensor_conf.beacon_t) );
			//printf("%lu 2 %lu\n",  beacon_matrix[1]+(epoch*sensor_conf.epoch_t) + ((sensor_conf.listen_t + sensor_conf.beacon_t)/2), (sensor_conf.listen_t + sensor_conf.beacon_t) );
			//printf("\n");
			
			if (epoch >= EPOCH_DELAY) {
				ret = checkDetectionState();
			}
	
			if ( ((ret == TRUE) || (epoch >= (MAX_EPOCHS-1))) && (epoch >= EPOCH_DELAY))
			{								
				//printf("DETECTION IN %d EPOCHS ITERATION %d DELAY 1 %lu DELAY 2 %lu \n\n", epoch+1, iteration, delay[0], delay[1]);
				
				instant_contact = instant_contact + (epoch-EPOCH_DELAY)*sensor_conf.epoch_t;				

				cdf[instant_contact/ SLOT_TIME ]++;
				
				iteration_result.detection_results[iteration] = epoch+1; //to keep epochs between 1-MAX_EPOCHS
				iteration_result.collision_results[iteration] = (unsigned long) (collisions / sensor_conf.n_sensors);
				freeMatrix(previous_listen_matrix, previous_beacon_matrix);
				previous_listen_matrix = previous_beacon_matrix = NULL;
				break;
			}
			freeMatrix(previous_listen_matrix, previous_beacon_matrix);
			previous_listen_matrix = listen_matrix;
			previous_beacon_matrix = beacon_matrix;
		}
	}
	//cumulative version of the detection data stored
	for (i=1; i < ((sensor_conf.epoch_t*TIMES) / SLOT_TIME ) ; i++) {
	//for (i=((sensor_conf.epoch_t) / SLOT_TIME ); i < ((sensor_conf.epoch_t*TIMES) / SLOT_TIME ) ; i++) {
		cdf[i] = cdf[i] + cdf[i-1];
	}
	
	for (i=0; i < ((sensor_conf.epoch_t*TIMES) / SLOT_TIME ) ; i++) {
		//printf("%lu %lu\n", i * SLOT_TIME, cdf[i]);
		printf("%lu %f.2\n", i * SLOT_TIME, (double) cdf[i]/sensor_conf.n_number_iterations );
	}
	
	calcStatValue();
	gsl_rng_free (r);
	exit(0);
}
