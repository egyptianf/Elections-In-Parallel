# Elections-In-Parallel
A program to calcuate the elections of your country in a parallel way.
There is round one and round two.


Data are generated randomly in a file.txt .



To compile:
 mpicc -o elections elections.c

To run: /* change the number to indicate number of processes.*/
 mpiexec -n 6 elections 