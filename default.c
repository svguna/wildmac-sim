#include "sim_config.h"

#define ACTIVE (CONTACT_OVERLAP + BEACON + BEACON * SAMPLES)
#define RANGE (EPOCH - ACTIVE)

#define LOW BEACON
#define UP ((SAMPLES + 1) * BEACON)

int64_t check_contact(int64_t e1, int64_t t1, int64_t e2, int64_t t2)
{
    int contact = (t2 > t1 + LOW && t2 < t1 + UP) || 
        (t1 > t2 + LOW && t1 < t2 + UP);
    if (!contact) {
        printf ("#no contact (e1=%lld, e2=%lld, t1=%lld - %lld - %lld,"
                "t2=%lld - %lld - %lld)\n", e1, e2, t1, t1 + BEACON, 
                t1 + ACTIVE, t2, t2 + BEACON, t2 + ACTIVE); 
        return -1;
    }
    if (t2 > t1 + LOW && t2 < t1 + UP) {
        printf ("#%lld 1 (%lld, %lld, %lld, %lld, %lld, %lld) receives 2's "
                "beacon\n", t2, e1, e2, t1, t1 + BEACON, t1 + ACTIVE, 
                t1 + EPOCH);
        return t2;
    }
    printf ("#%lld 2 (%lld %lld %lld, %lld, %lld, %lld) receives 1's beacon\n",
            t1, e1, e2, t2, t2 + BEACON, t2 + ACTIVE, t2 + EPOCH);
    return t1;
}


int64_t get_next_epoch(int64_t e, int a)
{
    return e + EPOCH;
}

inline int64_t get_activity(int id, int64_t epoch)
{
    return epoch + gsl_rng_uniform_int(rnd_gen[id], RANGE);
}



