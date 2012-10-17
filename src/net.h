/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net.h -- quake's interface to the networking layer

#ifndef __NET_H__
#define __NET_H__

#include "common.h"
#include "sys_net.h"

struct SysNetData;

#define	PORT_ANY	-1

enum netsrc
{
	NS_CLIENT,
	NS_SERVER
};

struct qw_connection;

qboolean NET_OpenSocket(struct qw_connection *qwc, enum netsrc socknum, enum netaddrtype type);

void		NET_Init (struct qw_connection *qwc);
void		NET_Shutdown (struct qw_connection *qwc);
void		NET_ServerConfig (struct qw_connection *qwc, qboolean enable);	// open/close server socket

void		NET_ClearLoopback (struct qw_connection *qwc);
qboolean	NET_GetPacket(struct qw_connection *qwc, enum netsrc sock, struct sizebuf_s *message, struct netaddr *from);
void		NET_SendPacket(struct qw_connection *qwc, enum netsrc sock, int length, void *data, const struct netaddr *to);
void		NET_Sleep (int msec);

qboolean	NET_CompareAdr(const struct netaddr *a, const struct netaddr *b);
qboolean	NET_CompareBaseAdr(const struct netaddr *a, const struct netaddr *b);
qboolean	NET_IsLocalAddress(const struct netaddr *a);
char		*NET_AdrToString(const struct netaddr *a);
char		*NET_BaseAdrToString(const struct netaddr *a);
qboolean	NET_StringToAdr(struct qw_connection *qwc, struct SysNetData *sysnetdata, const char *s, struct netaddr *a);
char *NET_GetHostnameForAddress(struct qw_connection *qwc, const struct netaddr *addr);

#endif

