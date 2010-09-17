#include <assert.h>
#include "sim_config.h"
#include "sim_api.h"

#define ACTIVE (CONTACT_OVERLAP + BEACON * (SAMPLES + 1))
#define RANGE (EPOCH - ACTIVE)

#define LOW (CONTACT_OVERLAP) 
#define UP (ACTIVE - CONTACT_OVERLAP)

void assert_config()
{
    printf("epoch = %lld, beacon = %lld, overlap = %lld, samples = %lld, "
            "on = %f%%\n", EPOCH, BEACON, CONTACT_OVERLAP, SAMPLES,
            (float) ACTIVE / EPOCH);
    
    assert(ACTIVE < EPOCH / 2);
}

int64_t check_contact(int64_t e1, int64_t t1, int64_t e2, int64_t t2,
        int64_t true_contact)
{
    int contact = (t2 > t1 + LOW && t2 < t1 + UP) || 
        (t1 > t2 + LOW && t1 < t2 + UP);
   
    if (contact && t1 < true_contact && t2 < true_contact) {
        int64_t sample;
        int64_t rcv = t1;
        int64_t tx = t2;
        if (t1 > t2) {
            rcv = t2;
            tx = t1;
        }
        sample = (tx - rcv - CONTACT_OVERLAP) / BEACON;
        if ((tx - rcv - CONTACT_OVERLAP) % BEACON != 0)
            sample++;
        if (rcv + CONTACT_OVERLAP + sample * BEACON < true_contact)
            contact = 0;
    }

    if (!contact) {
        printf ("#no contact (e1=%lld, e2=%lld, t1=%lld - %lld - %lld,"
                "t2=%lld - %lld - %lld, tc=%lld)\n", e1, e2, t1, t1 + BEACON, 
                t1 + ACTIVE, t2, t2 + BEACON, t2 + ACTIVE, true_contact); 
        return -1;
    }
    if (t2 > t1 + LOW && t2 < t1 + UP) {
        printf ("#%lld 1 (%lld, %lld, %lld, %lld, %lld, %lld, %lld) "
                "receives 2's beacon\n", t2, e1, e2, t1, t1 + BEACON, 
                t1 + ACTIVE, t1 + EPOCH, true_contact);
        return t2;
    }
    printf ("#%lld 2 (%lld %lld %lld, %lld, %lld, %lld, %lld) receives 1's "
            "beacon\n", t1, e1, e2, t2, t2 + BEACON, t2 + ACTIVE, t2 + EPOCH, 
            true_contact);
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



