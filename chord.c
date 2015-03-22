/*
* (c) Abhishek Bose-Kolanu, March 21 2015
* Assignment 2: Chord
* CPS 512 Distributed Systems
* Professor Bruce Maggs
* TA Zhong Wang
*/

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


/* ========================================================================
		NODE_ID STRUCT
   ========================================================================
*/
   typedef struct Node_id {
   	char * ip; //IPv4 or IPv6 address in ASCII format. will need inet_addr() or inet_aton() to cast it
   	uint16_t port;
   	uint32_t pos; //position on our 32-bit ring
   } Node_id;



/* ========================================================================
		PREDECESSOR STRUCT
   ========================================================================
*/
   typedef struct Predecessor_t {
   	Node_id pNode_id; //holds a predecessor node
   } Predecessor_t;
/*
   on startup do: 
   struct Predecessor_t Predecessors[2];
   Predecessors[1] = some Node_id;
   Predecessors[2] = some other Node_id;
 */


/* ========================================================================
		FINGER STRUCT
   ========================================================================
*/
   typedef struct Finger_t {
   	uint32_t start; //n + 2^(i-1) where 1 <= i <= 32 and i is the index of the finger
   					//starting from 1, so 1st finger's i = 1
   	// interval: [finger[i].start, finger[i+1].start); if (key >= A && key < B) { this finger is responsible}
   	Node_id node;
   } Finger_t;
/*
	on startup do:
	struct Finger_t Fingers[32];
	for (int i = 0; i <= 31; i++){
		Fingers[i].start = n + 2^(i); //write this without the ^ operator
		//n is the pos of the current node
		//i is already "minus 1" because 0 <= i <= 31
	}

/* ========================================================================
   ========================================================================
		CHORD MESSAGE STRUCTS
   ========================================================================
   ========================================================================
*/

/* ========================================================================
		KEEP_ALIVE STRUCT
   ========================================================================
*/
   typedef struct Keep_Alive_m{
   	int mtype; //message type: KEEP_ALIVE or KEEP_ALIVE_ACK
   } Keep_Alive_m;

/* ========================================================================
		SEARCH_REQ STRUCT
   ========================================================================
*/
   typedef struct Search_Req_m{	
   	int mtype; //must be initialized to SRCH_REQ;
   	uint32_t key; // target key
   } Search_Req_m;
   
/* ========================================================================
		SEARCH_REPLY STRUCT
   ========================================================================
*/
   typedef struct Search_Reply_m{
   	int mtype; //must be initialized to SRCH_REPLY;
   	Node_id closest_pred;
   	Node_id my_successor;
   } Search_Reply_m;
   //returns my closest predecessor to key k, which might be myself, and 
   //my successor

/* ========================================================================
		QUERY_CONN_REQ STRUCT
   ========================================================================
*/
   typedef struct Query_Conn_Req_m{
   	int mtype; //must be initialized to QUERY_CONN_REQ;
   } Query_Conn_Req_m;

/* ========================================================================
		QUERY_REQ STRUCT
   ========================================================================
*/
   typedef struct Query_Req_m{
   	int mtype; //must be initialized to QUERY_REQ;
   	uint32_t key; // target key
   } Query_Req_m;

/* ========================================================================
		QUERY_REPLY STRUCT
   ========================================================================
*/
   typedef struct Query_Reply_m{
   	int mtype; //must be initialized to QUERY_REPLY;
   	uint32_t key; // the target key from Query_Req_m
   	Node_id responsible; // node responsible for key
   } Query_Reply_m;

/* ========================================================================
		UPDATE_ENTRY STRUCT
   ========================================================================
*/
   typedef struct Update_Entry_m{
   	int mtype; // UPDATE_PRED or UPDATE_FING
   	Node_id new_node;
   	int finger_index; //0 to 31, which represents the finger 1 to 32
   					  //remember, The 32nd finger is has a start of n+2^31
   } Update_Entry_m;

/* ========================================================================
		PRED_REQ STRUCT
   ========================================================================
*/
   typedef struct Pred_Req_m{
   	int mtype = PRED_REQ;
   } Pred_Req_m;

/* ========================================================================
		PRED_REPLY STRUCT
   ========================================================================
*/
   typedef struct Pred_Reply_m{
   	int mtype; //must be initialized to PRED_REPLY;
   	Node_id my_predecessor;
   } Pred_Reply_m;


/* ========================================================================
   ========================================================================
		UTILITY FUNCTIONS
   ========================================================================
   ========================================================================
*/


/* ========================================================================
		HASHING FUNCTION
   ========================================================================
*/
   /*
   *
   * Acecpts a string to produce a SHA-1 hash of
   *
   * Returns a 4-byte uint32_t that is the bitwise  
   * XOR of each 4-byte chunk of the SHA-1 hash
   *
   * This is equivalent to the position on our 32-bit 
   * chord ring
   *
   */

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


/* ========================================================================
		FIND_MY_IP FUNCTION
   ========================================================================
*/
   /*
   *
   * Finds the IPv4 address of the local node
   * Assumes machine has only one non-loopback address 
   *
   * Returns IPv4 in ASCII format
   *
   * Note: will return on first IPv4 it finds that is NOT
   * 127.0.0.1 or 0.0.0.0
   * 
   * Note: eth0 = first physical ethernet device on Linux
   * en0 = physical wired device on OSX
   * en1 = physical wifi device on OSX
   *
   */

   char * findMyIP()
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

        	if ( !areSame && !areSame2) { //if they're different
        		return inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
        	}
    	} 
	}
	
	if (ifAddrStruct!=NULL) {
		freeifaddrs(ifAddrStruct);
	}
	
	return 0;
   }



/* ========================================================================
   ========================================================================
		MAIN FUNCTION
   ========================================================================
   ========================================================================
*/

   int main(int argc, char *argv[])
   {
   	
   	if (argc < 2) {
   		printf("Usage: %s port OR %s [IP Address] [remote port]\n", argv[0], argv[0]);
   		exit(1);
   	}

   	uint16_t myPort;
   	char * myIP = findMyIP();


   	if (argc == 2) {
   		myPort = atoi(argv[1]);
   		//do n.join where i'm the only node
   		char * s2 = argv[1];
   		char * toHash = malloc(strlen(s2) + strlen(myIP) + 1);
   		strcpy(toHash, myIP);
   		strcat(toHash, s2);
   		uint32_t myHash = giveHash(toHash);
   	}

   	if (argc == 3){
   		char * remote_IP = argv[1];
   		uint16_t remote_Port = argv[2];
   		//do n.join with a remote node
   	}

   }