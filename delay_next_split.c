#include "sim_config.h"

#define ACTIVE (BEACON + BEACON * SAMPLES)
#define RANGE (EPOCH - ACTIVE)

#define UP (SAMPLES * BEACON - CONTACT_OVERLAP)

int64_t check_contact(int64_t e1, int64_t b1, int64_t l1, int64_t e2, 
        int64_t b2, int64_t l2)
{
    int contact = (b2 + BEACON - CONTACT_OVERLAP > 0 && b2 > l1 && 
            b2 < l1 + UP) || 
        (b1 + BEACON - CONTACT_OVERLAP > 0 && b1 > l2 && b1 < l2 + UP);
    if (!contact) {
        return -1;
    }
    if (b2 + BEACON - CONTACT_OVERLAP > 0 && b2 > l1 && b2 < l1 + UP) {
        printf ("#%lld 1 (%lld, %lld) receives 2's beacon\n", b2, l1, 
                b1 + BEACON * SAMPLES);
        return b2;
    }
    printf ("#%lld 2 (%lld, %lld) receives 1's beacon\n", b1, l2, 
            b2 + BEACON * SAMPLES);
    return b1;
}


int64_t get_next_epoch(int64_t e, int a, int l)
{
    int64_t next1 = a + BEACON;
    int64_t next = l + BEACON * SAMPLES;
    if (next1 > next)
        next = next1;
    if (next < e + EPOCH)
        return e + EPOCH;
    return next;
}


int64_t get_value(int id, int64_t epoch, int64_t begin_restrict,
        int64_t end_restrict)
{
    int64_t range = EPOCH, result = epoch;
    if (begin_restrict != -1) 
        range += begin_restrict - end_restrict;
    
    result += gsl_rng_uniform_int(rnd_gen[id], range);
    if (result >= begin_restrict)
        result += end_restrict - begin_restrict;
    return result;
}


int64_t get_beacon(int id, int64_t epoch, int64_t begin_restrict,
        int64_t end_restrict)
{
    return get_value(id, epoch, begin_restrict, end_restrict);
}


int64_t get_listen(int id, int64_t epoch, int64_t begin_restrict,
        int64_t end_restrict)
{
    return get_value(id, epoch, begin_restrict, end_restrict);
}

