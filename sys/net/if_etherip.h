/*      $NetBSD: if_etherip.h,v 1.1 2006/11/23 04:07:07 rpaulo Exp $        */

/*
 *  Copyright (c) 2006, Hans Rosenfeld <rosenfeld@grumpf.hope-2000.org>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Hans Rosenfeld nor the names of his
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 */

#ifndef _NET_IF_ETHERIP_H_
#define _NET_IF_ETHERIP_H_

#include <sys/queue.h>

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_inet.h"
#endif

#include <netinet/in.h>

LIST_HEAD(, etherip_softc) etherip_softc_list;

struct etherip_softc {
        struct device   sc_dev;
        struct ifmedia  sc_im;
        struct ethercom sc_ec;
        struct sockaddr *sc_src;                /* tunnel source address      */
        struct sockaddr *sc_dst;                /* tunnel destination address */
        union {
#ifdef INET
                struct route     scr_ro;        /* cached inet route          */
#endif
#ifdef INET6
                struct route_in6 scr_ro6;       /* cached inet6 route         */
#endif
        } sc_scr;
        long sc_route_expire;
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
        void *sc_si;                            /* softintr handle            */
#endif
        LIST_ENTRY(etherip_softc) etherip_list; /* list of etherip tunnels    */
};
#define sc_ro  sc_scr.scr_ro
#define sc_ro6 sc_scr.scr_ro6

struct etherip_header {
        u_int8_t eip_ver;       /* version/reserved */
        u_int8_t eip_pad;       /* required padding byte */
};

#define ETHERIP_VER_VERS_MASK   0x0f
#define ETHERIP_VER_RSVD_MASK   0xf0
#define ETHERIP_VERSION         0x03

#define ETHERIP_TTL 30
#define ETHERIP_ROUTE_TTL 10

#endif /* !_NET_IF_ETHERIP_H_ */
