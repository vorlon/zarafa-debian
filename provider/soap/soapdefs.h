#ifndef SOAPDEFS_H_
#define SOAPDEFS_H_

/*
 * We want to use strtod_l and sprintf_l. By defining WITH_C_LOCALE
 * gSoap will not undef HAVE_STRTOD_L and HAVE_SPRINTF_L (if they
 * were defined in the first place).
 */
#define WITH_C_LOCALE


#include <platform.h>

# include <bits/types.h>
# undef __FD_SETSIZE
# define __FD_SETSIZE 8192

#endif // ndef SOAPDEFS_H_
