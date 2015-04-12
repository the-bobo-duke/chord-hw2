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
#define KEEP_ALIVE 12
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
#define SUCC_REQ 13
#define SUCC_REPLY 14
#define MAX_CMSG_LENGTH 80 // chord_msg is 64 bytes, upped to 80 just in case

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
   *
   *
   * For SRCH_REQ's, the return value should specify closest_pred
   * and successor should be the successor of closest_pred
   *
   *
   *
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
void * initFingerTable(char * fargv[]);
void updateOthers();
Node_id findSuccessor(int args[]);
Node_id whoisMySuccessor();


/* ========================================================================
   ========================================================================
      LISTENER FUNCTIONS
   ========================================================================
   ========================================================================
*/

/* ========================================================================
      WHOIS MY SUCCESSOR FUNCTION
   ========================================================================
*/
   /*
   *
   *
   * Returns my successor
   *
   *
   */

   Node_id whoisMySuccessor(){
      Node_id node = Fingers[0].node;
      return node;
   }

/* ========================================================================
      FIND SUCCESSOR FUNCTION
   ========================================================================
*/
   /*
   *
   * 
   * Finds successor of target_key iteratively
   * Returns the closest_pred of target_key 
   * Function that calls findSuccessor() must check to see if target_key
   * is in (closest_pred, succ(closest_pred)] -- must request succ from closest_pred
   *
   *
   */

   Node_id findSuccessor(int args[]){
      fprintf(stderr, "In findSuccessor\n");
      int connfd = args[0];
      uint16_t myPort = args[1];
      uint32_t target_key = args[2];
   
      // lock a mutex -- or should i just return something and not be a thread?
      int i = 0;
      for (i = 0; i <= 30; i++){
         int A = Fingers[i].start;
         int B = Fingers[i+1].start;
         if (target_key >= A && target_key < B)
            return Fingers[i].node;
         else {
            if (i == 30)
               return Fingers[31].node;
         }
      }

      // unlock mutex
      // destroy mutex?
      Node_id error;
      error.port = 0;
      return error; // some error must have happened
   }

/* ========================================================================
      UPDATE OTHERS FUNCTION
   ========================================================================
*/
   /*
   *
   * 
   * Updates other nodes after a node has joined and seeded its finger table
   * 
   * 
   * 
   */

   void updateOthers(){
      return;
   }




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

void * initFingerTable(char * fargv[]){ 

   // open connection to n-prime
   //char * remote_IP = fargv[0];
   fprintf(stderr, "fargv[0]: %s\n", fargv[0]);
   uint16_t remote_Port = atoi(fargv[1]);
   fprintf(stderr, "remote_Port: %d\n", remote_Port);
   int serverfd;
   serverfd = Open_clientfd(fargv[0], remote_Port);

   if ( serverfd < 0 )
      {
         fprintf(stderr, "Failed to connect to %s:%d\nExiting\n", fargv[0], remote_Port);
         exit(EXIT_FAILURE);
      }
   fprintf(stderr, "Successfully connected to: %s on %d\n", fargv[0], remote_Port);
   sleep(3);

   fprintf(stderr, "Seeding my finger table by asking %s:%d for help\n", fargv[0], remote_Port);
   
   //send chord_msg's for each Finger[i]
   int i;
   for (i = 0; i <= 31; i++){
      chord_msg cmsg = (chord_msg){ .mtype = SRCH_REQ, .target_key = Fingers[i].start, .finger_index = i };
      chord_msg * cmsg_ptr = &cmsg;
      //fprintf(stderr, "Requesting cmsg.mtype: %d    cmsg.target_key: %d\n", cmsg.mtype, cmsg.target_key);
      rio_writen(serverfd, cmsg_ptr, MAX_CMSG_LENGTH);   
   }
   
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
      
      //int initFingerTable_flag;

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
         fprintf(stderr, "Starting a new ring\n");
         fprintf(stderr, " My ip is: %s\n My port is: %u\n My chord id is: %u\n", myIP, myPort, myHash);


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

         fprintf(stderr, " My ip is: %s\n My port is: %u\n My chord id is: %u\n", myIP, myPort, myHash);

         //blank my fingers
         int i;
         for (i = 0; i <= 31; i++){
            uint32_t offset;
            offset = 1 << i;
            Fingers[i].start = myself.pos + offset;
         }


         //call initFingerTable
         char * fargv[2];
         fargv[0] = argv[2];
         fargv[1] = argv[3];
         Pthread_create(&tid, NULL, initFingerTable, fargv);
         Pthread_detach(tid); //frees tid so we can make the next thread

         //call updateOthers
         updateOthers();

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

      int connfd = args[0];
      uint16_t myPort = args[1];
      int newargv[3];
         newargv[0] = connfd;
         newargv[1] = myPort;
         //newargv[2] = cmsg.target_key;

      pthread_t tid;

      chord_msg replyMsg = (chord_msg) { .mtype=999 };
      fprintf(stderr, "Value of replyMsg.mtype is: %d \n", replyMsg.mtype);

      //chord_msg * rMsg_ptr = &replyMsg;
      
      while(1){
         char usrbuf[MAX_CMSG_LENGTH];
         rio_readn(connfd, usrbuf, MAX_CMSG_LENGTH);
         chord_msg cmsg;
         chord_msg * cmsg_ptr = &cmsg;
         memcpy(cmsg_ptr, usrbuf, sizeof(chord_msg));

         /* trash
         char usrbuf3[MAX_CMSG_LENGTH];
         Node_id result2;
         chord_msg cmsg3 = (chord_msg){ .mtype = 999 };
         chord_msg * cmsg_ptr3 = &cmsg3;
         chord_msg cmsg2 = (chord_msg){ .mtype = SUCC_REPLY, .my_successor = NULL }; //.my_successor = result2 };
         chord_msg * cmsg_ptr2 = &cmsg2;
         */

         //fprintf(stderr, "Value of usrbuf cmsg.mtype: %d   cmsg.target_key: %u\n", cmsg.mtype, cmsg.target_key);
         
         // switch handler for various message types
         switch(cmsg.mtype){
            case 999 :
               fprintf(stderr, "Success, received a reply!\n");
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            case KEEP_ALIVE :
               break;
            case KEEP_ALIVE_ACK :
               break;
            case SRCH_REQ :
               fprintf(stderr, "In SRCH_REQ at line: %d\n", __LINE__);
               int x = rio_writen(connfd, &replyMsg, MAX_CMSG_LENGTH);   
               fprintf(stderr, "Value of x is: %d\n", x);
                  /*
               //findSuccessor();
               newargv[2] = cmsg.target_key;
               Node_id result = findSuccessor(newargv);
               if (result.port == 0){
                     fprintf(stderr, "Error on findSuccessor call line: %d, cmsg.target_key: %u\n", __LINE__, cmsg.target_key);
                  }
               else {
                  // request successor of result node;
                  char usrbuf2[MAX_CMSG_LENGTH];
                  chord_msg cmsg2 = (chord_msg){ .mtype = SUCC_REPLY, .my_successor = result };
                  chord_msg * cmsg_ptr2 = &cmsg2;
                  rio_writen(connfd, cmsg_ptr2, MAX_CMSG_LENGTH);   
                  // evaluate if target_key is >= result and < result.successor
                  // if yes, then result is the answer
                  // otherwise, run findSuccessor again on result node

                  
                  
               }
               */
               //clear out cmsg
               //cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
               
            case SRCH_REPLY :
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            case QUERY_CONN_REQ :
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            case QUERY_REQ :
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            case QUERY_REPLY :
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            case UPDATE_PRED :
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            case UPDATE_FING :
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            case PRED_REQ :
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            case PRED_REPLY :
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            case SUCC_REQ :
               /*
               result2 = whoisMySuccessor();
               char usrbuf2[MAX_CMSG_LENGTH];
               cmsg2.my_successor = result2;
               rio_writen(connfd, cmsg_ptr2, MAX_CMSG_LENGTH);   
               */
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            case SUCC_REPLY :
               //clear out cmsg
               cmsg.mtype = 0; 
               memset(usrbuf, 0, MAX_CMSG_LENGTH);
               break;
            default :
               // just continue listening
               break;
         }
         
      }

      // make a define like MAX_CMSG_LENGTH and use that for rio_readn
      // so long as mtype is always the first int there we can just look for that
      // to i.d. the message type

      /*
         Need to while(1) for a message from the connected node
         
         Need to analyze what the message is asking for

         And then serve the requested content
      */


   }