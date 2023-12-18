#include <stdio.h>
#include <stdlib.h>


int main()
{
   char *p = NULL;
   char *n = NULL;
   char *m = NULL;


   printf("Test 5 is testing some basic allocation\n");
   
   p = malloc(10000);
   n = malloc(30);

   free(p);

   p = malloc(500);
   m = malloc(300);

   free(p);
   free(n);
   free(m);

   return 0;
}
