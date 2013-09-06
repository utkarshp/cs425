#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string>
using namespace std;

void error(string err)
{
	perror(err.c_str());
	exit(0);
}

main(int argc, char *argv[])
{
        char *recv_data, send_data[2000];
	int servport,sockfd,n;
	char address[1024];
	bzero(address,sizeof(address));
        bzero(send_data,sizeof(send_data));

	if (argc==1)
	{
		printf("Enter server ip-address:-\n");
		scanf("%s",address);
		printf("Enter port of server:\n");
		scanf("%d",&servport);
	}
	else if (argc==2)
		error("usage: file_name ip_address port_no.\n");

	else
	{
		servport=atoi(argv[2]);
		strcpy(address,argv[1]);
	}

	struct sockaddr_in servaddr;
	struct hostent *server;
	int recvd_bytes,fdmax;

        fd_set master, read_fds;
        FD_ZERO(&master);
        FD_ZERO(&read_fds);

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if (sockfd<0) error("Failure on creating socket");

	server=gethostbyname(address);
	if (server==NULL) error("Error connecting to Server");

	bzero((char *)&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	bcopy((char *)server->h_addr,(char *)&servaddr.sin_addr.s_addr,server->h_length);
	servaddr.sin_port=htons(servport);
	char encrypted[1024];

	if (connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0)
		error("Error Connecting");

 	FD_SET(sockfd,&master);
        FD_SET(STDIN_FILENO,&master);
        fdmax=sockfd;

        int selfid=0;

	while(1)
	{
                read_fds=master;
		if (select(fdmax+1,&read_fds,NULL,NULL,NULL)==-1)
		 	error("Selection error");
		
		if (FD_ISSET(sockfd,&read_fds))
		{
			bzero(encrypted,sizeof(encrypted));
			recvd_bytes=recv(sockfd,encrypted,sizeof(encrypted),0);
			printf("%s",encrypted);
		}
                else if (FD_ISSET(STDIN_FILENO,&read_fds))
		{
			bzero(encrypted,sizeof(encrypted));
			bzero(send_data,sizeof(send_data));
			printf("utk->");
			fgets(encrypted,1023,stdin);
			send(sockfd,encrypted,strlen(encrypted),0);

		}
	}
}
