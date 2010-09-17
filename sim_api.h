#ifndef __SIM_API_H
#define __SIM_API_H

void assert_config();
int64_t check_contact(int64_t e1, int64_t t1, int64_t e2, int64_t t2,
        int64_t true_contact);
int64_t get_next_epoch(int64_t e, int a);
int64_t get_activity(int id, int64_t epoch);

#endif
