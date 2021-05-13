/* @file lwippools.h */
/* LWIP_MALLOC_MEMPOOL(number_elements, element_size) */
#if MEM_USE_POOLS
LWIP_MALLOC_MEMPOOL_START
LWIP_MALLOC_MEMPOOL(10, 256)
LWIP_MALLOC_MEMPOOL(4, 512)
LWIP_MALLOC_MEMPOOL(4, 1512)
LWIP_MALLOC_MEMPOOL_END
#endif

/* No custom pools defined. */
