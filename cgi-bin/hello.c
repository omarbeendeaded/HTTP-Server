#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{	
	char* query = getenv("QUERY_STRING");

	// Extract value
	strtok(query, "=");
	char* name = strtok(NULL, "=");

	// Default if nothing is passed
	if (name == NULL) name = "World";

	// Send HTTP Response
	printf("HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: ");
	printf("%li\n\nHello %s!\n", strlen(name) + 7, name);
}
