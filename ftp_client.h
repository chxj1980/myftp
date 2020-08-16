#ifndef __FTP_CLIENT_H__
#define __FTP_CLIENT_H__

#include<iostream>


#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

#define YEAR        (3600*24*30*12)
#define MOU         (3600*24*30)
#define DAY         (3600*24)
#define SIZE_M      (1024*1024)

//#define FILE_PTH    "/sdcard"
#define FILE_PTH    "./"


class ftp_client
{
	public:
		ftp_client();
		~ftp_client();
		
		int start_ftp(const char *hostname, const char *user,  const char *passd);
		
	private:
		
		struct in_addr addr_m;

		int create_sock(const char *hostname);
		int create_data_sock(int port);
		bool get_serverip(const char *hostname, struct in_addr *addr);
		char *find_char(const char *data, int len, const char  find_char, int times);
		int send_and_recv(int sock,    	   char *sendbuff, int sendsize, const char *recvbuff, int recvsize);
		bool server_isready(int sockfd);
		int login_server(int sockfd, const char *user, const char *passd);
		int enter_pasv_mode(int sockfd);
		int cd_dir_list_files(int cmdsockfd, int datasockfd, char *downfile);
		int find_newest_upgfile(int sock, char *recvdate, int szie, char *upgfile);
		int parse_file(char *date, int size,  char *filename);
		int down_upgfile(int cmdsockfd,  int datasockfd, char *downfile);
		int quit_ftp(int cmdsock, int datasock);
};

#endif
