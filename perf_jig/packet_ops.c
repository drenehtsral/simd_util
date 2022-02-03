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
#include <arpa/inet.h>

#include "../include/simd_util.h"

#include "perf_jig.h"

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

typedef struct {
    u8 dst[6];
    u8 src[6];
    u16 et;
    u8 next[0];
} eth_hdr_t;

typedef struct {
    u16 tag;
    u16 et;
    u8 next[0];
} dot_q_t;

typedef struct {
    u8  ihl : 4,
    ver : 4;
    u8  ecn : 2,
    dscp : 6;
    u16 total_len;
    u16 ident;
    u16 flags_offs;
    u8  ttl, proto;
    u16 csum;
    u32 src, dst;
    u32 opts[0];
} ip4_hdr_t;

typedef struct {
    u16     hw_type;
    u16     l3_type;
    u8      hw_size;
    u8      l3_size;
    u16     opcode;
    u8      sender_mac[6];
    u32     sender_ip;
    u8      target_map[6];
    u32     target_ip;
} PACKED arp_eth_ip4_t;

typedef struct {
    u32     tc_h : 4,
            ver : 4,
            tc_l : 4,
            label : 20;
    u16     paylen;
    u8      nexthdr, hoplim;
    u32     src[4];
    u32     dst[4];
    u8      next[0];
} ip6_hdr_t;

typedef struct {
    u8      nexthdr, optlen;
    u8      data[6];
    u8      next[0];
} ip6_generic_opt_t;

typedef struct {
    u16 sport, dport, len, csum;
    u8 next[0];
} udp_hdr_t;

typedef struct {
    u16 sport, dport;
    u32 seq, ack;
    u16 doff_flags, wsize;
    u16 csum, urg;
    u8 next[0];
} tcp_hdr_t;

#define CONST_HTONS(x) (((x >> 8) & 0xFF) | ((x << 8) & 0xFF00))

#define HW_ETHERNET (0x0001)

#define ET_CVLAN    (0x8100)
#define ET_SVLAN    (0x88A8)
#define ET_IP4      (0x0800)
#define ET_ARP      (0x0806)
#define ET_IP6      (0x86DD)

#define L4T_ICMP    (0x01)
#define L4T_TCP     (0x06)
#define L4T_UDP     (0x11)
#define L4T_SCTP    (0x84)

static u8 ip6_opt_hdr_flags[256] = {
    [0] = 0x01, [43] = 0x01, [44] = 0x01, [50] = 0x01, [51] = 0x01, [60] = 0x01, [135] = 0x01, [139] = 0x01, [140] = 0x01, [253] = 0x01, [254] = 0x01,
};

typedef union {
    u32_4   u32_4;
    u16_8   u16_8;
    u32     u32[4];
    u16     u16[8];
} l3_addr_t;

#define MD_PROTO_L2_CTAG    (1 << 0)
#define MD_PROTO_L2_STAG    (1 << 1)

#define MD_PROTO_L3_IP4     (1 << 8)
#define MD_PROTO_L3_IP6     (2 << 8)
#define MD_PROTO_L3_ARP     (3 << 8)

#define MD_PROTO_L4_TCP     (1 << 16)
#define MD_PROTO_L4_UDP     (2 << 16)
#define MD_PROTO_L4_SCTP    (3 << 16)
#define MD_PROTO_L4_ICMP    (4 << 16)

typedef union {
    u32_16  zmm;
    struct {
        // Flow Key Fields
        l3_addr_t               src_ip;
        l3_addr_t               dst_ip;
        u16                     src_port, dst_port;
        u16                     c_tag, s_tag;
        u32                     proto_flags;
        // Other Metadata fields
        const pcap_pkt_hdr_t   *pph;
        u16                     offs_vlan;
        u16                     offs_l3;
        u16                     offs_l4;
        u16                     offs_payload;
    };
} pkt_metadata_t;

static const pkt_metadata_t flow_key_mask = {
    .src_ip.u32 = {~0U, ~0U, ~0U, ~0U },
    .dst_ip.u32 = {~0U, ~0U, ~0U, ~0U },
    .src_port = ~((u16)0), .dst_port = ~((u16)0),
    .c_tag = ~((u16)0), .s_tag = ~((u16)0),
    .proto_flags = ~0U,
};

/*
 * XXX: TLDR: Work-in-progress / half-baked idea...
 *
 *    This function is all sorts of janky and shouldn't yet be used for anything important.
 * Eventually, if cleaned up and made more complete and robust, it could be used as a
 * reference (single lane / non parallel) implementation against which to test a SIMD
 * parallel packet digestor function for use as part of a SIMD parallel packet processing
 * state machine.
 *    Any such implementation intended for real-world use would generally differ in a couple
 * of important ways however (the specifics of which would depend entirely on the use case
 * itself).  The general gist of these difference would be:
 *    == It's likely that the network adaptor would be used to perform some parsing, validation,
 *       and steering including:
 *      = Extracting fields from the packet headers into the packet descriptor on Rx.
 *      = Checksum validation for L2-->L4
 *      = Packet steering by some sort of hash (at the least) or potentially hash + listener.
 *    == It's likely that an application-specific system could drop (or at least shunt aside to
 *       a resource-limited slow path) any packets which did not pass a basic sanity check.  For
 *       example, an application that handled queries submitted via UDP and replied via UDP
 *       might legitimately shunt ARP, ICMP, TCP, etc. to an exception-path queue.
 *    == If the application expects to use SIMD parallel processing at some level, and maintains
 *       coherent state for the traffic it's handling, it may be advantageous to separate traffic
 *       into independent streams early in the process (either in hardware by hashing flow keys,
 *       or at a suitably early point in the software processing).  Depending on the application,
 *       traffic might be separated at different layers, and at different points in the pipeline.
 *       This can be done as some combination of batching like operations (for improved batch
 *       coherence) and/or by batching unlike items for reduced lane-to-lane conflicts:
 *      = By remote host (bin by IP address at packet level)
 *      = By flow (bin by 5-tuple at packet level)
 *      = By connection (bin by 5-tuple in either direction at the packet level)
 *      = By session (bin by some token of session membership higher up the stack).
 *      = By resource (bin by the nature of the addressed resource higher up the stack).
 *      = By operation (bin by the nature of the operation requested wherever in the stack this
 *        divergence can be detected).
 *    == What it means to "safely" handle any particular packet very much depends on the context of
 *       the use case, the nature of the data in play, and the threat models resulting.  This shapes
 *       how each stage of parsing and validation is best considered.
 *    For these reasons, I wish to remind the reader (and myself) to think carefully about the
 * use case at hand and what information is critical to parse out and use in deciding how to steer
 * each packet.  It is also important to ensure that the expected behavior for any arbitrary packet
 * (including malformed or malicious packets) is well defined and can be reasoned about.  (Even if
 * it just means shunting them to an exception queue for slow-path processing).
 */
int gen_pkt_metadata(const pcap_pkt_hdr_t * const RESTR pph, pkt_metadata_t * const RESTR md)
{
    const u8 *next = pph->pkt;
    const u8 * const pkt = next;
    const unsigned upper_bound = (pph->incl_len < pph->orig_len) ? pph->incl_len : pph->orig_len;

    md->zmm &= 0;
    md->pph = pph;
    md->offs_vlan = md->offs_l3 = md->offs_l4 = md->offs_payload = (u16) ~0U;

    int out_of_bounds(const unsigned extent)
    {
        const unsigned cur = next - pkt;
        const unsigned end = cur + extent;
        return (end > upper_bound);
    }

    if (out_of_bounds(sizeof(eth_hdr_t))) {
        return -1;
    }

    const eth_hdr_t *eh = (eth_hdr_t *)pkt;

    const u16 *et = &eh->et;
    next = eh->next;

    while ((*et == CONST_HTONS(ET_CVLAN)) | (*et == CONST_HTONS(ET_SVLAN))) {
        if (out_of_bounds(sizeof(dot_q_t))) {
            return -1;
        }

        const dot_q_t *q = (dot_q_t *)next;

        if (*et == CONST_HTONS(ET_CVLAN)) {
            md->c_tag = q->tag;
            md->proto_flags |= MD_PROTO_L2_CTAG;
        } else {
            md->s_tag = q->tag;
            md->proto_flags |= MD_PROTO_L2_STAG;
        }

        md->offs_vlan = next - pkt;
        printf("\tvlan at offset 0x%x\n", md->offs_vlan);
        et = &q->et;
        next = q->next;
    }

    u8 l4_type = 0xFF;
    const u16 l3_type = *et;

    switch(l3_type) {
        case CONST_HTONS(ET_IP4):
            {
                if (out_of_bounds(sizeof(ip4_hdr_t))) {
                    return -1;
                }

                const ip4_hdr_t * const ip4 = (const ip4_hdr_t *)next;

                if (ip4->ver != 4) {
                    return -1;
                }

                md->offs_l3 = next - pkt;
                md->proto_flags |= MD_PROTO_L3_IP4;
                md->src_ip.u32[0] = ip4->src;
                md->dst_ip.u32[0] = ip4->dst;
                printf("\tL3 at offset 0x%x -- IPv4\n", md->offs_l3);

                const unsigned optlen = ip4->ihl * sizeof(u32);

                if (out_of_bounds(optlen)) {
                    return -1;
                }

                next += optlen;
                l4_type = ip4->proto;
                break;
            }

        case CONST_HTONS(ET_IP6):
            {
                if (out_of_bounds(sizeof(ip6_hdr_t))) {
                    return -1;
                }

                const ip6_hdr_t * const ip6 = (const ip6_hdr_t *)next;

                if (ip6->ver != 6) {
                    return -1;
                }

                unsigned i;
                md->offs_l3 = next - pkt;
                md->proto_flags |= MD_PROTO_L3_IP4;

                for (i = 0; i < 4; i++) {
                    md->src_ip.u32[i] = ip6->src[i];
                    md->dst_ip.u32[i] = ip6->dst[i];
                }

                printf("\tL3 at offset 0x%x -- IPv4\n", md->offs_l3);
                printf("\tlabel = 0x%05x\n", ip6->label); // XXX: Broken bit field / endianness
                l4_type = ip6->nexthdr;
                next = ip6->next;

                while (!out_of_bounds(sizeof(ip6_generic_opt_t)) && ip6_opt_hdr_flags[l4_type]) {
                    const ip6_generic_opt_t * opt = (const ip6_generic_opt_t *)next;
                    l4_type = opt->nexthdr;
                    next = opt->next + (8 * opt->optlen);
                }

                break;
            }

        case CONST_HTONS(ET_ARP):
            {
                if (out_of_bounds(sizeof(arp_eth_ip4_t))) {
                    return -1;
                }

                const arp_eth_ip4_t * const arp = (const arp_eth_ip4_t *)next;
                md->offs_l3 = next - pkt;

                if ((arp->l3_type != CONST_HTONS(ET_IP4)) | (arp->hw_type != CONST_HTONS(HW_ETHERNET))) {
                    return -1;
                }

                md->proto_flags |= MD_PROTO_L3_ARP;
                next += sizeof(arp_eth_ip4_t);
                printf("\tL3 at offset 0x%x -- ARP\n", md->offs_l3);
                break;
            }

        default:
            break;
    }

    switch(l4_type) {
        case L4T_TCP:
            {
                if (out_of_bounds(sizeof(tcp_hdr_t))) {
                    return -1;
                }

                md->offs_l4 = next - pkt;
                printf("\tL4 at offset %x -- TCP\n", md->offs_l4);
                const tcp_hdr_t * const tcp = (const tcp_hdr_t *)next;

                md->proto_flags |= MD_PROTO_L4_TCP;
                md->src_port = tcp->sport;
                md->dst_port = tcp->dport;

                printf("\tL3 at offset 0x%x -- IPv4\n", md->offs_l3);
                const unsigned opt_len = ((tcp->doff_flags & 0x00F0) >> 2) - sizeof(tcp_hdr_t);

                if (out_of_bounds(opt_len)) {
                    return -1;
                }

                next = tcp->next + opt_len;

                md->offs_payload = next - pkt;
                break;
            }

        case L4T_UDP:
            {
                if (out_of_bounds(sizeof(udp_hdr_t))) {
                    return -1;
                }

                md->offs_l4 = next - pkt;
                printf("\tL4 at offset %x -- UDP\n", md->offs_l4);
                const udp_hdr_t * const udp = (const udp_hdr_t *)next;

                md->proto_flags |= MD_PROTO_L4_UDP;
                md->src_port = udp->sport;
                md->dst_port = udp->dport;

                next = udp->next;

                md->offs_payload = next - pkt;
                break;
            }

        case L4T_ICMP:
            {
                md->offs_l4 = next - pkt;
                printf("\tL4 at offset %x -- ICMP\n", md->offs_l4);
                break;
            }

        default:
            break;
    }

    printf("\tproto_flags = 0x%08x\n", md->proto_flags);
    return 0;
}

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

        if ((hdr->incl_len == 0) & (hdr->orig_len == 0)) {
            break;
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

static int perf_test_pcap_load(const char **args)
{
    char errbuf[1024] = {};
    seg_desc_t pcap_seg = {};
    seg_desc_t wlist_seg = {.maplen = HUGE_2M_SIZE, .psize = HUGE_2M_SIZE, .flags = SEG_DESC_INITD | SEG_DESC_ANON};

    if (args[1] == NULL) {
        printf("%s: file is required.\n", args[0]);
        return -1;
    }

    if (map_segment(args[1], &pcap_seg, errbuf, sizeof(errbuf) - 1)) {
        printf("%s: %s\n", args[0], errbuf);
        return -1;
    }

    printf("%s\n", errbuf);

    if (map_segment(NULL, &wlist_seg, errbuf, sizeof(errbuf) - 1)) {
        unmap_segment(&pcap_seg);
        printf("%s: %s\n", args[0], errbuf);
        return -1;
    }

    printf("%s\n", errbuf);

    const pcap_pkt_hdr_t **hdrs = (const pcap_pkt_hdr_t **)wlist_seg.ptr;
    const int max_ent = wlist_seg.maplen / sizeof(void *);
    const int num_ent_ret = get_pcap_pkt_hdrs((const u8 *)pcap_seg.ptr, pcap_seg.maplen, hdrs, max_ent);
    const int num_ent = (num_ent_ret < max_ent) ? num_ent_ret : max_ent;
    int i;
    pkt_metadata_t foo_md;

    for (i = 0; i < num_ent; i++) {
        printf("%d: %p/%u/%u\n", i + 1, hdrs[i]->pkt, hdrs[i]->incl_len, hdrs[i]->orig_len);
        gen_pkt_metadata(hdrs[i], &foo_md);
    }

    unmap_segment(&pcap_seg);
    unmap_segment(&wlist_seg);

    debug_print_vec(flow_key_mask.zmm, ~0);
    return 0;
}

PERF_FUNC_ENTRY(pcap_load, "Load pcap file", "file");
