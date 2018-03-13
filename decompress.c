#include <stdio.h>

const char *uncompress(const char *s);

int main()
{
    char buf[16384];

    while(fgets(buf, 16384, stdin)) {
	puts(uncompress(buf));
    }
    return 0;
}
