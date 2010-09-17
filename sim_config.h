#ifndef __SIM_CONFIG_H
#define __SIM_CONFIG_H

#include <gsl/gsl_rng.h>

#define TIME_LIMIT 150000000LL

extern int64_t EPOCH, BEACON, CONTACT_OVERLAP, SAMPLES;

#define REPEAT 10000LL

#define EPOCH_LIMIT (TIME_LIMIT / EPOCH)

extern gsl_rng *rnd_gen[2];

#endif
