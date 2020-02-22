#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#include "../include/simd_util.h"

void consume_data(const void * const RESTR data, const size_t len)
{
    /*
     * Do nothing, but if this is linked as an external object
     * or library, the optimizer cannot know that it does nothing
     * so it forces the compiler to actually generate code to do
     * what the program says... (this is useful for keeping
     * benchmarks honest).
     */
}

/*
 * This is a quick and dirty universal masked-vector-printing function,
 * It is not intended to be particularly efficient or fancy, but for
 * convenience it can be called using the debug_print_vec() macro
 * defined in base_util.h
 */
void _debug_print_vec(const void *data, const unsigned nlanes,
                      const unsigned lanesize, const char *name,
                      const unsigned long long mask)
{
    unsigned i, first = 1;

    switch (lanesize) {
        case 1:
        case 2:
        case 4:
        case 8:
            break;

        default:
            return;
    }

    if ((lanesize * nlanes) > sizeof(u64_8)) {
        return;
    }

    printf("%s = {", name);

    for (i = 0; i < nlanes; i++) {
        if (!((mask >> i) & 1)) {
            continue;
        }

        const char *pfx = first ? " " : ", ";
        first = 0;

        switch (lanesize) {
            case 1:
                printf("%s[%u]=0x%02x", pfx, i, ((const unsigned char *)data)[i]);
                break;

            case 2:
                printf("%s[%u]=0x%04x", pfx, i, ((const unsigned short *)data)[i]);
                break;

            case 4:
                printf("%s[%u]=0x%08x", pfx, i, ((const unsigned int *)data)[i]);
                break;

            case 8:
                printf("%s[%u]=0x%016llx", pfx, i,
                       ((const unsigned long long *)data)[i]);
                break;

            default:
                break;
        }
    }

    printf(" }\n");
}

int randomize_data(void * const RESTR data, const size_t len)
{
    const unsigned max_chunk = (1 << 21);
    char * RESTR trav = (char *)data;
    size_t left = len;

    int ret;
    const int fd = open("/dev/urandom", O_RDONLY);

    if (fd < 0) {
        return -1;
    }

    while (left) {
        const unsigned chunk = (left <= max_chunk) ? left : max_chunk;
        ret = read(fd, trav, chunk);

        if (ret <= 0) {
            if ((errno == EINTR) & (ret < 0)) {
                continue;
            }

            close(fd);
            return -1;
        }

        trav += chunk;
        left -= chunk;
    }

    close(fd);
    return 0;
}

#if STANDALONE_TEST

int main(int argc, char **argv)
{
    const u64_8 foo_vec = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};
    const u64_8 bar_vec = TSC_SLOPPY() << foo_vec;

    const unsigned n = VEC_LANES(bar_vec);
    unsigned i;
    printf("bar_vec = { ");

    for (i = 0; i < n; i++) {
        printf("[%2u] = 0x%016llx ", i, bar_vec[i]);
    }

    printf("}\n");
    return 0;
}

#endif /* STANDALONE_TEST */
