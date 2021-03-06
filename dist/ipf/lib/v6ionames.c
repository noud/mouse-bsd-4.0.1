/*	$NetBSD: v6ionames.c,v 1.5.4.2 2007/07/16 11:05:24 liamjfoy Exp $	*/

/*
 * Copyright (C) 2003-2005 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: v6ionames.c,v 1.1.4.3 2006/06/16 17:21:18 darrenr Exp
 */
#include "ipf.h"


#ifdef	USE_INET6

struct	ipopt_names	v6ionames[] ={
	{ IPPROTO_HOPOPTS,	0x000001,	0,	"hopopts" },
	{ IPPROTO_IPV6,		0x000002,	0,	"ipv6" },
	{ IPPROTO_ROUTING,	0x000004,	0,	"routing" },
	{ IPPROTO_FRAGMENT,	0x000008,	0,	"frag" },	
	{ IPPROTO_ESP,		0x000010,	0,	"esp" },
	{ IPPROTO_AH,		0x000020,	0,	"ah" },
	{ IPPROTO_NONE,		0x000040,	0,	"none" },	
	{ IPPROTO_DSTOPTS,	0x000080,	0,	"dstopts" },
	{ IPPROTO_MOBILITY,	0x000100,	0,	"mobility" },
	{ 0, 			0,		0,	(char *)NULL }
};

#endif
