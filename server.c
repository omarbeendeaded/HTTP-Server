#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define STDOUT   1
#define BUF_SIZE 100
#define BACKLOG  2
#define PORT     80

extern char** environ;

void sendChunk (char* chunk, int cfd, int newline);
void do_cat    (char* path, int cfd);
void do_ls     (char *dir, int cfd);
void getPath   (char* req, char* path, char* query);
int  checkPath (char* path);

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	int server_fd, cfd;
	ssize_t numRead;
	char buf[BUF_SIZE];
	int opt = 1;

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("SERVER: socket create failed (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

#if 1
	// Forcefully attaching socket to the port
	if (setsockopt(server_fd, SOL_SOCKET,
		       SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
#endif

	/* Construct server socket address, bind socket to it,
	   and make this a listening socket */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		printf("SERVER: bind failed (%s)\n", strerror(errno));
		exit(1);
	}

	if (listen(server_fd, BACKLOG) == -1) {
		printf("SERVER: listen failed (%s)\n", strerror(errno));
		exit(1);
	}

	for (;;) {		/* Handle client connections iteratively */

		/* Accept a connection. The connection is returned on a new
		   socket, 'cfd'; the listening socket ('server_fd') remains open
		   and can be used to accept further connections. */

		cfd = accept(server_fd, NULL, NULL);
		if (cfd == -1) {
			printf("SERVER: accept failed (%s)\n", strerror(errno));
			exit(1);
		}

		// Make server concurrent
		int rt = fork();
		if (rt > 0) continue;


		/* Transfer data from connected socket to stdout until EOF */

		while ((numRead = read(cfd, buf, BUF_SIZE)) > 0) {
			if (write(STDOUT_FILENO, buf, numRead) != numRead) {
				printf("SERVER: partial/failed write (%s)\n", strerror(errno));
				exit(1);
			}
			
			// Check if GET request received
			if (strncmp(buf, "GET", 3) == 0)
			{
				// Get path to file/dir & query if found
				char path[200]; 
				char query[200];
				getPath(buf, path, query);
						
				int pathType = checkPath(path);

				// ls or cat
				if (pathType == 1 || pathType == 2)
				{	
				
					// Send HTTP response
					char header[200] = "HTTP/1.1 200 OK\nContent-Type: text/plain\nTransfer-Encoding: chunked\n\n";
					write(cfd, header, strlen(header));
					printf("\n\n%s", header);
					
					if (pathType == 1) do_ls(path, cfd);
					else               do_cat(path, cfd);
				}

				// Run CGI script
				else if (pathType == 3)
				{
					// Set environment variable for CGI script
					if (setenv("QUERY_STRING", query + 1, 1) < 0)
					{
						perror("setenv");
						exit(1);
					}

					int ret = fork();
					if (ret == 0)
				       	{
						// Redirect output to socket
						dup2(cfd, STDOUT);

						char* args[2];
						args[0] = path;
						args[1] = NULL;
						
						execvp(path, args);
						perror("exec");
						exit(1);
					}
					else
					{
						int status;
						if (wait(&status) == -1)
						{
							perror("fork");
							exit(1);
						}

						// Delete query after script is done
						unsetenv("QUERY_STRING");
					}
				}
				// If path does not exist send 404 error
				else
				{
					char* error = "HTTP/1.1 404 Not Found\nContent-Type: text/html\nContent-Length: 34\n\n<h1>ERROR 404: URL Not Found</h1>\n";
					write(cfd, error, strlen(error));
				}
			
			}
		}

		if (numRead == -1) {
			printf("SERVER: read failed (%s)\n", strerror(errno));
			exit(1);
		}

		if (close(cfd) == -1) {
			printf("SERVER: close failed (%s)\n", strerror(errno));
			exit(1);
		}
	}
}


void getPath(char* req, char* path, char* query)
{
	char fullPath[200];
	getcwd(fullPath, 200);

	strtok(req, " ");
	char* relPath = strtok(NULL, " ");

	const char* qStart = strchr(relPath, '?');

	if (qStart != NULL) strncat(fullPath, relPath, qStart - relPath);
	else                strcat(fullPath, relPath);

	fullPath[strlen(fullPath)] = '\0';

	strcpy(path, fullPath);
	if (qStart != NULL) strcpy(query, qStart);	
}

int checkPath(char* path)
{
	struct stat path_stat;

	if (stat(path, &path_stat) != 0) return 0;

	const char* ext = strrchr(path, '.');
	if (ext && strcmp(ext, ".cgi") == 0) return 3;

	if (S_ISDIR(path_stat.st_mode))  return 1;
	if (S_ISREG(path_stat.st_mode))  return 2;
	
	return 0;
}

void do_ls(char *dir, int cfd)
{
	struct dirent *entry;
	DIR *dp = opendir(dir);
	if (dp == NULL) 
	{
		fprintf(stderr, "Cannot open directory:%s\n", dir);
		return;
	}
	
	errno = 0;
	
	while ((entry = readdir(dp)) != NULL) 
	{
		if (entry == NULL && errno != 0) 
		{
			perror("readdir failed");
			exit(errno);
		} 
		else 
		{
			if (entry->d_name[0] == '.') continue;

			sendChunk(entry->d_name, cfd, 1);
		}
	}
	
	write(cfd, "0\r\n\r\n", 5);
	closedir(dp);
}

void do_cat(char* path, int cfd)
{
	char buf[100];
	int fd = open(path, O_RDONLY);
	
	if (fd < 0)
	{
		printf("SERVER: Failed to open file\n");
		exit(1);
	}
	
	int numRead;
	while((numRead = read(fd, buf, sizeof(buf) - 1)) > 0)
	{
		buf[numRead] = '\0';
		sendChunk(buf, cfd, 0);
	}

	write(cfd, "0\r\n\r\n", 5);
	close(fd);
}

void sendChunk(char* chunk, int cfd, int newline)
{
	char hexStr[20];
	int chunkSize = strlen(chunk);
	sprintf(hexStr, "%X", chunkSize + newline);
	
	write(cfd, hexStr, strlen(hexStr));
	write(cfd, "\r\n", 2);
	
	if (write(cfd, chunk, chunkSize) != chunkSize) 
	{
		printf("SERVER: partial/failed write (%s)\n", strerror(errno));
		exit(1);
	}
	
	if (newline) write(cfd, "\n\r\n", 3);

	printf("%s\n", hexStr);
	printf("%s\n", chunk);
}

