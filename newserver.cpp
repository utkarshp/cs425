#include <iostream>
#include <map>
#include <algorithm>
#include <string>
#include <cstring>
#include <vector>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>

#define LIMIT 100
using namespace std;
void error (string err)
{
	perror(err.c_str());
	exit(0);
}

int main(int argc, char *argv[])
{
	map<string, string> userdata[LIMIT];
	string demand[LIMIT];
	int portno;
	if (argc == 2)
		portno = atoi(argv[1]);
	else if (argc == 1)
	{
		cout << "Enter Port Number ";
		cin >> portno;
	}
	else
	{
		cout << "Usage: <Program Name> <Port Number>\n";
		return 0;
	}	
	fd_set master, read_fds, write_fds;
	int serverfd, newfd, fdmax;
	short activity[LIMIT];
	bzero(activity, sizeof(short) * LIMIT);

	if ((serverfd = socket(AF_INET,SOCK_STREAM,0)) == -1) error("Socket call failed\n");
	
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);	
	
	sockaddr_in serveraddr,clientaddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(portno);
	bzero(serveraddr.sin_zero,8);

	if (bind(serverfd, (sockaddr *) &serveraddr, sizeof(serveraddr)) == -1)
		error ("Error in Binding\n");				//Notice the typecast. It is required so that compiler doesn't give warning

	cout << "Server Established successfully\n";
	cout << "Running on port " << portno << '\n';

	if (listen(serverfd,100) == -1) error("Error while extablishing connection\n");

	FD_SET(serverfd, &master);	
	fdmax = serverfd;
	string msg;
	char buf[1024];
	while(1)
	{
		read_fds = write_fds = master;
		if (select(fdmax+1,&read_fds, &write_fds, NULL, NULL) == -1)
			error("Error in Selection\n");
		for( int i=0; i<=fdmax; i++)
		{
			if (FD_ISSET(i,&read_fds))
			{
				if (i==serverfd)
				{
					socklen_t addrlen = sizeof(clientaddr);
					newfd = accept(serverfd, (sockaddr *) &clientaddr, &addrlen);
					if (newfd == -1) error("Error while Trying to accept\n");
					FD_SET(newfd, &master);
					if (newfd > fdmax) fdmax = newfd;
					activity[newfd] = 1;
					cout << "Successfully accepted connection from ";
					char guestname[20];
					getnameinfo((sockaddr *) &clientaddr, addrlen, guestname, sizeof(char)*20, NULL, 0, NI_NUMERICHOST);
					cout << string(guestname) << "\n";
					msg = "You have successfully connected to server";
					send(newfd,msg.c_str(),msg.size(),0);
						/*
						Send Syntax of commands to the client
						*/					
					msg = "\nUsage:\n1)Sharing files:-\n\t\t<SHARE>\n\t\t\t<file1> <path/of/file1>\n\t\t\t<file2> <path/of/file2>\n\t\t\t.\n\t\t\t.\n\t\t\t.\n\t\t<END>\ne.g. <SHARE>\n<test.cpp> </home/username/test.cpp>\n<END>\n\n2)Searching files:-\n\t\t<SEARCH>\n\t\t\tfilename1\n\t\t\tfilename2\n\t\t\t.\n\t\t\t.\n\t\t\t.\n\t\t<END>\ne.g. <SEARCH>\ntest.cpp\n<END>\n\nPlease note that the file you searched will automatically be downloaded to your working directory if it is found.\n\n";
					if (send(newfd,msg.c_str(),msg.size(),0) == -1) error("Error while sendeing message\n");

				}
				else
				{
					bzero(buf,sizeof(buf));
					if (recv(i,buf,sizeof(buf),0) <= 0)			//This means the client is no more.
					{
						cout << "Client on socket " << i << " Has disconnected\n";
						FD_CLR(i,&master);
						activity[i] = 0;
						close(i);
						/*
							Delete the file table from this client
							*/
						userdata[i].clear();
					}
					else
					{
						if (activity[i] == 1)
						{		
							msg = string(buf);		//Expecting a share or search query.
							cout << msg;
							if (msg == "<SHARE>\n")
								activity[i] = 2;
							else if (msg == "<SEARCH>\n")
								activity[i] = 3;
							else
							{
								msg = "\nInvalid Command\nUsage:\n1)Sharing files:-\n\t\t<SHARE>\n\t\t\t<file1> <path/of/file1>\n\t\t\t<file2> <path/of/file2>\n\t\t\t.\n\t\t\t.\n\t\t\t.\n\t\t<END>\ne.g. <SHARE>\n<test.cpp> </home/username/test.cpp>\n<END>\n\n2)Searching files:-\n\t\t<SEARCH>\n\t\t\tfilename1\n\t\t\tfilename2\n\t\t\t.\n\t\t\t.\n\t\t\t.\n\t\t<END>\ne.g. <SEARCH>\ntest.cpp\n<END>\n\nPlease note that the file you searched will automatically be downloaded to your working directory if it is found.\n\n";
								send(i,msg.c_str(),msg.size(),0);
							}
						}
						else if (activity[i] == 2)		//Sharing going on.
						{
							msg = string(buf);
							int fname1,fname2,fpath1,fpath2;
							fname1 = 0;
							fname2 = msg.find_first_of('>');
							fpath1 = fname2 + 2;
							fpath2 = msg.size()-2;
							try
							{
								string filename = msg.substr(fname1+1,fname2-1);
								if (filename == "END")
								{
									activity[i] = 1;
									continue;
								}
								string path = msg.substr(fpath1+1,fpath2-fpath1-1);
								userdata[i][filename] = path;
							}
							catch(...)
							{
								msg = "Invalid Syntax.\nPlease Try again\n";
								send(i,msg.c_str(),msg.size(),0);
								continue;
							}
						}
						else if (activity[i] == 3)					//Searching going on
						{
							msg = string(buf);
							int fname1, fname2;
							fname1 = 0;
							fname2 = msg.size() - 2;			//Also includes \n. Have to subtract that.
							int flag = 0;
							string filename = msg.substr(fname1 + 1, fname2 - 1);
							if (filename == "END")
							{
								activity[i] = 1;
								continue;
							}
							for (int j = 0; j <= fdmax; j++)
								if (!activity[j]) continue;
								else if (userdata[j].count(filename))
								{
									if (flag==0)
									{
										msg = "List of client id with matching filenames are:\nID\t\tPath\n\n";
										flag = 1;
									}
									msg += to_string(j);
									msg += "\t\t" + userdata[j][filename] + '\n';
								}
							if (flag == 0)
								msg = "Filename not found on any client connected\n";
							else
							{
								msg += "Choose any id from these\n";
								activity[i] = 4;
								demand[i] = filename;
							}
							send(i,msg.c_str(),msg.size(),0);
							
						}
						else if (activity[i] == 4)
						{
							msg = string(buf);
							int choice;
							try
							{
								choice = stoi(msg);
							}
							catch(...)
							{
								msg = "Not a number. Please Try again\n";
								send(i,msg.c_str(),msg.size(),0);
								continue;
							}
							if (choice < 0 || choice >= LIMIT)
							{
								msg = "You are back to Search menu\n";
								send(i,msg.c_str(),msg.size(),0);
								activity[i] = 3;
								continue;
							}
							if (userdata[choice].count(demand[i]) == 0)
							{
								msg = "Incorrect choice. Please Try again\n";
								msg += "To go to previous menu, enter -1\n";
								send(i,msg.c_str(),msg.size(),0);
								continue;
							}
							cout << "Successful\n";
							/*HERE I stopped on saturday night*/
						}
						
					}
				}
			}
			else if(FD_ISSET(i,&write_fds))
			{
				continue;
			}
		}
	}
}