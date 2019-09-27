=================================================================
How to compile:
=================================================================
    -Simply type "make" into the command line and all of 
     the necessary programs will compile via the Makefile

=================================================================
How to run:
=================================================================
    -After compiling the programs you can run each of them
     by typing "./server 5000" and "./client"

    -NOTE: Make sure to run the server before the client
     or the client won't be able to establish a connection.

=================================================================
How the client works:
=================================================================
    -The client lets you type in your own commands for the 
     server. When a connection to the server has been successfully
     established, you will be greeted with a command line.
     From there you can type in commands such as "SYS ls"
     and "SYS who" and recieve a response from the server.
     You can quit anytime by typing "quit" or "q".
     
    -NOTE: All commands must be prefaced with "SYS" in order 
     to be recognized by the server.

     