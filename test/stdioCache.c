#include <stdio.h>
#include <stdlib.h>

int main()
{
    char temp[20];
    int ret;
    FILE *fp = fopen("./test.txt", "r+");
    ret = fread(temp, 20, 1, fp);
    printf("we read: %s\n",temp);
    ret = fwrite("abcdefg\n", 8, 1, fp);

    return 0;
}
