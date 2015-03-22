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
   	uint32_t intermediate = 0;
   	//get SHA-1 of preImage
   	size_t length = sizeof(preImage);
   	unsigned char hash[SHA_DIGEST_LENGTH];
   	SHA1(preImage, length, hash);
   	uint32_t SHA_output = atoi(hash);

   	for (int i = 0; i < 4; i++){
   		intermediate = memcpy(&image, (void*) (output+i), 4);
   		image = image ^ intermediate;
   	}
   	return image;
   }