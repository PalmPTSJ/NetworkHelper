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
    for(int i = 0;i < n;i++) {
        int a,b;
        scanf("%d %d",&a,&b);
        printf("%d\n",a+b);
    }
}
