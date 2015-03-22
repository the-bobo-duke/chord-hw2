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
   	uint32_t intermediate = 0;
   	
   	//get SHA-1 of preImage
   	size_t length = sizeof(preImage);
   	unsigned char hash[SHA_DIGEST_LENGTH];
   	SHA1(preImage, length, hash);
   	int i;
   	for (i = 0; i < sizeof(hash); i+=4){
   		intermediate = memcpy(&image, (void*) (hash+i), 4);
   		image = image ^ intermediate;
   	}
   	return image;
   }

int main()
{
	printf("Program beginning \n");
	printf("Let's find the Chord ID for the string: StupendousOhMyGodAwesomeCool\n");
	char * our_string = "StupendousOhMyGodAwesomeCool";
	uint32_t chord_id = giveHash(our_string);
	fprintf(stderr, "The Chord ID is: %u\n", chord_id);
	return 0;
}