#ifndef __SIM_CONFIG_H
#define __SIM_CONFIG_H

#include <gsl/gsl_rng.h>

#define TIME_LIMIT 60000000LL

#define EPOCH 100000LL
#define BEACON 5000LL
#define SAMPLES 9LL
#define REPEAT 100000LL

#define CONTACT_OVERLAP 250
#define EPOCH_LIMIT (TIME_LIMIT / EPOCH)

extern gsl_rng *rnd_gen[2];

#endif
