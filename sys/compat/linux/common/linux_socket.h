/*	$NetBSD: linux_socket.h,v 1.13 2005/12/11 12:20:19 christos Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_SOCKET_H
#define _LINUX_SOCKET_H

/*
 * Various Linux socket defines. Everything that is not re-defined here
 * is the same as in NetBSD.
 *
 * COMPAT_43 is assumed, and the osockaddr struct is used (it is what
 * Linux uses)
 */

/*
 * Address families. There are fewer of them, and they're numbered
 * a bit different
 */

#define LINUX_AF_UNSPEC		0
#define LINUX_AF_UNIX		1
#define LINUX_AF_INET		2
#define LINUX_AF_AX25		3
#define LINUX_AF_IPX		4
#define LINUX_AF_APPLETALK	5
#define LINUX_AF_NETROM		6
#define LINUX_AF_BRIDGE		7
#define LINUX_AF_ATMPVC		8
#define LINUX_AF_X25		9
#define LINUX_AF_INET6		10
#define LINUX_AF_ROSE		11
#define LINUX_AF_DECnet		12
#define LINUX_AF_NETBEUI	13
#define LINUX_AF_SECURITY	14
#define LINUX_pseudo_AF_KEY	15
#define LINUX_AF_NETLINK	16
#define LINUX_AF_PACKET		17
#define LINUX_AF_ASH		18
#define LINUX_AF_ECONET		19
#define	LINUX_AF_ATMSVC		20
#define	LINUX_AF_SNA		22
#define LINUX_AF_MAX		32

/*
 * Option levels for [gs]etsockopt(2). Only SOL_SOCKET is different,
 * the rest matches IPPROTO_XXX
 */

/* SOL_SOCKET is machine dependant on Linux */
#define LINUX_SOL_IP		0
#define LINUX_SOL_TCP		6
#define LINUX_SOL_UDP		17
/* Unused for now: */
#define LINUX_SOL_IPV6		41
#define LINUX_SOL_ICMPV6	58
#define LINUX_SOL_RAW		255
#define LINUX_SOL_IPX		256
#define LINUX_SOL_AX25		257
#define LINUX_SOL_ATALK		258
#define LINUX_SOL_NETROM	259
#define LINUX_SOL_ROSE		260
#define LINUX_SOL_DECNET	261
#define LINUX_SOL_X25		262
#define LINUX_SOL_PACKET	263
#define LINUX_SOL_ATM		264
#define LINUX_SOL_AAL		265

/*
 * Options for [gs]etsockopt(2), socket level are machine dependant.
 */

/*
 * Options for [gs]etsockopt(2), IP level.
 */

#define LINUX_IP_TOS		1
#define LINUX_IP_TTL		2
#define	LINUX_IP_MULTICAST_IF	32
#define	LINUX_IP_MULTICAST_TTL	33
#define	LINUX_IP_MULTICAST_LOOP	34
#define	LINUX_IP_ADD_MEMBERSHIP	35
#define	LINUX_IP_DROP_MEMBERSHIP 36

/*
 * Options for [gs]etsockopt(2), TCP level.
 */

#define	LINUX_TCP_NODELAY	1
#define	LINUX_TCP_MAXSEG	2

/* "Socket"-level control message types: */
#define LINUX_SCM_RIGHTS	1	/* same as SCM_RIGHTS */
#define LINUX_SCM_CREDENTIALS	2	/* accepts ucred rather than sockcred */
#define LINUX_SCM_CONNECT	3	/* not supported in NetBSD */
#define LINUX_SCM_TIMESTAMP	LINUX_SO_TIMESTAMP
				/* not actually implemented in Linux 2.5.15? */

/*
 * Message flags (for sendmsg/recvmsg)
 */
#define LINUX_MSG_OOB		0x001
#define LINUX_MSG_PEEK		0x002
#define LINUX_MSG_DONTROUTE	0x004
#define LINUX_MSG_TRYHARD	0x004
#define LINUX_MSG_CTRUNC	0x008
#define LINUX_MSG_PROBE		0x010	/* Don't send, only probe path */
#define LINUX_MSG_TRUNC		0x020
#define LINUX_MSG_DONTWAIT	0x040	/* this msg should be nonblocking */
#define LINUX_MSG_EOR		0x080	/* data completes record */
#define LINUX_MSG_WAITALL	0x100	/* wait for full request or error */
#define LINUX_MSG_FIN		0x200
#define LINUX_MSG_EOF		LINUX_MSG_FIN
#define LINUX_MSG_SYN		0x400
#define LINUX_MSG_CONFIRM	0x800	/* Confirm path validity */
#define LINUX_MSG_RST		0x1000
#define LINUX_MSG_ERRQUEUE	0x2000	/* fetch message from error queue */
#define LINUX_MSG_NOSIGNAL	0x4000	/* do not generate SIGPIPE */
#define LINUX_MSG_MORE		0x8000	/* Sender will send more */

/*
 * Linux alignment requirement for CMSG struct manipulation.
 * Linux aligns on (long) boundary on all architectures.
 */
#define LINUX_CMSG_ALIGN(n)	\
	(((n) + sizeof(long)-1) & ~(sizeof(long)-1))
#define LINUX_CMSG_DATA(cmsg)	\
	((u_char *)(void *)(cmsg) + __CMSG_ALIGN(sizeof(struct cmsghdr)))
#define	LINUX_CMSG_NXTHDR(mhdr, cmsg)	\
	(((__caddr_t)(cmsg) + LINUX_CMSG_ALIGN((cmsg)->cmsg_len) + \
			    LINUX_CMSG_ALIGN(sizeof(struct cmsghdr)) > \
	    (((__caddr_t)(mhdr)->msg_control) + (mhdr)->msg_controllen)) ? \
	    (struct cmsghdr *)NULL : \
	    (struct cmsghdr *)((__caddr_t)(cmsg) + \
	        LINUX_CMSG_ALIGN((cmsg)->cmsg_len)))
#define LINUX_CMSG_ALIGNDIFF	\
	(CMSG_ALIGN(sizeof(struct cmsghdr)) - LINUX_CMSG_ALIGN(sizeof(struct cmsghdr)))

/*
 * Machine specific definitions.
 */
#if defined(__i386__)
#include <compat/linux/arch/i386/linux_socket.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_socket.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_socket.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_socket.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_socket.h>
#elif defined(__arm__)
#include <compat/linux/arch/arm/linux_socket.h>
#elif defined(__amd64__)
#include <compat/linux/arch/amd64/linux_socket.h>
#else
#error Undefined linux_socket.h machine type.
#endif

#endif /* !_LINUX_SOCKET_H */
