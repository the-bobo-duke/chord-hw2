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
[error]

