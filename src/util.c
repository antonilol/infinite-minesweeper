#include "util.h"

#include <stdio.h>
#include <stdlib.h>

void handle_alloc_error() {
	printf("Failed to allocate memory\n");
	exit(1);
}
