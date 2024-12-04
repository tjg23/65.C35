/**********************************
 * Name: Tyler Gorton             *
 * Changes:                       *
 * - Added variable v (vowels)    *
 *   to track number of vowels    *
 *   - Init to 0 with l,w,c       *
 * - Add new check in for loop    *
 *   for vowels                   *
 *   - Increment v                *
 * - Add v to printf call at      *
 *   end of wc function           *
 **********************************/

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char buf[512];

void
wc(int fd, char *name)
{
  int i, n;
  int l, w, c, v, inword;

  l = w = c = v = 0;
  inword = 0;
  while((n = read(fd, buf, sizeof(buf))) > 0){
    for(i=0; i<n; i++){
      c++;
      if(buf[i] == '\n')
        l++;
      /* Check if character is a vowel */
      if(strchr("aAeEiIoOuU", buf[i]))
	v++;
      if(strchr(" \r\t\n\v", buf[i]))
        inword = 0;
      else if(!inword){
        w++;
        inword = 1;
      }
    }
  }
  if(n < 0){
    printf("wc: read error\n");
    exit(1);
  }
  printf("%d %d %d %d %s\n", l, w, c, v, name);
}

int
main(int argc, char *argv[])
{
  int fd, i;

  if(argc <= 1){
    wc(0, "");
    exit(0);
  }

  for(i = 1; i < argc; i++){
    if((fd = open(argv[i], O_RDONLY)) < 0){
      printf("wc: cannot open %s\n", argv[i]);
      exit(1);
    }
    wc(fd, argv[i]);
    close(fd);
  }
  exit(0);
}
