#ifndef _BASE_UTIL_H_
#define _BASE_UTIL_H_

void _debug_print_vec(const void *data, const unsigned nlanes,
                      const unsigned lanesize, const char *name,
                      const unsigned long long mask);

#define debug_print_vec(_vec, _mask)                        \
({                                                          \
    typeof(_vec) __vec = (_vec);                            \
    const unsigned _sz = sizeof(__vec);                     \
    const unsigned _ls = sizeof(__vec[0]);                  \
    const unsigned _nl = _sz / _ls;                         \
    _debug_print_vec(&(__vec), _nl, _ls, #_vec, (_mask));   \
}) /* end of macro */

#define PAGE_SHIFT      (12)
#define PAGE_SIZE       (1U << PAGE_SHIFT)
#define PAGE_MASK       (PAGE_SIZE - 1)

#define HUGE_2M_SHIFT   (21)
#define HUGE_2M_SIZE    (1U << HUGE_2M_SHIFT)
#define HUGE_2M_MASK    (HUGE_2M_SIZE - 1)

#define HUGE_1G_SHIFT   (30)
#define HUGE_1G_SIZE    (1U << HUGE_1G_SHIFT)
#define HUGE_1G_MASK    (HUGE_1G_SIZE - 1)

typedef struct {
    u64 maplen, psize;
    void *ptr;
} seg_desc_t;

void consume_data(const void * const RESTR data, const size_t len);
int randomize_data(void * const RESTR data, const size_t len);
int get_page_size(const char *path);
seg_desc_t map_segment(const char *path);

#endif /* _BASE_UTIL_H_ */
