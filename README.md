A bunch of 4 processes (i.e. servers with port numbers) with an internally generated random logical clock value. 

1 server acts as a client and sends a socket request to each of the 3 other servers requesting for their logical clocks. 

They respond and the client then uses Berkeley's time synchronization algorithm to synchronize a central clock value and sends to each server a separate offset value that allows each server to now have a uniform logical clock value with every other server.
