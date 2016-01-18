#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
using namespace std;
int main()
{
    int n;
    scanf("%d",&n);
    while(n--) {
        int x;
        scanf("%d",&x);
        int a,b,c,d,e;
        scanf("%d %d %d %d %d",&a,&b,&c,&d,&e);
        if(a+b+c+d+e == x) printf("%d %d %d %d %d\n",a,b,c,d,e);
        else if(b+c+d+e == x) printf("0 %d %d %d %d\n",b,c,d,e);
        else if(a+c+d+e == x) printf("%d 0 %d %d %d\n",a,c,d,e);
        else if(a+b+d+e == x) printf("%d %d 0 %d %d\n",a,b,d,e);
        else if(a+b+c+e == x) printf("%d %d %d 0 %d\n",a,b,c,e);
        else if(a+b+c+d == x) printf("%d %d %d %d 0\n",a,b,c,d);
        else printf("-1\n");
    }
}
