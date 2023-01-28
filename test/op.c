#include <stdio.h>

int main()
{
    int i;
    i = 3;

    printf(" + : %d\n", i + i);
    printf(" - : %d\n", i - i);
    printf(" * : %d\n", i * i);
    printf(" / : %d\n", i / i);
    printf(" % : %d\n", i % i);
    printf(" & : %d\n", i & i);
    printf(" | : %d\n", i | i);
    printf(" ^ : %d\n", i ^ i);
    printf(" << : %d\n", i << i);
    printf(" >> : %d\n", i >> i);

    while( i != 10 )
    {
        printf(" %d : %d\n", i, i);
        i++;
    }

    return 0;
    
}