#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>

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
void OPT_SIZE _debug_print_vec( const void *data, const unsigned nlanes,
                                const unsigned lanesize, const char *name,
                                const unsigned long long mask, FILE *fout)
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

    fprintf(fout, "%s = {", name);

    for (i = 0; i < nlanes; i++) {
        if (!((mask >> i) & 1)) {
            continue;
        }

        const char *pfx = first ? " " : ", ";
        first = 0;

        switch (lanesize) {
            case 1:
                fprintf(fout, "%s[%u]=0x%02x", pfx, i, ((const unsigned char *)data)[i]);
                break;

            case 2:
                fprintf(fout, "%s[%u]=0x%04x", pfx, i, ((const unsigned short *)data)[i]);
                break;

            case 4:
                fprintf(fout, "%s[%u]=0x%08x", pfx, i, ((const unsigned int *)data)[i]);
                break;

            case 8:
                fprintf(fout, "%s[%u]=0x%016llx", pfx, i,
                        ((const unsigned long long *)data)[i]);
                break;

            default:
                break;
        }
    }

    fprintf(fout, " }\n");
}

int OPT_SIZE randomize_data(void * const RESTR data, const size_t len)
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

int load_data(void * const RESTR data, const size_t len, const char *fname)
{
    const unsigned max_chunk = (1 << 21);
    char * RESTR trav = (char *)data;
    size_t left = len;

    int ret;
    const int fd = open(fname, O_RDONLY);

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

int OPT_SIZE get_page_size(const char *path)
{
    struct stat st = {};

    if (stat(path, &st)) {
        return -1;
    }

    return (st.st_blksize <= PAGE_SIZE) ? PAGE_SIZE : st.st_blksize;
}

int OPT_SIZE map_segment(const char *path, seg_desc_t *seg, char *errbuf, const unsigned eblen)
{
    char dirtmp[1024] = {};
    char *dir;
    int fd = -1, ret = 0;
    struct stat st = {};

    if (eblen && !errbuf) {
        return -1;
    } else if (errbuf) {
        memset(errbuf, 0, eblen);
    }

    if (seg == NULL) {
        if (errbuf) {
            snprintf(errbuf, eblen - 1, "seg must not be NULL.");
        }

        return -1;
    }

    if (!(seg->flags & SEG_DESC_INITD)) {
        memset(seg, 0, sizeof(*seg));

        if (path == NULL) {
            if (errbuf) {
                snprintf(errbuf, eblen - 1, "path OR initialized seg must be specified.");
            }

            return -1;
        }

        if (access(path, R_OK)) {
            if (errbuf) {
                const typeof(errno) err_tmp = errno;
                snprintf(errbuf, eblen - 1, "Cannot read file %s: %s", path, strerror(err_tmp));
            }

            return -1;
        }

        seg->flags = SEG_DESC_INITD;

        if (access(path, W_OK)) {
            seg->flags |= SEG_DESC_RO;
        }
    }


    if (path) {
        if (seg->flags & SEG_DESC_CREATE) {
            if (stat(path, &st) == 0) {
                unlink(path);
            }

            strncpy(dirtmp, path, sizeof(dirtmp) - 1);
            dir = dirname(dirtmp);
            ret = stat(dir, &st);

            if (ret < 0) {
                const typeof(errno) err_tmp = errno;
                snprintf(errbuf, eblen - 1, "Cannot stat directory %s: %s",
                         dirtmp, strerror(err_tmp));
                return -1;
            }

            const u64 psize = (st.st_blksize <= PAGE_SIZE) ? PAGE_SIZE : st.st_blksize;
            const u64 pmask = psize - 1;
            seg->maplen = (seg->maplen + pmask) & ~pmask;

            fd = open(path, O_CREAT | O_RDWR, 0600);

            if (fd < 0) {
                if (errbuf) {
                    const typeof(errno) err_tmp = errno;
                    snprintf(errbuf, eblen - 1, "Cannot create file %s: %s",
                             path, strerror(err_tmp));
                }

                return -1;
            }

            ret = posix_fallocate(fd, 0, seg->maplen);

            if (ret < 0) {
                if (errbuf) {
                    const typeof(errno) err_tmp = errno;
                    snprintf(errbuf, eblen - 1, "Cannot allocate file %s to size 0x%lx: %s",
                             path, seg->maplen, strerror(err_tmp));
                }

                return -1;
            }
        } else {
            fd = open(path, (seg->flags & SEG_DESC_RO) ? O_RDONLY : O_RDWR);

            if (fd < 0) {
                if (errbuf) {
                    const typeof(errno) err_tmp = errno;
                    snprintf(errbuf, eblen - 1, "Cannot open file %s for %s: %s",
                             path, (seg->flags & SEG_DESC_RO) ? "read" : "read+write",
                             strerror(err_tmp));
                }

                return -1;
            }
        }

        ret = fstat(fd, &st);
        seg->psize = (st.st_blksize <= PAGE_SIZE) ? PAGE_SIZE : st.st_blksize;
        seg->maplen = st.st_size & ~(seg->psize - 1);

        if (ret < 0) {
            if (errbuf) {
                const typeof(errno) err_tmp = errno;
                snprintf(errbuf, eblen - 1, "Cannot stat file %s: %s",
                         path, strerror(err_tmp));
            }

            return -1;
        }

        if (seg->flags & SEG_DESC_UNLINK) {
            unlink(path);
        }
    } else {
        seg->flags |= SEG_DESC_ANON;
    }

    const int req_prot = PROT_READ | ((seg->flags & SEG_DESC_RO) ? 0 : PROT_WRITE);
    const int req_flags = MAP_SHARED | ((path == NULL) ? MAP_ANON : 0) | MAP_POPULATE |
                          ((seg->flags & SEG_DESC_ADDR_FIXED) ? MAP_FIXED : 0);

    void * const req_ptr = (seg->flags & (SEG_DESC_ADDR_FIXED | SEG_DESC_ADDR_HINT)) ?
                           seg->ptr : NULL;

    seg->ptr = mmap(req_ptr, seg->maplen, req_prot, req_flags, fd, 0);

    if (seg->ptr == MAP_FAILED) {
        if (errbuf) {
            const typeof(errno) err_tmp = errno;
            snprintf(errbuf, eblen - 1, "Cannot mmap() file %s from 0 to 0x%lx: %s",
                     path, seg->maplen, strerror(err_tmp));
        }

        seg->ptr = NULL;
    }

    if (fd >= 0) {
        close(fd);
    }

    if (seg->ptr == NULL) {
        if (errbuf && !*errbuf) {
            snprintf(errbuf, eblen - 1, "Something went sideways: "
                     "prot=%s maplen=0x%lx psize=0x%lx vaddr=%p flags=0x%lx",
                     (seg->flags & SEG_DESC_RO) ? "RO" : "RW",
                     seg->maplen, seg->psize, seg->ptr, seg->flags);
        }

        return -1;
    } else {
        if (errbuf) {
            snprintf(errbuf, eblen - 1, "OK: prot=%s"
                     " maplen=0x%lx psize=0x%lx vaddr=%p flags=0x%lx",
                     (seg->flags & SEG_DESC_RO) ? "RO" : "RW",
                     seg->maplen, seg->psize, seg->ptr, seg->flags);
        }

        return 0;
    }
}

int OPT_SIZE unmap_segment(const seg_desc_t * const seg)
{
    if (seg == NULL) {
        return -1;
    }

    return munmap(seg->ptr, seg->maplen);
}
