#ifndef COMMON_H
# define COMMON_H

  /* Make sure fopen64 and friends are defined. */
# if !defined(_LARGEFILE64_SOURCE) || !_LARGEFILE64_SOURCE
#  undef _LARGEFILE64_SOURCE 
#  define _LARGEFILE64_SOURCE 1
# endif

# include <stdlib.h>
# include <string.h>
# include <math.h>

#endif /* COMMON_H */
