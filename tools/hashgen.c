#include <stdio.h>
#include <string.h>

#define Uint32 unsigned long
#define Uint16 unsigned short

// hash function, which operates on a single line: hashes everything to the first occurence
// of a '=' character (if it does not exist, the whole line is hashed)
static Uint16 hashid(char *s)
{
	register Uint32 acc;
	int i, l;

	l = 0;
	while (s[l]!=0 && s[l]!='=' && s[l]!='[' && s[l]!='.') l++;
	s[l]=0;
	for (acc=0, i=0;i<(l&~1);i++)
		acc += (Uint32) s[i] + (((Uint32) s[i+1])<<8);
	if (l%2)
		acc ^= s[l-1];
	acc = (acc&0xffff)^(acc>>16);
	return acc;
}

int main(void)
{
	char s[100];

	printf("Enter lines to evaluate. To stop, input `exit'\n");
	do {
		scanf("%s", s);
		printf("[%s] -> %d <-> 0x%x\n", s, hashid(s), hashid(s));
		} while (strcmp(s, "exit"));
	return 0;
}
