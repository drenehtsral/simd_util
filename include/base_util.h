#ifndef _BASE_UTIL_H_
#define _BASE_UTIL_H_

void _debug_print_vec(const void *data, const unsigned nlanes,
                      const unsigned lanesize, const char *name,
                      const unsigned long long mask, FILE *f);

#define debug_print_vec(_vec, _mask)                        \
({                                                          \
    typeof(_vec) __vec = (_vec);                            \
    const unsigned _sz = sizeof(__vec);                     \
    const unsigned _ls = sizeof(__vec[0]);                  \
    const unsigned _nl = _sz / _ls;                         \
    _debug_print_vec(&(__vec), _nl, _ls, #_vec, (_mask),    \
        stdout);                                            \
}) /* end of macro */

#define debug_fprint_vec(_f, _vec, _mask)                   \
({                                                          \
    typeof(_vec) __vec = (_vec);                            \
    const unsigned _sz = sizeof(__vec);                     \
    const unsigned _ls = sizeof(__vec[0]);                  \
    const unsigned _nl = _sz / _ls;                         \
    _debug_print_vec(&(__vec), _nl, _ls, #_vec, (_mask),    \
        (_f));                                              \
}) /* end of macro */

#define PAGE_SHIFT      (12)
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PAGE_MASK       (PAGE_SIZE - 1)

#define HUGE_2M_SHIFT   (21)
#define HUGE_2M_SIZE    (1UL << HUGE_2M_SHIFT)
#define HUGE_2M_MASK    (HUGE_2M_SIZE - 1)

#define HUGE_1G_SHIFT   (30)
#define HUGE_1G_SIZE    (1UL << HUGE_1G_SHIFT)
#define HUGE_1G_MASK    (HUGE_1G_SIZE - 1)

#define SEG_DESC_INITD          (1 << 0)    // Segment descriptor is pre-initialized
#define SEG_DESC_RO             (1 << 1)    // Segment should be mapped read-only
#define SEG_DESC_UNLINK         (1 << 3)    // Segment should be unlinked after mapping
#define SEG_DESC_CREATE         (1 << 4)    // Segment should be created
#define SEG_DESC_ADDR_HINT      (1 << 5)    // Segment should try to map at supplied addr
#define SEG_DESC_ADDR_FIXED     (1 << 6)    // Segment must be mapped at supplied address
#define SEG_DESC_ANON           (1 << 7)    // Segment should be allocated via anonymous mmap()

typedef struct {
    u64 maplen, psize, flags;
    void *ptr;
} seg_desc_t;

void consume_data(const void * const RESTR data, const size_t len);
int randomize_data(void * const RESTR data, const size_t len);
int load_data(void * const RESTR data, const size_t len, const char *fname);
int get_page_size(const char *path);

int map_segment(const char *path, seg_desc_t *seg, char *errbuf, const unsigned eblen);
int unmap_segment(const seg_desc_t * const seg);

#endif /* _BASE_UTIL_H_ */
