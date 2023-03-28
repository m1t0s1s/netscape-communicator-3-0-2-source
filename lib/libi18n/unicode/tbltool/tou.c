#include <stdio.h>
extern void SetMapValue(short u,short c);
void getinput()
{
  char buf[256];
  short c,u,idx;
  for(;gets(buf)!=NULL;)
  {
     if(buf[0]=='0' && buf[1] == 'x')
        {
          sscanf(buf,"%hx %hx",&c,&u);
          SetMapValue(c, u);
        }
  }
}
