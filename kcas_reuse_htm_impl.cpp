#include "kcas_reuse_htm_impl.h"

KCASHTM<100000> kcasInstance;
thread_local TIDGenerator kcas_tid;