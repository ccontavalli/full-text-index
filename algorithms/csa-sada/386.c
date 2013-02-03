#include	<sys/types.h>
#include	<sys/times.h>
#include	<sys/param.h>

/*int hz = HZ;
double tcmp = 4.91;*/

static struct tms start3;

void startclock3() {
  times(&start3);
}

long stopclock3() {
  struct tms t;
  times(&t);
  return(t.tms_utime - start3.tms_utime);
}

