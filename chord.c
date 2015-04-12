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
#include <ifaddrs.h>

#include <inttypes.h>

#define MAX_HEADER 8192
#define KEEP_ALIVE 22
#define KEEP_ALIVE_ACK 11
#define SRCH_REQ 2
#define SRCH_REPLY 3
#define QUERY_CONN_REQ 4
#define QUERY_REQ 5
#define QUERY_REPLY 6
#define UPDATE_PRED 7
#define UPDATE_FING 10
#define PRED_REQ 8
#define PRED_REPLY 9
#define MAX_CMSG_LENGTH 80 // largest chord message is chord_reply at 40 bytes

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

/*
   ********* Set unused fields to NULL before sending over the wire
*/

typedef struct chord_msg{
   int mtype; // see #DEFINES at top for choices
   uint32_t target_key; //target_key for a search
   Node_id closest_pred; //closest_pred to some searched-for key k OR my_predecessor if 
                           // this is a Pred_Reply_m
   Node_id my_successor; //my_successor for node responding to a search request
   Node_id new_node; //for insertion into finger table if it's an update message
   int finger_index; //where to insert the new_node
} chord_msg;

/*
*
*
* The below messages have been collapsed into one "chord_msg" above
*
*
typedef struct chord_req{
   int mtype; // see #DEFINES at top for choices
   uint32_t target_key; //target_key for a search
} chord_req;

typedef struct chord_reply{
   int mtype; // see #DEFINES at top for choices
   Node_id closest_pred; //closest_pred to some searched-for key k OR my_predecessor if 
                           // this is a Pred_Reply_m
   Node_id my_successor;
} chord_reply;

typedef struct chord_update{
   int mtype; // see #DEFINES at top for choices
   Node_id new_node;
   int finger_index; //where to insert the new_node
} chord_update;
*/



/* ========================================================================
   ========================================================================
      GLOBAL VARIABLES
   ========================================================================
   ========================================================================
*/



Predecessor_t Predecessors[2];
Finger_t Fingers[32];   
Node_id myself;





/* ========================================================================
   ========================================================================
      FUNCTION DECLARATIONS
   ========================================================================
   ========================================================================
*/

void *threadFactory(int args[]);
uint32_t giveHash(char * preImage);
char * findMyIP();
Node_id nodeConstructor(char * ip, uint16_t port, uint32_t hash);
void initFingerTable(char * remote_IP, uint16_t remote_Port);




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
   * Acecpts a string to produce a SHA-1 hash
   *
   * Returns a 4-byte uint32_t that is the bitwise  
   * XOR of each 4-byte chunk of the SHA-1 hash
   *
   * This is equivalent to the position on our 32-bit 
   * chord ring
   *
   * Verified to consistently produce same output on same inputs
   * 
   */

   uint32_t giveHash(char * preImage)
   {
      uint32_t image = 0;
            //fprintf(stderr, "In giveHash, preImage is: %s\n", preImage);
      //get SHA-1 of preImage
      size_t length = strlen(preImage);
            //fprintf(stderr, "In giveHash, length is: %zu\n", length);
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
      HELPER FUNCTIONS
   ========================================================================
   ========================================================================
*/



/* ========================================================================
      NODE_CONSTRUCTOR FUNCTION
   ========================================================================
*/

   /*
   *
   * Node constructor
   *
   */

   Node_id nodeConstructor(char * ip, uint16_t port, uint32_t hash){
      Node_id node;
      node.ip = ip;
      node.port = port;
      node.pos = hash;
      return node;
   }


/* ========================================================================
      INIT_FINGER_TABLE FUNCTION
   ========================================================================
*/
   /*
   *
   * Performs n.init_finger_table(n') from Figure 6 of Chord Paper
   *
   * Verified to actually edit the finger table array back in main
   *
   * Needs: Accept(); rcv_search; find_successor; update_entry; whois_your_predecessor;
   *
   */

void initFingerTable(char * remote_IP, uint16_t remote_Port){ //pass in finger table struct array by reference

   // open connection to n-prime
   //char * remote_IP_t = remote_IP;
   //uint16_t remote_Port_t = remote_Port;
   int serverfd;
   serverfd = Open_clientfd(remote_IP, remote_Port);
   if ( serverfd < 0 )
      {
         fprintf(stderr, "Failed to connect to %s:%d\nExiting\n", remote_IP, remote_Port);
         exit(EXIT_FAILURE);
      }
   fprintf(stderr, "Successfully connected to: %s on %d\n", remote_IP, remote_Port);

   fprintf(stderr, "Seeding my finger table by asking %s:%d for help\n", remote_IP, remote_Port);
   
   //send chord_msg's for each Finger[i]
   chord_msg cmsg = (chord_msg){ .mtype = SRCH_REQ, .target_key = 0 };
   chord_msg * cmsg_ptr = &cmsg;
   fprintf(stderr, "cmsg.mtype: %d    cmsg.target_key: %d\n", cmsg.mtype, cmsg.target_key);
   rio_writen(serverfd, cmsg_ptr, MAX_CMSG_LENGTH);

/*
   Fingers[0].node = ask n-prime to find_successor(Fingers[1].start);
   set my predecessor to the predecessor of my successor;
   tell my successor that i am its new predecessor;
   int i;
   for (i = 0; i <= 31; i++){
      if (n <= Fingers[i+1].start < Fingers[i].node){
         Fingers[i+1].node = Fingers[i].node;
      }
      else{
         Fingers[i+1].node = ask n-prime to find_successor(Fingers[i+1].start);
      }
   }
*/
   int i = 0;
   for (i = 0; i <= 31; i++){
      Fingers[i].node.pos = 35;
      //fprintf(stderr, "Value of Fingers[%d].start is: %u\n", i, Fingers[i].start);
   }
   //return Fingers;
   //if not void, needs to return pointer to first element of array of structs
}



/* ========================================================================
   ========================================================================
      MAIN FUNCTION
   ========================================================================
   ========================================================================
*/

   int main(int argc, char *argv[])
   {
      //see GLOBAL VARIABLES for global variables declared ahead of main

      if (argc < 2) {
         printf("Usage: %s [local port] to start a new ring \nOR %s [local port] [IP Address] [remote port] to join\n", argv[0], argv[0]);
         exit(1);
      }

      uint16_t myPort = atoi(argv[1]);
      uint32_t myHash;
      char * myIP = findMyIP();

      int listenfd, connfd, clientlen, optval;
      struct sockaddr_in clientaddr;
      pthread_t tid;
      //Fingers, Predecessors, and myself are global variables

      if (argc == 2) {
         //do n.join where i'm the only node


         //find my Chord ID
         char * s2 = argv[1];
         char * toHash = malloc(strlen(s2) + strlen(myIP) + 1);
         strcpy(toHash, myIP);
         strcat(toHash, s2);
         myHash = giveHash(toHash);
         myself = nodeConstructor(myIP, myPort, myHash);

         fprintf(stderr, "My chord id is: %u\n", myHash);


         //initialize Predecessors and Finger Table
         Predecessors[1].pNode_id = myself;
         Predecessors[2].pNode_id = myself;

         int i;
      for (i = 0; i <= 31; i++){
            Fingers[i].node = myself;
            uint32_t offset;
            offset = 1 << i;
            Fingers[i].start = myself.pos + offset; 
            //fprintf(stderr, "myself.pos is: %u, offset is: %u\n", myself.pos, offset);
            //fprintf(stderr, "Fingers[%d].start is: %u\n", i, Fingers[i].start);
         }

      }

      if (argc == 4){
         char * remote_IP = argv[2];
         uint16_t remote_Port = atoi(argv[3]);
         //do n.join with a remote node

         //find my Chord ID
         char * s2 = argv[1]; // this is just our port
         char * toHash = malloc(strlen(s2) + strlen(myIP) + 1);
         strcpy(toHash, myIP);
         strcat(toHash, s2);
         myHash = giveHash(toHash);
         myself = nodeConstructor(myIP, myPort, myHash);

         fprintf(stderr, "My chord id is: %u\n", myHash);

         //blank my fingers
         int i;
         for (i = 0; i <= 31; i++){
            uint32_t offset;
            offset = 1 << i;
            Fingers[i].start = myself.pos + offset;
         }

         //call initfingers - do we need to start listening before we call this?
         initFingerTable(remote_IP, remote_Port);


      }

   /* ================================================================
   * START LISTENING ON MY PORT
   * =================================================================
   */
   listenfd = Open_listenfd(myPort);
   optval = 1;
   if ( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) < 0 ){
      fprintf(stderr, "Error on line: %d\n", __LINE__);
      perror("Error: ");
      return -1;
   }

   while(1){
      clientlen = sizeof(clientaddr);
      connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
      if (connfd >= 0){
         fprintf(stderr, "Accepted a connection, line: %d\n", __LINE__);
         
         char usrbuf[MAX_CMSG_LENGTH];
         rio_readn(connfd, usrbuf, MAX_CMSG_LENGTH);
         chord_msg cmsg;
         chord_msg * cmsg_ptr = &cmsg;
         memcpy(cmsg_ptr, usrbuf, sizeof(chord_msg));
         fprintf(stderr, "Value of usrbuf cmsg.mtype: %d   cmsg.target_key: %d\n", cmsg.mtype, cmsg.target_key);

         int newargv[2];
         newargv[0] = connfd;
         newargv[1] = myPort;
         //fprintf(stderr, "Value of connfd before packaging is: %d\n", connfd);
         Pthread_create(&tid, NULL, threadFactory, newargv);
         Pthread_detach(tid); //frees tid so we can make the next thread
      }

   }

   return 0; //exit main
   }

   void *threadFactory(int args[]){
      fprintf(stderr, "In threadFactory\n");

      // make a define like MAX_CMSG_LENGTH and use that for rio_readn
      // so long as mtype is always the first int there we can just look for that
      // to i.d. the message type

      /*
         Need to while(1) for a message from the connected node
         
         Need to analyze what the message is asking for

         And then serve the requested content
      */


   }