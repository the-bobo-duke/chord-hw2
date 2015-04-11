

int open_clientfd(char *hostname, int port)
{
  int clientfd;
  struct hostent *hp;
  struct sockaddr_in serveraddr;
  if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1; /* Check errno for cause of error */
/* Fill in the server’s IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL)
    return -2; /* Check h_errno for cause of error */
      bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0],
      (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);
    /* Establish a connection with the server */
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
      return -1;
    return clientfd;
  }

int toMalloc = 3 * sizeof(int) + sizeof(uint16_t) + sizeof(Node_id);
         Package_t *newargv = malloc(sizeof(Package_t));
         fprintf(stderr, "Value of connfd before packaging is: %d\n", connfd);
         newargv->connfd = connfd;
         newargv->myPort = myPort;
         newargv->myself = myself;
         newargv->Predecessors = Predecessors;
         newargv->Fingers = Fingers;
         Pthread_create(&tid, NULL, threadFactory, newargv);
         Pthread_detach(tid); //frees tid so we can make the next thread

/*
typedef struct Package_t {
   int connfd;
   uint16_t myPort;
   Node_id myself;
   Predecessor_t Predecessors[2];
   Finger_t Fingers[32];
   } Package_t;*/


newargv[0] = connfd; // int
newargv[1] = myPort; // uint16_t
newargv[2] = myself; // Node_id
newargv[3] = Predecessors; // int, because it's a ptr
newargv[4] = Fingers; // int, because it's a ptr

  
/* ========================================================================
      BLANK_FINGER_TABLE_CONSTRUCTOR FUNCTION
   ========================================================================
*/

   /*
   *
   * Correctly initializes each Fingers[i].start
   *
   * Fingers[i].node remains unassigned
   *
   * Returns blank, 32-entry finger table
   *
   */


   Finger_t blankFingers(Finger_t * Fingers, Node_id node){
      int i;
      for (i = 0; i <= 31; i++){
         uint32_t offset;
      offset = 1 << i;
         Fingers[i].start = node.pos + offset;
      }
      return Fingers[32];
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

         if ( !areSame && !areSame2) { //if they're different, print
            return addressBuffer;
            //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
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