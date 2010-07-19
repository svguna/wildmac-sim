#include <stdio.h>
#include <sys/time.h>
#include <gsl/gsl_rng.h>
#include <unistd.h>
#include <stdlib.h>

// Time (us) that a receiver must detect (at a minimum) a beacon.
#define CONTACT_OVERLAP 250

#define TIME_LIMIT 60000000LL

#define EPOCH 100000LL
#define BEACON 10000LL
#define SAMPLES 4LL
#define REPEAT 10000LL

#define EPOCH_LIMIT (TIME_LIMIT / EPOCH)

#define ACTIVE (BEACON + BEACON * SAMPLES)
#define RANGE (EPOCH - ACTIVE)

#define LOW BEACON
#define UP ((SAMPLES + 1) * BEACON - CONTACT_OVERLAP)

gsl_rng *rnd_gen[2];

static unsigned long get_seed()
{
    long seed = random();
    printf("#seed: %ld\n", seed);
    return seed;
}


static inline int64_t check_contact(int64_t e1, int64_t t1, int64_t e2, 
        int64_t t2)
{
    int contact = (t2 > 0 && t2 > t1 + LOW && t2 < t1 + UP) || 
        (t1 > 0 && t1 > t2 + LOW && t1 < t2 + UP);
    if (!contact) {
        return -1;
    }
    if (t2 > 0 && t2 > t1 + LOW && t2 < t1 + UP) {
        printf ("#%lld 1 (%lld, %lld, %lld, %lld) receives 2's beacon\n", t2, 
                t1, t1 + BEACON, t1 + ACTIVE, t1 + EPOCH);
        return t2;
    }
    printf ("#%lld 2 (%lld, %lld, %lld, %lld) receives 1's beacon\n", t1, t2, 
            t2 + BEACON, t2 + ACTIVE, t2 + EPOCH);
    return t1;
}


static inline int64_t get_next_epoch(int64_t e, int a)
{
    return e + EPOCH;
}

static inline int64_t get_activity(int id, int64_t epoch)
{
    return epoch + gsl_rng_uniform_int(rnd_gen[id], RANGE);
}


static int wildmac_sim()
{
    int64_t epoch1, epoch2, new_epoch1, new_epoch2;
    int64_t a1, a2, new_a1, new_a2;
    int64_t contact = -1;
    int to_advance = 1;
    int n1 = 0, n2 = 0;

    rnd_gen[0] = gsl_rng_alloc(gsl_rng_mrg);
    rnd_gen[1] = gsl_rng_alloc(gsl_rng_mrg);

    do {
        gsl_rng_set(rnd_gen[0], get_seed());
        new_epoch1 = epoch1 = 0 - gsl_rng_uniform_int(rnd_gen[0], EPOCH);
    } while (epoch1 == 0);
    do {
        gsl_rng_set(rnd_gen[1], get_seed());
        new_epoch2 = epoch2 = 0 - gsl_rng_uniform_int(rnd_gen[1], EPOCH);
    } while (epoch2 == 0 || epoch2 == epoch1);

    printf("#start: %lld %lld\n", epoch1, epoch2);

    new_a1 = a1 = get_activity(0, epoch1);
    new_a2 = a2 = get_activity(1, epoch2);

    contact = check_contact(epoch1, a1, epoch2, a2);

    while (contact < 0 && n1 < EPOCH_LIMIT && n2 < EPOCH_LIMIT) {
        if (to_advance == 1) {
            new_epoch1 = get_next_epoch(epoch1, a1);
            n1++;
            new_a1 = get_activity(0, new_epoch1);
            contact = check_contact(new_epoch1, new_a1, epoch2, a2);
            if (contact >= 0)
                break;
            to_advance = 2;
            continue;
        }

        new_epoch2 = get_next_epoch(epoch2, a2);
        n2++;
        new_a2 = get_activity(1, new_epoch2);

        contact = check_contact(new_epoch2, new_a2, epoch1, a1);
        if (contact >= 0)
            break;
        contact = check_contact(new_epoch2, new_a2, new_epoch1, new_a1);
        if (contact >= 0)
            break;
        a1 = new_a1;
        a2 = new_a2;
        epoch1 = new_epoch1;
        epoch2 = new_epoch2;
        to_advance = 1;
    }
    
    if (contact < 0) 
        printf("#no contact\n");
    else
        printf("#epochs: %d %d\n", n1, n2);

    gsl_rng_free(rnd_gen[0]);
    gsl_rng_free(rnd_gen[1]);
    return contact;
}


static int compar(const void *a, const void *b)
{
    int64_t *v1 = (int64_t *) a;
    int64_t *v2 = (int64_t *) b;
    if (*v1 < *v2)
        return -1;
    if (*v1 == *v2)
        return 0;
    return 1;
}


int main()
{
    int i;
    int64_t contacts[REPEAT];
    int contacts_cnt = 0;

    for (i = 0; i < REPEAT; i++) {
        int64_t tmp = wildmac_sim();
        if (tmp >= 0)
            contacts[contacts_cnt++] = tmp;
    }

    qsort(contacts, contacts_cnt, sizeof(int64_t), compar);
    for (i = 0; i < contacts_cnt; i++) {
        if (i < contacts_cnt - 1 && contacts[i + 1] == contacts[i])
            continue;
        printf("%lld %f %f\n", contacts[i], (float) i / contacts_cnt,
                (float) i / REPEAT);
    }
    
    return 0;
}
