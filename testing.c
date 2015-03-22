#include <stdio.h>
#include "csapp.h"
#include <pthread.h>
#include <openssl/sha.h>

#include <string.h> //why not?
#include <sys/socket.h> //could be useful
#include <sys/types.h> //sounds important
#include <arpa/inet.h> //see above
#include <unistd.h> //who knows?
#include <stdlib.h> //standard sounds good

#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>


#define MAX_HEADER 8192
#define KEEP_ALIVE 0
#define KEEP_ALIVE_ACK 1
#define SRCH_REQ 2
#define SRCH_REPLY 3
#define QUERY_CONN_REQ 4
#define QUERY_REQ 5
#define QUERY_REPLY 6
#define UPDATE_PRED 7
#define UPDATE_FING 10
#define PRED_REQ 8
#define PRED_REPLY 9

uint32_t giveHash(char * preImage)
{
	uint32_t image = 0;

   	//get SHA-1 of preImage
	size_t length = sizeof(preImage);
	unsigned char hash[SHA_DIGEST_LENGTH];
	SHA1(preImage, length, hash);
	int i;
	for (i = 0; i < sizeof(preImage); i+=4){
		uint32_t intermediate;
		memcpy(&intermediate, (void*) (hash+i), 4);
		image = image ^ intermediate;
	}
	return image;
}


char * findMyIP()
{
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

 	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

 	/* I want IP address attached to "en1" */
	strncpy(ifr.ifr_name, "en1", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

}

char * findMyIP2()
{
	struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
        	tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
        	char addressBuffer[INET_ADDRSTRLEN];
        	inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

        	uint32_t ip1 = *(uint32_t*) &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
        	char * src2 = "127.0.0.1";
        	uint32_t ip2;
        	inet_pton(AF_INET, src2, &ip2);
        	_Bool areSame = !(ip1 ^ ip2);

        	char * src3 = "0.0.0.0";
        	uint32_t ip3;
        	inet_pton(AF_INET, src3, &ip3);
        	_Bool areSame2 = !(ip2 ^ ip3);

        	if ( !areSame && !areSame2 ) { //if they're different, print
        		printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        		return inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
        	}
        } 
        /*
        else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
        } 
        */
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return 0;
}
int main(int argc, char *argv[])
{

	if (argc < 2) {
		printf("Usage: %s port OR %s [IP Address] [remote port]\n", argv[0], argv[0]);
		exit(1);
	}

	uint16_t myPort;
	char * myIP2 = findMyIP2();
	char * myIP = findMyIP();
	long long tester = 1;
	tester = tester << 32;

	if (argc == 2) {
		myPort = atoi(argv[1]);
   		//do n.join where i'm the only node
		char * s2 = argv[1];
		char * toHash = malloc(strlen(s2) + strlen(myIP2) + 1);
		strcpy(toHash, myIP2);
		strcat(toHash, s2);
		uint32_t myHash = giveHash(toHash);
		fprintf(stderr, "My Chord ID is: %u\n", myHash);
		fprintf(stderr, "My port is: %d\n", myPort);
		fprintf(stderr, "My IP from findMyIP1 is: %s\n", myIP);
		fprintf(stderr, "My IP from findMyIP2 is: %s\n", myIP2);

		if (myHash < tester){
			fprintf(stderr, "Still a uint32\n");
		}
	}

	return 0;
}