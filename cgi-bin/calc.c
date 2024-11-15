#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main()
{
	char* query = getenv("QUERY_STRING");

	// Default values
	char* strA = "0";
	char* strB = "0";
	char* op = "add";

	// Extract data
	char* tok = strtok(query, "=");
	while (tok != NULL)
	{
		if (strcmp(tok, "A") == 0)  strA = strtok(NULL, "&");
		if (strcmp(tok, "B") == 0)  strB = strtok(NULL, "&");
		if (strcmp(tok, "op") == 0) op = strtok(NULL, "&");

		tok = strtok(NULL, "=");
	}

	int A = atoi(strA);
	int B = atoi(strB);
	int res = 0;


	// Operation
	if      (strcmp(op, "add") == 0) res = A + B;
	else if (strcmp(op, "sub") == 0) res = A - B;
	else if (strcmp(op, "mul") == 0) res = A * B;
	else if (strcmp(op, "div") == 0) res = A / B;

	char strRes[20];
	sprintf(strRes, "%d", res);

	printf("HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: ");	

        char body[200];
	sprintf(body, "Operand A: %s\nOperand B: %s\nOperation: %s\nResult: %s\n", strA, strB, op, strRes);
	printf("%li\n\n%s", strlen(body), body);
}




