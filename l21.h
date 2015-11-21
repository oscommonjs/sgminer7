#ifndef LYRA2RE_H
#define LYRA2RE_H

#include "miner.h"
#define LYRA_SCRATCHBUF_SIZE (1536) // matrix size [12][4][4] uint64_t or equivalent
#define LYRA_SECBUF_SIZE (4) // (not used)
extern void l2_regenhash(struct work *work);
extern void do_regenhash(struct work *work);
extern void ph_regenhash(struct work *work);
extern void wh_regenhash(struct work *work);
extern void wx_regenhash(struct work *work);

#endif /* LYRA2RE_H */
