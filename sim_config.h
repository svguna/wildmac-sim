#ifndef __SIM_CONFIG_H
#define __SIM_CONFIG_H

#include <gsl/gsl_rng.h>

#define TIME_LIMIT 60000000LL

#define EPOCH 100000LL
#define BEACON 5000LL
#define SAMPLES 5LL
#define REPEAT 10000LL

#define CONTACT_OVERLAP 250
#define EPOCH_LIMIT (TIME_LIMIT / EPOCH)

int64_t check_contact(int64_t e1, int64_t t1, int64_t e2, int64_t t2);
int64_t get_next_epoch(int64_t e, int a);
int64_t get_activity(int id, int64_t epoch);

extern gsl_rng *rnd_gen[2];

#endif
