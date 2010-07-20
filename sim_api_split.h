#ifndef __SIM_API_H
#define __SIM_API_H

int64_t check_contact(int64_t e1, int64_t b1, int64_t l1, int64_t e2, 
        int64_t b2, int64_t l2);
int64_t get_next_epoch(int64_t e, int a, int l);
int64_t get_beacon(int id, int64_t epoch, int64_t begin_restrict,
        int64_t end_restrict);
int64_t get_listen(int id, int64_t epoch, int64_t begin_restrict,
        int64_t end_restrict);

#endif
