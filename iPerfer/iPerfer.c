#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

void error(char *msg)
{
	perror(msg);
    exit(1);
}


int server(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, client, n;

	struct sockaddr_in serv_addr, cli_addr;
	char buffer[2000];
    char fin[11] = "\0";

    time_t t1, t2=0;
	double duration = 0;
	long int recv_byte = 0;
	long int recv_ = 0;
	float rate = 0.0;

	if(argc != 4)
		error("Error: missing or extra arguments");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		error("ERROR opening socket");
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");

	bzero((char *)&serv_addr, sizeof(serv_addr));
	portno = atoi(argv[3]);
	if (portno < 1024 || portno > 65535)
		error("Error: port number must be in the range of [1024, 65535]");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);

    client = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &client);
    if (newsockfd < 0)
    	error("ERROR on accept");

    t1 = time(NULL);
    while (1)
    {
    	bzero(buffer, 2000);
    	n = recv(newsockfd, buffer, 1000, MSG_WAITALL);
    	if (n < 0) error("ERROR reading from socket");
        recv_byte += n;
        
        strncpy(fin, buffer, 10);
        fin[10] = '\0';
     	if (!strcmp(fin, "disconnect"))
     	{
            //printf("now we sent acknowledge\n");
     		n = send(newsockfd, "acknowledge", 11, 0); 
     		if (n < 0) error("ERROR writing to socket");
            close(newsockfd);
     		break;
     	}
    }

    t2 = time(NULL);
    duration = difftime(t2, t1);

    close(sockfd);

    // printf("%ld\n", t1);
    // printf("%ld\n", t2);
    recv_ = recv_byte/1000;
    rate = recv_byte*8.0/(1000000*duration);
    //printf("received = %ld KB, Rate = %.3f Mbps, duration = %.3f s\n", recv_, rate, duration);
    printf("Received=%ld KB, Rate=%.3f Mbps\n", recv_, rate);

    return 0;
}


int client(int argc, char *argv[])
{
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    time_t t1, t2=0;
    double duration, timer = 0;

    char buffer[2000];
    char msg[1000];
    char fin[1000] = "disconnect";

    long int sent_byte = 0;
    long int sent = 0;
    float rate = 0.0;

    long int count = 0;

    int i;
    for (i = 0;i < 1000;i ++)
    	msg[i] = '0';

    for (i=10;i<1000;i++)
        fin[i] = '0';

    if (argc != 8) {
       fprintf(stderr,"usage %s -c -h <hostname> -p <port> -t <time>\n", argv[0]);
       error("Error: missing or extra arguments");
    }

    portno = atoi(argv[5]);
    if (portno < 1024 || portno > 65535)
    	error("Error: port number must be in the range of [1024, 65535]");

    timer = atof(argv[7]);
    if (timer <= 0)
    	error("Error: time argument must be greater than 0");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[3]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    t1 = time(NULL);
    //printf("\n%ld\n", t1);
    while (1)
    {
    	bzero(buffer, 2000);
    	bcopy((char *)msg, (char *)buffer, 1000);
    	n = send(sockfd, buffer, 1000, 0);
    	if (n < 0)
    		error("ERROR writing to socket");
    	sent_byte += n;

        t2 = time(NULL);
        if (timer < difftime(t2, t1))
    	{
    		break;
    	}
    }

    while (1)
    {
    	bzero(buffer,1000);
        n = send(sockfd, fin, 1000, 0);
        if (n < 0) error("ERROR writing to socket");
        sent_byte += n;

    	n = recv(sockfd,buffer,1000, 0);
    	if (n < 0) error("ERROR reading from socket");

    	if (!strcmp(buffer, "acknowledge"))
    	{
    		close(sockfd);
            t2 = time(NULL);
    		break;
    	}
    }

    duration = difftime(t2, t1);
    sent = sent_byte/1000;
    rate = sent_byte*8/(1000000*duration);
    //printf("sent = %ld KB, Rate = %.3f Mbps, duration = %.3f s\n", sent, rate, duration);
    printf("Sent=%ld KB, Rate=%.3f Mbps\n", sent, rate);

    return 0;
}



int main(int argc, char *argv[])
{
	if(!strcmp(argv[1], "-s"))
		server(argc, argv);
	else if(!strcmp(argv[1], "-c"))
        client(argc, argv);
    else
        error("Error:invalid service mode!");

    return 0;
}
