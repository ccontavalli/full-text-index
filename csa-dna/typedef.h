//#define BIT64

#ifdef BIT64
//typedef long long i64;  /*  for gcc */
typedef long int i64; /* for -m64 */
typedef unsigned long int u64; /* for -m64 */
#else
typedef long i64; /* for 32-bit cpu */
typedef unsigned long u64; /* for 32-bit cpu */
#endif
typedef int i32;
typedef unsigned int u32;
typedef unsigned short u16;

