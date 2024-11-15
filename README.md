# HTTP Server
An HTTP server the supports 3 types of requests:
- Listing directory
- Printing contents of a file
- Running CGI scripts

## Compilation & Startup
``` bash
gcc -o server server.c
sudo ./server
```

## Requesting data
### List directory & print file
Send a GET request containing the path of the desired file/directory, similar to this:
``` HTTP
GET /exampleDir/ex.txt HTTP/1.1
Host: example.com
Connection: keep-alive
```
### Running CGI Scripts
There are 2 example CGI scripts written in C in `cgi-bin/`, 
```
gcc -o cgi-bin/hello.cgi cgi-bin/hello.c
```
to use them compile them on the host machine and run using GET requests like this:

#### hello.cgi
Prints "Hello, `name`!"
``` HTTP
GET /cgi-bin/hello.cgi?name=Omar HTTP/1.1
Host: example.com
Connection: keep-alive
```
Where `name=Omar` after the path passes the value of the variable `name` to the CGI script

#### calc.cgi
Performs the operation `op`: addition, subtraction, multiplication, division on `A` & `B`
``` HTTP
GET /cgi-bin/hello.cgi?A=7&B=24&op=sub HTTP/1.1
Host: example.com
Connection: keep-alive
```
