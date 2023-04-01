#pragma once
#include <stdint.h>
#include <assert.h>

#include "socket.h"

extern int h_errno;

struct hostent
{
   char  *h_name;
   char **h_aliases;
   int    h_addrtype;
   int    h_length;
   char **h_addr_list;
#define	h_addr h_addr_list[0]
};

struct hostent* gethostbyname(const char *name);
