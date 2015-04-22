# chord-hw2
A scaled down (no concurrent joins/exits/queries) version of Chord for Bruce Maggs' CPS 512 Distributed Systems Duke University Spring 2015

To run: 
1) Starting a new ring: make && ./chord 8008
2) Joining existing ring: ./chord 8080 192.168.254.5 8008

Replace the IP and Port pairs with whatever you want. When you start a new ring or start a new client that joins an existing ring its IP, Port, and Chord ID will all print to console.

Current error:
In initFingerTable() when rpcWrapper() is called we get the following error:

Assertion failed: (rc >= 0), function si_item_release, file /SourceCache/Libinfo/Libinfo-392.1/lookup.subproj/si_data.c, line 181.

This is with gcc --version:
i686-apple-darwin11-llvm-gcc-4.2 (GCC) 4.2.1 (Based on Apple Inc. build 5658) (LLVM build 2336.11.00)


The error is more descriptive when we compile with xyz:

make && ./chord 8008
gcc -c chord.c && gcc -c csapp.c
csapp.c:265:31: warning: passing 'int *' to parameter of type 'socklen_t *'
      (aka 'unsigned int *') converts between pointers to integer types with
      different sign [-Wpointer-sign]
    if ((rc = accept(s, addr, addrlen)) < 0)
                              ^~~~~~~
/usr/include/sys/socket.h:610:69: note: passing argument to parameter here
int     accept(int, struct sockaddr * __restrict, socklen_t * __restrict)
                                                                        ^
1 warning generated.
gcc -pthread csapp.o chord.o -o chord
clang: warning: argument unused during compilation: '-pthread'
Undefined symbols for architecture x86_64:
  "_main", referenced from:
     implicit entry/start for main executable
ld: symbol(s) not found for architecture x86_64
clang: error: linker command failed with exit code 1 (use -v to see invocation)
make: *** [all] Error 1

This is with gcc --version:
Configured with: --prefix=/Applications/Xcode.app/Contents/Developer/usr --with-gxx-include-dir=/usr/include/c++/4.2.1
Apple LLVM version 5.1 (clang-503.0.38) (based on LLVM 3.4svn)
Target: x86_64-apple-darwin12.6.0
Thread model: posix