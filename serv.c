/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2019, University Politehnica of Bucharest.
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

struct servent *getservbyname(const char *name __unused,
	const char *proto __unused)
{
	return NULL;
}

struct servent *getservbyport(int port __unused,
	const char *proto __unused)
{
	return NULL;
}

int getservbyport_r(int port, const char *prots, struct servent *se, char *buf, size_t buflen, struct servent **res)
{
  int i;
  struct sockaddr_in sin = {
    .sin_family = AF_INET,
    .sin_port = port,
  };

  if (!prots) {
    int r = getservbyport_r(port, "tcp", se, buf, buflen, res);
    if (r) r = getservbyport_r(port, "udp", se, buf, buflen, res);
    return r;
  }
  *res = 0;

  /* Align buffer */
  i = (uintptr_t)buf & (sizeof(char *)-1);
  if (!i) i = sizeof(char *);
  if (buflen < 3*sizeof(char *)-i)
    return ERANGE;
  buf += sizeof(char *)-i;
  buflen -= sizeof(char *)-i;

  if (strcmp(prots, "tcp") && strcmp(prots, "udp")) return EINVAL;

  se->s_port = port;
  se->s_proto = (char *)prots;
  se->s_aliases = (void *)buf;
  buf += 2*sizeof(char *);
  buflen -= 2*sizeof(char *);
  se->s_aliases[1] = 0;
  se->s_aliases[0] = se->s_name = buf;

  switch (getnameinfo((void *)&sin, sizeof sin, 0, 0, buf, buflen,
		      strcmp(prots, "udp") ? 0 : NI_DGRAM)) {
  case EAI_MEMORY:
  case EAI_SYSTEM:
    return ENOMEM;
  default:
    return ENOENT;
  case 0:
    break;
  }

  /* A numeric port string is not a service record. */
  if (strtol(buf, 0, 10)==ntohs(port)) return ENOENT;

  *res = se;
  return 0;
}
