# Elections-In-Parallel
<h2>A program to calcuate the elections of your country in a parallel way.
There is round one and round two.</h2>


Data are generated randomly in a file.txt .



To compile:
 <code>$ mpicc -o elections elections.c</code>

To run: <em>/* change the number to indicate number of processes.*/</em>
 <code>$ mpiexec -n 6 elections</code>
