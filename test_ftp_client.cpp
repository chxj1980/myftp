#include "ftp_client.h"

int main()
{
	ftp_client ftp;

	const char * host  = "ftp://sproxxe.com/";
	const char * user  = "xx99300065";
	const char * passd = "Sprxx2302";
	
	ftp.start_ftp(host, user, passd);
	return 0;
}
