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

int get_page_size(const char *path)
{
    struct stat st = {};

    if (stat(path, &st)) {
        return -1;
    }

    return (st.st_blksize <= PAGE_SIZE) ? PAGE_SIZE : st.st_blksize;
}

seg_desc_t map_segment(const char *path)
{
    seg_desc_t tmp = {};
    const int fd = open(path, O_RDWR);

    if (fd < 0) {
        return tmp;
    }

    struct stat st = {};

    fstat(fd, &st);

    tmp.psize = (st.st_blksize <= PAGE_SIZE) ? PAGE_SIZE : st.st_blksize;

    tmp.maplen = st.st_size & ~(tmp.psize - 1);

    tmp.ptr = mmap(NULL, tmp.maplen, PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_POPULATE, fd, 0);

    if (tmp.ptr == MAP_FAILED) {
        tmp.ptr = NULL;
    }

    close(fd);
    return tmp;
}

#if STANDALONE_TEST

int main(int argc, char **argv)
{
    seg_desc_t seg = map_segment("/dev/hugepages/foobar");

    printf("ptr = %p  maplen = 0x%lx psize = 0x%lx\n", seg.ptr, seg.maplen, seg.psize);
    return 0;
}

#endif /* STANDALONE_TEST */
