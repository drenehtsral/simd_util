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

void consume_data(const void * const RESTR data, const size_t len);
int randomize_data(void * const RESTR data, const size_t len);


#endif /* _BASE_UTIL_H_ */
