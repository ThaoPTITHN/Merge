#include<stdio.h>
int main()
{
    int x = 50;
    int *p = &x;
    printf("\nDia chi cua bien x la : %x", &x);
    printf("\nGia tri chua trong bien con tro p: %x", p);
    printf("\nGia tri cua bien con tro p: %d",*p);
}