# TCP_Socket-Programming

In this assignment you will develop a pair of programs that will interact to model a game, which is described below. The game is simple, but the assignment will give you hands-on practice with creating a multi-process application, processing command line arguments, setting up and monitoring network communication channels between the processes (using TCP sockets), and reading / writing information between processes across sockets.


The game that will be modeled is called hot potato, in which there are some number of players who quickly toss a potato from one player to another until, at a random point, the game ends and the player holding the potato is “it”. The object is to not be the one holding the potato at the end of the game. In this assignment, you will create a ring of “player” processes that will pass the potato around. Thus, each player process has a left neighbor and a right neighbor. Also, there will be a “ringmaster” process that will start each game, report the results, and shut down the game.
