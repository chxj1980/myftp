#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "ftp_client.h"


#define FTP_FLAG  "ftp://"


ftp_client::ftp_client()
{

}

ftp_client::~ftp_client()
{

}

int ftp_client::start_ftp(const char *hostname, const char *user,  const char *passd)
{
	if( (hostname == NULL) || (user == NULL) || (passd == NULL) ) {
		printf("[start_ftp]: Input [hostname] or [user] or [passd] is NULL, exit\n");
		return -1;
	}
	
	int ret = 0;
	int cmdsockfd = 0;
	int datasockfd = 0;
	int server_port = 0;
	char downfile[64] = {'\0'};
	
	cmdsockfd = create_sock(hostname);
	if (server_isready(cmdsockfd) != true)  return -1;

	ret = login_server(cmdsockfd, user, passd);
	if (ret < 0) return -1;

	server_port = enter_pasv_mode(cmdsockfd);
	if (server_port < 0) return -1;

	datasockfd = create_data_sock(server_port);
	if (datasockfd <= 0) return -1;

	ret = cd_dir_list_files(cmdsockfd, datasockfd, downfile);
	if (ret < 0) return -1;

	ret = down_upgfile(cmdsockfd, datasockfd, downfile);
	if (ret < 0) return -1;

	ret = quit_ftp(cmdsockfd, datasockfd);
	if (ret < 0) return -1;

	return 0;
}

bool ftp_client::server_isready(int sockfd)
{	
	int size = 0;
	char recvbuff[256] = {'\0'};

	size= recv(sockfd, recvbuff, sizeof(recvbuff), 0);
	if(size < 0){
		printf("Recv failed, error:%m\n");
		return false;
	}
	
	if((recvbuff) && (strstr((char *)recvbuff, "220")) ){
		printf("[server_isready]: ftp server have been ready !\n");
		return true;
	}
	else{
		printf("[server_isready]: ftp server doesn't be ready, exit !\n");
		false;
	}
}

int ftp_client::login_server(int sockfd, const char *user, const char *passd)
{
	if( (!user) || (!passd)){
		printf("[login_server]: Input [user] or [passd] is NULL, exit !\n");
		return -1;
	}
	
	char cmdbuff[256] = {'\0'};
	char recvbuff[256] = {'\0'};
	int len = 0;
	len = sprintf(cmdbuff, "USER %s\r\n", user);
	if (send_and_recv(sockfd, cmdbuff, len, recvbuff, sizeof(recvbuff)) < 0){
		printf("[login_server]:  [USER] option failed, exit !\n");
		return -1;
	}

	if( !strstr(recvbuff, "331")){
		printf("[login_server]: recv server [user] reply failed, exit !\n");
		return -1;	
	}

	memset(cmdbuff, '\0', sizeof(cmdbuff));
	memset(recvbuff, '\0', sizeof(recvbuff));
	len = sprintf(cmdbuff, "PASS %s\r\n", passd);
	if (send_and_recv(sockfd, cmdbuff, len, recvbuff, sizeof(recvbuff)) < 0){
		printf("[login_server]:  [USER] option failed, exit !\n");
		return -1;
	}

	if(strstr(recvbuff, "230")){
		printf("[login_server]: login ftp server success !\n");
		return 0;
	}else{
		printf("[login_server]: login ftp server failed,[passd] error, exit !\n");
		return -1;
	}
}

//227 Entering Passive Mode (121,42,70,22,156,65).
int ftp_client::enter_pasv_mode(int sockfd)
{
	char cmdbuff[256] = {'\0'};
	char recvbuff[256] = {'\0'};
	int szie = 0;
	int len = 0;
	len = sprintf(cmdbuff, "PASV\r\n"); 	
	if ((szie = send_and_recv(sockfd, cmdbuff, len, recvbuff, sizeof(recvbuff))) < 0){
		printf("[enter_pasv_mode]:  [PASV] option failed, exit !\n");
		return -1;
	}

	printf("RECV:%s\n", recvbuff);
	char *tmp = NULL;
	if( !strstr(recvbuff, "227")){
		printf("[enter_pasv_mode]: recv server [PASV] reply failed, exit !\n");
		return -1;	
	}

	printf("[%s]", recvbuff );
	int port1 = 0;
	int port2 = 0;
	int port = 0;
	if(tmp = find_char(recvbuff, szie, ',', 4))
	{
		tmp++;
		port1 = strtoul(tmp, &tmp, 10);
		port2 = strtoul(tmp+1, NULL, 10);
		port = port1*256 + port2;
		
		printf("server port is:[%d]*256+[%d]=[%d]\n", port1, port2, port);
		return port;
	}

	printf("[enter_pasv_mode]: get server port failed, exit !\n");
	return -1;
}

char * ftp_client::find_char(const char *data, int len, const char  fchar, int times)
{
	if(data == NULL  ||  (len <= 0)){
		printf("Input data is NULL, or len is <= 0, exit !\n");
		return NULL;
	}

	char *p = NULL;
	int j = 1;
	int i = 0;
	while(i < len)
	{
		if(p = (char *)memchr(data + i, fchar, 1)){
			if(j == times)  return p;
			j++;
		}	
		i++;
	}
	
	return NULL;
}

int  ftp_client::cd_dir_list_files(int cmdsockfd, int datasockfd, char *downfile)
{
	if(!downfile){
		printf("[cd_dir_list_files]:Input [downfile] is NULL, exit !\n");
		return -1;
	}
	
	char cmdbuff[256] = {'\0'};
	char recvbuff[1024] = {'\0'};
	int szie = 0;
	int len = 0;

	len = sprintf(cmdbuff, "CWD /download\r\n"); 	
	if (send_and_recv(cmdsockfd, cmdbuff, len, recvbuff, sizeof(recvbuff)) < 0){
		printf("[cd_dir_list_files]:  [CWD /download] option failed, exit !\n");
		return -1;
	}
	
	if( !strstr(recvbuff, "250") ){
		printf("[cd_dir_list_files]: recv cmd server [CWD /download] reply failed, exit !\n");
		return -1;	
	}

	memset(recvbuff, 0, sizeof(recvbuff));
	memset(cmdbuff, 0, sizeof(cmdbuff));
	len = sprintf(cmdbuff, "LIST -al\r\n"); 	
	if (send_and_recv(cmdsockfd, cmdbuff, len, recvbuff, sizeof(recvbuff)) < 0){
		printf("[cd_dir_list_files]:  [LIST -al] option failed, exit !\n");
		return -1;
	}

	if( !strstr(recvbuff, "125") ){
		printf("[cd_dir_list_files]: recv cmd server [LIST -al] reply failed, exit !\n");
		return -1;	
	}

	memset(recvbuff, 0, sizeof(recvbuff));
	if( (szie = recv(datasockfd, recvbuff, sizeof(recvbuff), 0)) < 0){
		printf("[cd_dir_list_files]: recv data socket [LIST -al] failed, exit !\n");
		return -1;
	}

	find_newest_upgfile(cmdsockfd, recvbuff, szie, downfile);
	
	return 0;
}

int ftp_client::find_newest_upgfile(int sock, char *recvdate, int szie, char *upgfile)
{
	if( !recvdate  || !upgfile || (szie <= 0) ){
		printf("[find_newest_upgfile]: Input [recvdate] or [upgfile] is NULL, or [szie] <=0,  exit !\n");
		return -1;
	}

	int i = 0;
	char *p = NULL;
	char buff[128] = {'\0'};
	char filelist[5][64] = {'\0'};
	char *tmpdata = (char *)recvdate;

	while( p = strstr(tmpdata, "\r\n") )
	{	
		memcpy(buff, tmpdata, p - tmpdata);	
		parse_file(buff, p - tmpdata, filelist[i]);
		i++;
		tmpdata = p+2;
		memset(buff, '\0', sizeof(buff));
	}

	int len = 0;
	int j = 0;
	char cmdbuff[256] = {'\0'};
	char recvbuff[1024] = {'\0'};
	int year = 0;
	int mouth = 0;
	int day = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	char *timedata = NULL;
	int sque = 0;
	unsigned long long maxtime = 0LL;
	unsigned long long tmptime = 0LL;
	
	for(; j < i; j++)//find the newest upgrade file.
	{
		if(strstr(filelist[j], ".bin") && strstr(filelist[j], "bst")){
			
			len = sprintf(cmdbuff, "MDTM %s\r\n", filelist[j]); 
			if (send_and_recv(sock, cmdbuff, len, recvbuff, sizeof(recvbuff)) < 0){
				printf("[find_newest_upgfile]:  [MDTM] option failed, exit !\n");
				return -1;
			}

			if( !strstr(recvbuff, "213") ){
				printf("[find_newest_upgfile]: recv cmd server [MDTM] reply failed, exit !\n");
				memset(recvbuff, '\0', sizeof(recvbuff));
				continue;
			}
			
			timedata = &recvbuff[4];
			sscanf(timedata, "%4d%2d%2d%2d%2d%2d", &year, &mouth, &day, &hour, &min, &sec);
			tmptime = (year*YEAR + mouth*MOU + day*DAY + hour*3600 + min*60 + min);
			if(tmptime > maxtime){
				maxtime = tmptime;
				sque = j;
			}
			
			memset(recvbuff, '\0', sizeof(recvbuff));
		}
	}

	memcpy(upgfile, filelist[sque], strlen(filelist[sque]) );
	printf("The newest files:[%d]:[%s]\n", sque, upgfile);
	return 0;
}

int ftp_client::parse_file(char *date, int size,  char *filename)
{
	if(date == NULL || filename == NULL || size <= 0){
		printf("Input error, exit !\n");
		return -1;
	}
	
	char *tmp = NULL;
	if( tmp = strrchr(date, ' ' ) )
	{
		tmp++;
		memcpy(filename, tmp, strlen(tmp));
		return 1;
	}

	return -1;
}

int ftp_client::down_upgfile(int cmdsockfd, int datasockfd, char *downfile)
{
	if(!downfile){
		printf("[cd_dir_list_files]:Input [downfile] is NULL, exit !\n");
		return -1;
	}

	FILE* fd;
	unsigned int filesize = 0;
	char cmdbuff[256] = {'\0'};
	char recvbuff[2048] = {'\0'};
	int szie = 0;
	int len = 0;

	len = sprintf(cmdbuff, "SIZE %s\r\n", downfile); 	
	if (szie = send_and_recv(cmdsockfd, cmdbuff, len, recvbuff, sizeof(recvbuff)) < 0){
		printf("[down_upgfile]:  [SIZE] option failed, exit !\n");
		return -1;
	}

	if( !strstr(recvbuff, "213") ){
		printf("[down_upgfile]: recv cmd server [SIZE] reply failed, exit !\n");
		return -1;	
	}
	
	filesize = atoi(&recvbuff[4]);
	printf("File size is[%d]\n", filesize);
	
	char filepth[86] = {'\0'};
	len = sprintf(filepth, "%s/%s", FILE_PTH, downfile);
	if( (fd = fopen(filepth, "a+")) <= 0){
		printf("Open file failed , error:%m\n");
		return -1;
	}

	int downport = 0;
	int downsock = 0;
	downport = enter_pasv_mode(cmdsockfd);
	if (downport < 0) return -1;
	downsock = create_data_sock(downport);
	if (downsock <= 0) return -1;


	memset(cmdbuff, 0, sizeof(cmdbuff));
	memset(recvbuff, 0, sizeof(recvbuff));
	
	len = sprintf(cmdbuff, "RETR %s\r\n", downfile); 	
	if (szie = send_and_recv(cmdsockfd, cmdbuff, len, recvbuff, sizeof(recvbuff)) < 0){
		printf("[down_upgfile]:  [RETR] option failed, exit !\n");
		return -1;
	}
	/*printf("RECV:[%s]\n", recvbuff);
	if( !strstr(recvbuff, "150") ){
		printf("[down_upgfile]: recv cmd server [RETR] reply failed, exit !\n");
		return -1;	
	}
	*/
	
	int downsize = filesize;
	int currsize = 0;
	int times = 0;
	int tmpsize = 0;
	
	while(tmpsize <= downsize)
	{
		if( (currsize = recv(downsock, (char *)recvbuff, sizeof(recvbuff), 0)) < 0){
			printf("[send_and_recv]: recv data socket failed, exit !\n");
			continue ;
		}

		if (fwrite(recvbuff, currsize, 1, fd) < 0){
			printf("write upgrate file data failed \n");
			continue;
		}
		
		tmpsize += currsize;
		if( (times++ % 8) == 0 ) //send live pack.
		{
			memset(cmdbuff, '\0', sizeof(cmdbuff));
			len = sprintf(cmdbuff, "%d", times);
			if( send(downsock, (char *)cmdbuff, len, 0) < 0){
				printf("[send_and_recv]: recv data socket failed, exit !\n");
				continue ;
			}
			
			fflush(fd);
			if(tmpsize % SIZE_M == 0 ){
				printf("current dowing toalsize[%d], currsize[%d]: [%d%%]\n", downsize, tmpsize, tmpsize / downsize);
				fflush(stdout);
			}
		}

	}

	fclose(fd);
	close(downsock);
}

int ftp_client::quit_ftp(int cmdsock, int datasock)
{
	char cmdbuff[256] = {'\0'};
	char recvbuff[256] = {'\0'};
	int szie = 0;
	int len = 0;
	len = sprintf(cmdbuff, "QUIT\r\n"); 	
	if ((szie = send_and_recv(cmdsock, cmdbuff, len, recvbuff, sizeof(recvbuff))) < 0){
		printf("[enter_pasv_mode]:  [PASV] option failed, exit !\n");
		return -1;
	}

	printf("RECV:%s\n", recvbuff);
	char *tmp = NULL;
	if( !strstr(recvbuff, "221")){
		printf("[enter_pasv_mode]: recv server [PASV] reply failed, exit !\n");
		return -1;	
	}

	close(cmdsock);
	close(datasock);
	return 0;
}

int ftp_client::send_and_recv(int sock,  char *sendbuff, int sendsize, const char *recvbuff, int recvsize)
{	
	if(sock <= 0){
		printf("[send_and_recv]: Input [sock] <= 0, exit !\n");
		return -1;
	}

	if( (!sendbuff || !recvbuff)  || (sendsize <= 0 || recvsize <= 0) ){
		printf("[send_and_recv]: Input [sendbuff] or [recvbuff] is NULL, OR [sendsize] or [recvsize] <= 0, exit !\n");
		return -1;
	}
	
	int size = 0;
	if(send(sock, sendbuff, sendsize, 0) < 0){
		printf("[send_and_recv]: send socket failed, exit !\n");
		return -1;
	}

	if( (size = recv(sock, (char *)recvbuff, recvsize, 0)) < 0){
		printf("[send_and_recv]: recv socket failed, exit !\n");
		return -1;
	}

	return size;
}

int ftp_client::create_data_sock(int port)
{
	if(port <= 0 ){
		printf("[create_data_sock]: Input [port] <= 0, exit !\n");
	}

	struct sockaddr_in ftp_data_in;
	memset(&ftp_data_in, 0,  sizeof(struct sockaddr_in));

	int oneopt = 1;
	ftp_data_in.sin_family = AF_INET;
	ftp_data_in.sin_port = htons(port);
	ftp_data_in.sin_addr.s_addr = addr_m.s_addr;

	int sock = 0;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		perror("Create sock failed, error:");
		return -1;
	}
	
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &oneopt, sizeof(oneopt));
	if( connect(sock, ( struct sockaddr *)&ftp_data_in, sizeof(ftp_data_in)) < 0){
		perror("Connect sock failed, error:");
		return -1;
	}

	fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) & ~O_NONBLOCK);
	printf("cteate data sock [%d]\n", sock);
	
	return sock;
}

int ftp_client::create_sock(const char *hostname)
{
	if(hostname == NULL){
		printf("[start_ftp]: Input [hostname] is NULL, exit\n");
		return -1;
	}

	int oneopt = 1;
	struct sockaddr_in ftp_in;
	struct in_addr addr;
	memset(&ftp_in, 0,  sizeof(struct sockaddr_in));
	memset(&addr, 0,  sizeof(struct in_addr));
	
	ftp_in.sin_family = AF_INET;
	ftp_in.sin_port = htons(21);
	get_serverip(hostname, &addr);
	ftp_in.sin_addr.s_addr = addr.s_addr;
	addr_m = addr;
	printf("[%s]\n", inet_ntoa(ftp_in.sin_addr) );

	int sock = 0;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		perror("Create sock failed, error:");
		return -1;
	}
	
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &oneopt, sizeof(oneopt));
	if( connect(sock, ( struct sockaddr *)&ftp_in, sizeof(ftp_in)) < 0){
		perror("Connect sock failed, error:");
		return -1;
	}

	fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) & ~O_NONBLOCK);

	return sock;
}

bool ftp_client::get_serverip(const char *hostname, struct in_addr *addr)
{
	if( (hostname == NULL) || (addr == NULL)){
		printf("[start_ftp]: Input [hostname] OR [*addr] is NULL, exit\n");
		return -1;
	}

	char serhost[64] = {'\0'};
	char *tmp =NULL;
	char *end =NULL;
	struct hostent *host = NULL;
	
	if( strstr((char *)hostname, FTP_FLAG))
	{
		tmp = (char *)(hostname + strlen(FTP_FLAG));
		if(inet_aton(tmp, addr)){
			return true;
		}
		else{
			snprintf(serhost, strlen(tmp),  "%s", tmp);
			printf("[%s]\n", serhost);
			if( (host = gethostbyname(serhost))  )
			{
				addr->s_addr =  *((uint32_t *)host->h_addr_list[0]);
				printf("[get_serverip]:Get server ip [%s] \n", inet_ntoa(*((struct in_addr *)host->h_addr_list[0])) );
				return true;
			}
		}
	}

	printf("[get_serverip]: parse hostname failed, exit !\n");
	return false;
}
