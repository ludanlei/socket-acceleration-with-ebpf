#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/swab.h>

#ifndef __section
#define __section(NAME) 	\
	__attribute__((section(NAME), used))
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define __bpf_ntohl(x)                 __builtin_bswap32(x)
# define __bpf_htonl(x)                 __builtin_bswap32(x)
# define __bpf_constant_ntohl(x)        ___constant_swab32(x)
# define __bpf_constant_htonl(x)        ___constant_swab32(x)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define __bpf_ntohl(x)                 (x)
# define __bpf_htonl(x)                 (x)
# define __bpf_constant_ntohl(x)        (x)
# define __bpf_constant_htonl(x)        (x)
#else
# error "Check the compiler's endian detection."
#endif

#define bpf_htonl(x)                            \
        (__builtin_constant_p(x) ?              \
         __bpf_constant_htonl(x) : __bpf_htonl(x))
#define bpf_ntohl(x)                            \
        (__builtin_constant_p(x) ?              \
         __bpf_constant_ntohl(x) : __bpf_ntohl(x))

#ifndef FORCE_READ
#define FORCE_READ(X) (*(volatile typeof(X)*)&X)
#endif

#ifndef BPF_FUNC
#define BPF_FUNC(NAME, ...) 	\
	(*NAME)(__VA_ARGS__) = (void *) BPF_FUNC_##NAME
#endif

#ifndef printk
# define printk(fmt, ...)                                      \
    ({                                                         \
        char ____fmt[] = fmt;                                  \
        trace_printk(____fmt, sizeof(____fmt), ##__VA_ARGS__); \
    })
#endif

/* ebpf helper function
 * The generated function is used for parameter verification
 * by the eBPF verifier
 */
static int BPF_FUNC(msg_redirect_hash, struct sk_msg_md *md,
			void *map, void *key, __u64 flag);
static int BPF_FUNC(sock_hash_update, struct bpf_sock_ops *skops,
			void *map, void *key, __u64 flags);
static void BPF_FUNC(trace_printk, const char *fmt, int fmt_size, ...);

struct sock_key {
	__u32 sip4;
	__u32 dip4;
	__u8  family;
	__u8  pad1;   // this padding required for 64bit alignment
	__u16 pad2;   // else ebpf kernel verifier rejects loading of the program
	__u32 pad3;
	__u32 sport;
	__u32 dport;
} __attribute__((packed));

struct {
	__uint(type, BPF_MAP_TYPE_SOCKHASH);
	__uint(max_entries, 65535);
	__uint(map_flags, 0);
	__type(key, struct sock_key);
	__type(value, int);
} sock_ops_map SEC(".maps");
