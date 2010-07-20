#include "sim_config.h"

int64_t check_contact_basic(int64_t e_r, int64_t t_r, int id_r, int64_t e_t, 
        int64_t t_t, int id_t)
{
    int64_t end = e_r + EPOCH;
    int at_end = (end - t_r) / BEACON;
    if (at_end > SAMPLES)
        at_end = SAMPLES;

    if (t_t + BEACON - CONTACT_OVERLAP > 0 && t_t > t_r && 
            t_t < t_r + at_end * BEACON - CONTACT_OVERLAP) {
        printf ("#%lld %d (%lld, e%d, %lld, %lld) receives %d's beacon\n", t_t, 
                id_r, t_r, at_end, 
                t_r + at_end * BEACON - CONTACT_OVERLAP, end, id_t);
        return t_t;
    }

    if (at_end < SAMPLES) {
        int at_begin = SAMPLES - at_end;
        printf("#%lld %lld %d\n", end, t_r, at_begin);
        int offset = end - t_r - at_end * BEACON;
        if (t_t + BEACON - CONTACT_OVERLAP > 0 && t_t > e_r - offset && 
                t_t < e_r - offset + at_begin * BEACON - CONTACT_OVERLAP) {
            printf ("#%lld %d (%lld, b%d, %lld, %lld) receives %d's beacon\n", 
                    t_t, id_r, e_r - offset, at_begin, 
                    e_r - offset + at_begin * BEACON - CONTACT_OVERLAP,
                    e_r, id_t);
            return t_t;
        }
    }
    return -1;
}

int64_t check_contact(int64_t e1, int64_t b1, int64_t l1, int64_t e2, 
        int64_t b2, int64_t l2)
{
    int64_t t;
    t = check_contact_basic(e1, l1, 1, e2, b2, 2);
    if (t >= 0)
        return t;
    return check_contact_basic(e2, l2, 2, e1, b1, 1);
}


int64_t get_next_epoch(int64_t e, int a, int l)
{
    return e + EPOCH;
}


int64_t get_value(int id, int64_t epoch, int64_t begin_restrict,
        int64_t end_restrict)
{
    int64_t range = EPOCH, result = epoch;
    range += begin_restrict - end_restrict;
    
    result += gsl_rng_uniform_int(rnd_gen[id], range);
    if (result >= begin_restrict)
        result += end_restrict - begin_restrict;
    return result;
}


int64_t get_beacon(int id, int64_t epoch, int64_t begin_restrict,
        int64_t end_restrict)
{
    if (begin_restrict == -1) {
        begin_restrict = epoch;
        end_restrict = epoch + EPOCH - BEACON; 
    }
    if (end_restrict > epoch + EPOCH - BEACON)
        end_restrict = epoch + EPOCH - BEACON;

    return get_value(id, epoch, begin_restrict, end_restrict);
}


int64_t get_listen(int id, int64_t epoch, int64_t begin_restrict,
        int64_t end_restrict)
{
    return get_value(id, epoch, begin_restrict, end_restrict);
}


