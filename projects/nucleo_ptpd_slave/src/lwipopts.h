#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/* We try to keep the lwIP options common between all projects.  However,
 * if a certain project requires specific lwIP options we can specify
 * them here. */
#define LWIP_PTPD             1

/* Include the lwIP options file common to all projects. */
#include "lwipopts_shared.h"

#endif /* __LWIPOPTS_H__ */
