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

#if STANDALONE_TEST

typedef struct {
    u32 magic;              // (0xa1b2c3d4 for usec res., 0xa1b23c4d for nsec res)
    u16 v_major, v_minor;   // 2.4
    i32 tz_adj;             // timezone adjust
    u32 sigfigs;            // ignore
    u32 snaplen;            // maximum packet bytes stored per packet
    u32 network_encap;      // 1 = ethernet
} pcap_file_hdr_t;

typedef struct {
    u32 ts_sec, ts_frac;    // Seconds and usec or nsec depending
    u32 incl_len, orig_len; // Included length and original length
    u8 pkt[0];              // packet...
} pcap_pkt_hdr_t;

/*
 * Given a pointer to and length of a raw pcap file loaded into memory, this function will
 * (if hdrs != NULL and nhdrs > 0) populate an array of pointers to the pcap packet headers
 * in the file (until it runs out of space in hdrs).
 * Whether or not hdrs and nhdrs are supplied, the function returns the number of packets in
 * the pcap file or -1 if it cannot understand the file.
 */
int get_pcap_pkt_hdrs(const u8 * const data, const size_t len, const pcap_pkt_hdr_t **hdrs,
                      const unsigned nhdrs)
{
    if ((data == NULL) || (len < (sizeof(pcap_file_hdr_t) + sizeof(pcap_pkt_hdr_t)))) {
        return -1;
    }

    const u8 * trav = data;
    i64 left = len;

    const pcap_file_hdr_t * const fhdr = (pcap_file_hdr_t *)data;

    if ((fhdr->magic != 0xa1b2c3d4) & (fhdr->magic != 0xa1b23c4d)) {
        return -1;
    }

    const unsigned max_frac = (fhdr->magic != 0xa1b23c4d) ? 999999999 : 999999;

    if ((fhdr->v_major != 2) | (fhdr->network_encap != 1)) {
        return -1;
    }

    trav += sizeof(*fhdr);
    left -= sizeof(*fhdr);

    const u32 snap = fhdr->snaplen;
    int count = 0;

    while(left >= sizeof(pcap_pkt_hdr_t)) {
        const pcap_pkt_hdr_t * const hdr = (pcap_pkt_hdr_t *)trav;

        if ((hdrs != NULL) & (count < nhdrs)) {
            hdrs[count] = hdr;
        }

        trav += sizeof(pcap_pkt_hdr_t);
        left -= sizeof(pcap_pkt_hdr_t);

        if (((snap != 0) & (hdr->incl_len > snap)) |
                (hdr->incl_len > left) | (hdr->ts_frac > max_frac)) {
            break;
        }

        count++;
        trav += hdr->incl_len;
        left -= hdr->incl_len;
    }

    return count;
}

static void usage(const char *pn)
{
    printf("Usage:\n\t%s fname [size [addr [flags]]]\n", pn);
    printf("\tWhere fname is the file to map or \"\" for an anonymous mapping.\n");
    printf("\tSpecifying non-zero size implies the SEG_DESC_CREATE flag.\n");
    printf("\tSpecifying a non-NULL address implies SEG_DESC_ADDR_HINT.\n");
    printf("\tAvailable flags available in arg 4 are:\n");
    printf("\t\t F --> SEG_DESC_ADDR_FIXED\n");
    printf("\t\t U --> SEG_DESC_UNLINK\n");
    printf("\t\t R --> SEG_DESC_RO\n");
    exit(1);
}

int main(int argc, char **argv)
{
    char errbuf[1024];
    seg_desc_t _seg = {};
    seg_desc_t *seg = &_seg;
    char *path;

    if ((argc < 2) | (argc > 5)) {
        usage(argv[0]);
    }

    path = argv[1];

    if (path[0] == '\0') {
        path = NULL;
    }

    if (argc >= 3) {
        seg->maplen = strtoul(argv[2], NULL, 0);
        seg->flags |= (seg->maplen != 0) ? (SEG_DESC_CREATE | SEG_DESC_INITD) : 0;
    }

    if (argc >= 4) {
        seg->ptr = (void *) strtoul(argv[3], NULL, 0);
        seg->flags |= (seg->ptr != NULL) ? (SEG_DESC_ADDR_HINT | SEG_DESC_INITD) : 0;
    }

    if (argc == 5) {
        const char *trav = argv[4];
        seg->flags |= SEG_DESC_INITD;

        while (1) {
            const char flg = *(trav++);

            if (flg == '\0') {
                break;
            }

            switch(flg) {
                case 'F':
                    seg->flags |= SEG_DESC_ADDR_FIXED;
                    break;

                case 'U':
                    seg->flags |= SEG_DESC_UNLINK;
                    break;

                case 'R':
                    seg->flags |= SEG_DESC_RO;
                    break;

                default:
                    printf("Unknown flag character '%c'\n", flg);
                    exit(1);
            }
        }
    }

    int ret = map_segment(path, seg, errbuf, sizeof(errbuf));

    printf("%d -- %s\n", ret, errbuf);

    ret = unmap_segment(seg);

    printf("%d -- %s\n", ret, ret ? "Unmap failed" : "Unmap OK");

    struct stat st;

    if (stat("/home/lars/test_data.pcap", &st)) {
        printf("barf\n");
        return 2;
    }

    u8 *data = (u8 *)malloc(st.st_size);
    int fd = open("/home/lars/test_data.pcap", O_RDONLY);
    ret = read(fd, data, st.st_size);

    if (ret != st.st_size) {
        printf("puke\n");
        return 2;
    }

    close(fd);

    const pcap_pkt_hdr_t *hdrs[512];

    ret = get_pcap_pkt_hdrs(data, st.st_size, hdrs, sizeof(hdrs) / sizeof(hdrs[0]));
    int i;

    for (i = 0; i < ret; i++) {
        printf("hdrs[i] = %p (plen = %u)\n", hdrs[i], hdrs[i]->incl_len);
    }

    return 0;
}

#endif /* STANDALONE_TEST */
