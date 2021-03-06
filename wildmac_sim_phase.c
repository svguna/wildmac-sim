#include <stdio.h>
#include <sys/time.h>
#include <gsl/gsl_rng.h>
#include <unistd.h>
#include <stdlib.h>

#include "sim_config.h"
#include "sim_api.h"

gsl_rng *rnd_gen;

int64_t EPOCH, BEACON, CONTACT_OVERLAP, SAMPLES;

static unsigned long get_seed()
{
    static int s = 0;
    time_t t = time(NULL);
    s += getpid(); 
    
    srandom(t % (getpid() + s));
    long seed = random();
    printf("#seed: %ld\n", seed);
    return seed;
}


static int wildmac_sim()
{
    int64_t epoch1, epoch2, new_epoch1, new_epoch2;
    int64_t a1, a2, new_a1, new_a2;
    int64_t contact = -1;
    int64_t true_contact = 0;
    int to_advance = 1;
    int n1 = 0, n2 = 0;
    int i, tmp;


    tmp = gsl_rng_uniform_int(rnd_gen, 10000);
    new_epoch1 = epoch1 = 0;
    
    true_contact = gsl_rng_uniform_int(rnd_gen, EPOCH);
    new_epoch2 = epoch2 = -gsl_rng_uniform_int(rnd_gen, EPOCH);

    printf("#start: %lld %lld\n", epoch1, epoch2);

    new_a1 = a1 = get_activity(0, epoch1);
    new_a2 = a2 = get_activity(1, epoch2);

    contact = check_contact(epoch1, a1, epoch2, a2, true_contact);

    while (contact < 0 && n1 < EPOCH_LIMIT && n2 <= EPOCH_LIMIT) {
        if (to_advance == 1) {
            new_epoch1 = get_next_epoch(epoch1, a1);
            n1++;
            new_a1 = get_activity(0, new_epoch1);
            contact = check_contact(new_epoch1, new_a1, epoch2, a2,
                    true_contact);
            if (contact >= 0)
                break;
            to_advance = 2;
            continue;
        }

        new_epoch2 = get_next_epoch(epoch2, a2);
        n2++;
        new_a2 = get_activity(1, new_epoch2);

        contact = check_contact(new_epoch2, new_a2, epoch1, a1, true_contact);
        if (contact >= 0)
            break;
        contact = check_contact(new_epoch2, new_a2, new_epoch1, new_a1,
                true_contact);
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

    return contact - true_contact;
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


int main(int narg, char *varg[])
{
    int i;
    int64_t contacts[REPEAT];
    int contacts_cnt = 0;

    rnd_gen = gsl_rng_alloc(gsl_rng_mrg);

    gsl_rng_set(rnd_gen, get_seed());
    
    EPOCH = atoll(varg[1]);
    BEACON = atoll(varg[2]);
    CONTACT_OVERLAP = atoll(varg[3]);
    SAMPLES = atoll(varg[4]);

    assert_config();
    
    for (i = 0; i < REPEAT; i++) {
        int64_t tmp = wildmac_sim();
        if (tmp >= 0)
            contacts[contacts_cnt++] = tmp;
    }

    qsort(contacts, contacts_cnt, sizeof(int64_t), compar);
    for (i = 0; i < contacts_cnt; i++) {
        if (i < contacts_cnt - 1 && contacts[i + 1] == contacts[i])
            continue;
        printf("%f %f %f\n", (float) contacts[i], 
                (float) i / contacts_cnt, (float) i / REPEAT);
    }
    gsl_rng_free(rnd_gen);
    
    return 0;
}
