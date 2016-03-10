#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	void *mem = malloc(atoi(argv[1]));

	if (!mem) {
		fprintf(stderr, "failed to allocate, mem = %p\n", mem);
		exit(1);
	}
	printf("mem: %p\n", mem);
	free(mem);

	return 0;
}
