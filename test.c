#include<stdio.h>
#include<stdlib.h>

int main(){
int *a = malloc(sizeof(int)*0);
printf("%d",a[0]);
return 0;
}
