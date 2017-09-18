/******************************************************************************
* Copyright (c) 2004, 2015  Ericsson AB
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html

******************************************************************************/
///////////////////////////////////////////////////////////////////////////////
//
//  File:               TCCInterface_ip.h
//  Description:        TCC Useful Functions: Interface Functions
//  Rev:                R22B
//  Prodnr:             CNL 113 472
//  Updated:            2009-02-02
//  Contact:            http://ttcn.ericsson.se
//
///////////////////////////////////////////////////////////////////////////////

/*
  Code here is from iproute distributed under GPL
*/

#include <linux/posix_types.h>
#include <asm/types.h>
#include <linux/types.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define RTM_NEWADDR	20
#define RTM_DELADDR	21
#define NLM_F_CREATE	0x400	/* Create, if it does not exist	*/
#define NLM_F_EXCL	0x200	/* Do not touch, if it exists	*/
#define NLM_F_ACK		4	/* Reply with ack, with zero or error code */
#define NETLINK_ROUTE		0	/* Routing/device hook				*/
#define PREFIXLEN_SPECIFIED 1

#define NLMSG_ALIGNTO	4
#define NLMSG_ALIGN(len) ( ((len)+NLMSG_ALIGNTO-1) & ~(NLMSG_ALIGNTO-1) )
#define NLMSG_HDRLEN	 ((int) NLMSG_ALIGN(sizeof(struct nlmsghdr)))
#define NLMSG_LENGTH(len) ((len)+NLMSG_ALIGN(NLMSG_HDRLEN))
#define NLMSG_DATA(nlh)  ((void*)(((char*)nlh) + NLMSG_LENGTH(0)))
#define RTA_ALIGNTO	4
#define RTA_ALIGN(len) ( ((len)+RTA_ALIGNTO-1) & ~(RTA_ALIGNTO-1) )
#define RTA_LENGTH(len)	(RTA_ALIGN(sizeof(struct rtattr)) + (len))

#define NLMSG_TAIL(nmsg) \
	((struct rtattr *) (   ((char *)(nmsg)) + (NLMSG_ALIGN((nmsg)->nlmsg_len))  ))

#define RTA_LENGTH(len)	(RTA_ALIGN(sizeof(struct rtattr)) + (len))
#define RTA_DATA(rta)   ((void*)(((char*)(rta)) + RTA_LENGTH(0)))

int preferred_family = AF_UNSPEC;

enum
{
	IFA_UNSPEC,
	IFA_ADDRESS,
	IFA_LOCAL,
	IFA_LABEL,
	IFA_BROADCAST,
	IFA_ANYCAST,
	IFA_CACHEINFO,
	IFA_MULTICAST,
	__IFA_MAX,
};

enum rt_scope_t
{
	RT_SCOPE_UNIVERSE=0,
/* User defined values  */
	RT_SCOPE_SITE=200,
	RT_SCOPE_LINK=253,
	RT_SCOPE_HOST=254,
	RT_SCOPE_NOWHERE=255
};

struct rtattr
{
	unsigned short	rta_len;
	unsigned short	rta_type;
};

struct sockaddr_nl
{
	sa_family_t	nl_family;	/* AF_NETLINK	*/
	unsigned short	nl_pad;		/* zero		*/
	__u32		nl_pid;		/* port ID	*/
       	__u32		nl_groups;	/* multicast groups mask */
};

struct nlmsghdr
{
	__u32		nlmsg_len;	/* Length of message including header */
	__u16		nlmsg_type;	/* Message content */
	__u16		nlmsg_flags;	/* Additional flags */
	__u32		nlmsg_seq;	/* Sequence number */
	__u32		nlmsg_pid;	/* Sending process port ID */
};

struct ifaddrmsg
{
	__u8		ifa_family;
	__u8		ifa_prefixlen;	/* The prefix length		*/
	__u8		ifa_flags;	/* Flags			*/
	__u8		ifa_scope;	/* Address scope		*/
	__u32		ifa_index;	/* Link index			*/
};

typedef struct
{
	__u8 family;
	__u8 bytelen;
	__s16 bitlen;
	__u32 flags;
	__u32 data[4];
} inet_prefix;

struct ifa_cacheinfo
{
	__u32	ifa_prefered;
	__u32	ifa_valid;
	__u32	cstamp; /* created timestamp, hundredths of seconds */
	__u32	tstamp; /* updated timestamp, hundredths of seconds */
};

#ifndef	INFINITY_LIFE_TIME
#define     INFINITY_LIFE_TIME      0xFFFFFFFFU
#endif

struct rtgenmsg
{
	unsigned char		rtgen_family;
};

struct rtnl_handle
{
	int			fd;
	struct sockaddr_nl	local;
	struct sockaddr_nl	peer;
	__u32			seq;
	__u32			dump;
};

struct rtnl_handle rth = { -1 };

typedef int (*rtnl_filter_t)(const struct sockaddr_nl *,
			     struct nlmsghdr *n, void *);


#define NLM_F_ROOT	0x100	/* specify tree	root	*/
#define NLM_F_MATCH	0x200	/* return all matching	*/
#define NLM_F_REQUEST		1	/* It is request message. 	*/
#define NLMSG_ERROR		0x2	/* Error		*/

struct nlmsgerr
{
	int		error;
	struct nlmsghdr msg;
};

#define DN_MAXADDL 20
struct dn_naddr
{
        unsigned short          a_len;
        unsigned char a_addr[DN_MAXADDL];
};


int rtnl_open_byproto(struct rtnl_handle *rth, unsigned subscriptions,
		      int protocol)
{
	socklen_t addr_len;
	int sndbuf = 32768;
	int rcvbuf = 32768;

	memset(rth, 0, sizeof(*rth));

	rth->fd = socket(AF_NETLINK, SOCK_RAW, protocol);
	if (rth->fd < 0) {
	  TTCN_warning("Cannot open netlink socket");
	  return -1;
	}

	if (setsockopt(rth->fd,SOL_SOCKET,SO_SNDBUF,&sndbuf,sizeof(sndbuf)) < 0) {
	  TTCN_warning("Cannot set SO_SNDBUF");
	  return -1;
	}

	if (setsockopt(rth->fd,SOL_SOCKET,SO_RCVBUF,&rcvbuf,sizeof(rcvbuf)) < 0) {
	  TTCN_warning("Cannot set SO_RCVBUF");
	  return -1;
	}

	memset(&rth->local, 0, sizeof(rth->local));
	rth->local.nl_family = AF_NETLINK;
	rth->local.nl_groups = subscriptions;

	if (bind(rth->fd, (struct sockaddr*)&rth->local, sizeof(rth->local)) < 0) {
	  TTCN_warning("Cannot bind netlink socket");
	  return -1;
	}
	addr_len = sizeof(rth->local);
	if (getsockname(rth->fd, (struct sockaddr*)&rth->local, &addr_len) < 0) {
	  TTCN_warning("Cannot getsockname");
	  return -1;
	}
	if (addr_len != sizeof(rth->local)) {
	  //		fprintf(stderr, "Wrong address length %d\n", addr_len);
	  
		return -1;
	}
	if (rth->local.nl_family != AF_NETLINK) {
	  TTCN_warning("Wrong address family!");
	  return -1;
	}
	rth->seq = time(NULL);
	return 0;
}

int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions)
{
  return rtnl_open_byproto(rth, subscriptions, NETLINK_ROUTE);
}

void rtnl_close(struct rtnl_handle *rth)
{
  if (rth->fd >= 0) {
    close(rth->fd);
    rth->fd = -1;
  }
}


static int dnet_num(const char *src, u_int16_t * dst)
{
  int rv = 0;
  int tmp;
  *dst = 0;
  
  while ((tmp = *src++) != 0) {
    tmp -= '0';
    if ((tmp < 0) || (tmp > 9))
      return rv;
    
    rv++;
    (*dst) *= 10;
    (*dst) += tmp;
  }
  
  return rv;
}

static __inline__ u_int16_t dn_htons(u_int16_t addr)
{
        union {
                u_int8_t byte[2];
                u_int16_t word;
        } u;

        u.word = addr;
        return ((u_int16_t)u.byte[0]) | (((u_int16_t)u.byte[1]) << 8);
}

static int dnet_pton1(const char *src, struct dn_naddr *dna)
{
	u_int16_t area = 0;
	u_int16_t node = 0;
	int pos;

	pos = dnet_num(src, &area);
	if ((pos == 0) || (area > 63) || (*(src + pos) != '.'))
		return 0;
	pos = dnet_num(src + pos + 1, &node);
	if ((pos == 0) || (node > 1023))
		return 0;
	dna->a_len = 2;
	*(u_int16_t *)dna->a_addr = dn_htons((area << 10) | node);

	return 1;
}

int dnet_pton(int af, const char *src, void *addr)
{
	int err;

	switch (af) {
	case AF_DECnet:
		errno = 0;
		err = dnet_pton1(src, (struct dn_naddr *)addr);
		break;
	default:
		errno = EAFNOSUPPORT;
		err = -1;
	}

	return err;
}

int get_addr_1(inet_prefix *addr, const char *name, int family)
{
	memset(addr, 0, sizeof(*addr));

	if (strcmp(name, "default") == 0 ||
	    strcmp(name, "all") == 0 ||
	    strcmp(name, "any") == 0) {
		if (family == AF_DECnet)
			return -1;
		addr->family = family;
		addr->bytelen = (family == AF_INET6 ? 16 : 4);
		addr->bitlen = -1;
		return 0;
	}

	if (strchr(name, ':')) {
		addr->family = AF_INET6;
		if (family != AF_UNSPEC && family != AF_INET6)
			return -1;
		if (inet_pton(AF_INET6, name, addr->data) <= 0)
			return -1;
		addr->bytelen = 16;
		addr->bitlen = -1;
		return 0;
	}

	if (family == AF_DECnet) {
		struct dn_naddr dna;
		addr->family = AF_DECnet;
		if (dnet_pton(AF_DECnet, name, &dna) <= 0)
			return -1;
		memcpy(addr->data, dna.a_addr, 2);
		addr->bytelen = 2;
		addr->bitlen = -1;
		return 0;
	}

	addr->family = AF_INET;
	if (family != AF_UNSPEC && family != AF_INET)
		return -1;
	if (inet_pton(AF_INET, name, addr->data) <= 0)
		return -1;
	addr->bytelen = 4;
	addr->bitlen = -1;
	return 0;
}

int get_unsigned(unsigned *val, const char *arg, int base)
{
	unsigned long res;
	char *ptr;

	if (!arg || !*arg)
		return -1;
	res = strtoul(arg, &ptr, base);
	if (!ptr || ptr == arg || *ptr || res > UINT_MAX)
		return -1;
	*val = res;
	return 0;
}

int mask2bits(__u32 netmask)
{
	unsigned bits = 0;
	__u32 mask = ntohl(netmask);
	__u32 host = ~mask;

	/* a valid netmask must be 2^n - 1 */
	if ((host & (host + 1)) != 0)
		return -1;

	for (; mask; mask <<= 1)
		++bits;
	return bits;
}

static int get_netmask(unsigned *val, const char *arg, int base)
{
	inet_prefix addr;

	if (!get_unsigned(val, arg, base))
		return 0;

	/* try coverting dotted quad to CIDR */
	if (!get_addr_1(&addr, arg, AF_INET) && addr.family == AF_INET) {
		int b = mask2bits(addr.data[0]);
		
		if (b >= 0) {
			*val = b;
			return 0;
		}
	}

	return -1;
}

int get_prefix_1(inet_prefix *dst, char *arg, int family)
{
	int err;
	unsigned plen;
	char *slash;

	memset(dst, 0, sizeof(*dst));

	if (strcmp(arg, "default") == 0 ||
	    strcmp(arg, "any") == 0 ||
	    strcmp(arg, "all") == 0) {
		if (family == AF_DECnet)
			return -1;
		dst->family = family;
		dst->bytelen = 0;
		dst->bitlen = 0;
		return 0;
	}

	slash = strchr(arg, '/');
	if (slash)
		*slash = 0;

	err = get_addr_1(dst, arg, family);
	if (err == 0) {
		switch(dst->family) {
			case AF_INET6:
				dst->bitlen = 128;
				break;
			case AF_DECnet:
				dst->bitlen = 16;
				break;
			default:
			case AF_INET:
				dst->bitlen = 32;
		}
		if (slash) {
			if (get_netmask(&plen, slash+1, 0)
					|| (signed)plen > dst->bitlen) {
				err = -1;
				goto done;

			}
			dst->flags |= PREFIXLEN_SPECIFIED;
			dst->bitlen = plen;
		}
	}
done:
	if (slash)
		*slash = '/';
	return err;
}


int get_prefix(inet_prefix *dst, char *arg, int family)
{
	if (family == AF_PACKET) {
	  TTCN_warning("This inet prefix is not allowed in this context");
	  return -1;
	}
	if (get_prefix_1(dst, arg, family)) {
	  TTCN_warning("inet prefix is expected in this context");
	  return -1;
	}
	return 0;
}

int addattr_l(struct nlmsghdr *n, int maxlen, int type, const void *data,
	      int alen)
{
	int len = RTA_LENGTH(alen);
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > (unsigned)maxlen) {
	  return -1; 
	}
	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);
	return 0;
}

extern unsigned int if_nametoindex (const char *);

struct idxmap
{
	struct idxmap * next;
	unsigned	index;
	int		type;
	int		alen;
	unsigned	flags;
	unsigned char	addr[8];
	char		name[16];
};

static struct idxmap *idxmap[16];

unsigned ll_name_to_index(const char *name)
{
	static char ncache[16];
	static int icache;
	struct idxmap *im;
	int i;

	if (name == NULL)
		return 0;
	if (icache && strcmp(name, ncache) == 0)
		return icache;
	for (i=0; i<16; i++) {
		for (im = idxmap[i]; im; im = im->next) {
			if (strcmp(im->name, name) == 0) {
				icache = im->index;
				strcpy(ncache, name);
				return im->index;
			}
		}
	}

	return if_nametoindex(name);
}

static int default_scope(inet_prefix *lcl)
{
	if (lcl->family == AF_INET) {
		if (lcl->bytelen >= 1 && *(__u8*)&lcl->data == 127)
			return RT_SCOPE_HOST;
	}
	return 0;
}

int rtnl_talk(struct rtnl_handle *rtnl, struct nlmsghdr *n, pid_t peer,
	      unsigned groups, struct nlmsghdr *answer,
	      rtnl_filter_t junk,
	      void *jarg)
{
	unsigned status;
	unsigned seq;
	struct nlmsghdr *h;
	struct sockaddr_nl nladdr;
	struct iovec iov = {
	  /*.iov_base =*/ (void*) n,
	  /*.iov_len =*/ n->nlmsg_len
	};

	struct msghdr msg = {
	  /*.msg_name = */&nladdr,
	  /*.msg_namelen =*/ sizeof(nladdr),
	  /*.msg_iov =*/ &iov,
	  /*.msg_iovlen =*/ 1,
	};
	char   buf[16384];

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = peer;
	nladdr.nl_groups = groups;

	n->nlmsg_seq = seq = ++rtnl->seq;

	if (answer == NULL)
		n->nlmsg_flags |= NLM_F_ACK;

	status = sendmsg(rtnl->fd, &msg, 0);

	if (status < 0) {
	  TTCN_warning("Cannot talk to rtnetlink");
	  return -1;
	}

	memset(buf,0,sizeof(buf));

	iov.iov_base = buf;

	while (1) {
		iov.iov_len = sizeof(buf);
		status = recvmsg(rtnl->fd, &msg, 0);

		if (status < 0) {
		  if (errno == EINTR || errno == EAGAIN)
		    continue;
		  TTCN_warning("rtln receives error");
		  return -1;
		}
		if (status == 0) {
		  TTCN_warning("EOF on netlink");
		  return -1;
		}
		if (msg.msg_namelen != sizeof(nladdr)) {
		  return -1;//exit(1); 
		}
		for (h = (struct nlmsghdr*)buf; status >= sizeof(*h); ) {
			int err;
			int len = h->nlmsg_len;
			int l = len - sizeof(*h);

			if (l<0 || len>(signed)status) {
				if (msg.msg_flags & MSG_TRUNC) {
				  return -1;
				}
				TTCN_warning("Malformed rtln message");
				return -1;//exit(1);
			}

			if (nladdr.nl_pid != (unsigned)peer ||
			    h->nlmsg_pid != rtnl->local.nl_pid ||
			    h->nlmsg_seq != seq) {
				if (junk) {
					err = junk(&nladdr, h, jarg);
					if (err < 0){
						return err;
					}
				}
				/* Don't forget to skip that message. */
				status -= NLMSG_ALIGN(len);
				h = (struct nlmsghdr*)((char*)h + NLMSG_ALIGN(len));
				continue;
			}
		       
			if (h->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(h);
				if ((unsigned)l < sizeof(struct nlmsgerr)) {

				} else {
					errno = -err->error;
					if (errno == 0) {
						if (answer)
							memcpy(answer, h, h->nlmsg_len);
						return 0;
					}
					TTCN_warning("RTNETLINK answers error, %s", strerror(errno));
				}
				return -1;
			}
			if (answer) {
				memcpy(answer, h, h->nlmsg_len);
				return 0;
			}

			TTCN_warning("Unexpected rtln replay!");

			status -= NLMSG_ALIGN(len);
			h = (struct nlmsghdr*)((char*)h + NLMSG_ALIGN(len));
		}
		if (msg.msg_flags & MSG_TRUNC) {
			continue;
		}
		if (status) {
		  return -1;//exit(1);
		}
	}
}
