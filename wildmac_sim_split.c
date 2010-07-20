#include <stdio.h>
#include <sys/time.h>
#include <gsl/gsl_rng.h>
#include <unistd.h>
#include <stdlib.h>

#include "sim_config.h"
#include "sim_api_split.h"

gsl_rng *rnd_gen[2];

static unsigned long get_seed()
{
    long seed = random();
    printf("#seed: %ld\n", seed);
    return seed;
}


static int wildmac_sim()
{
    int64_t epoch1, epoch2, new_epoch1, new_epoch2;
    int64_t b1, b2, new_b1, new_b2;
    int64_t l1, l2, new_l1, new_l2;

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

    new_b1 = b1 = get_beacon(0, epoch1, -1, -1);
    new_b2 = b2 = get_beacon(1, epoch2, -1, -1);

    new_l1 = l1 = get_listen(0, epoch1, b1, b1 + BEACON);
    new_l2 = l2 = get_listen(1, epoch2, b2, b2 + BEACON);

    contact = check_contact(epoch1, b1, l1, epoch2, b2, l2);

    while (contact < 0 && n1 < EPOCH_LIMIT && n2 < EPOCH_LIMIT) {
        if (to_advance == 1) {
            new_epoch1 = get_next_epoch(epoch1, b1, l1);
            n1++;
            new_b1 = get_beacon(0, new_epoch1, -1, -1);
            new_l1 = get_listen(0, new_epoch1, new_b1, new_b1 + BEACON); 
            
            contact = check_contact(new_epoch1, new_b1, new_l1, epoch2, b2, l2);
            if (contact >= 0)
                break;
            to_advance = 2;
            continue;
        }

        new_epoch2 = get_next_epoch(epoch2, b2, l2);
        n2++;
        new_b2 = get_beacon(1, new_epoch2, -1, -1);
        new_l2 = get_listen(1, new_epoch2, new_b2, new_b2 + BEACON);

        contact = check_contact(new_epoch2, new_b2, new_l2, epoch1, b1, l1);
        if (contact >= 0)
            break;
        contact = check_contact(new_epoch2, new_b2, new_l2, new_epoch1, new_b1,
                new_l1);
        if (contact >= 0)
            break;
        b1 = new_b1;
        b2 = new_b2;
        l1 = new_l1;
        l2 = new_l2;
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
