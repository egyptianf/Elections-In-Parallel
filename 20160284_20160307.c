#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"


//This to check that the user inputs a valid candidate
_Bool valid_candidate(int candidates_num, int candidate)
{
    return (candidate <= candidates_num) && (candidate >= 1);
}
_Bool repeated(int* list, int size, int element)
{
    _Bool repeat=0;
    int i;
    for(i=0; i<size; i++)
    {
        if(element == list[i])
        {
            repeat = 1;
            break;
        }
    }
    return repeat;
}
/*pref_list ≡ preferences list*/
int* pref_list_input(int rows, int cols)
{
    int i,j;
    int *candidates_preferences = malloc( rows * cols * sizeof(int));
    for(i=0; i<rows; i++)
        {
            printf("Please enter the preferece list for voter %u\n", i+1);
            for(j=0; j< cols; j++)
            {
                scanf("%u", &candidates_preferences[(i * cols) + j]);
                int *my_array = candidates_preferences+((i*cols)*sizeof(int));
                if(! valid_candidate(cols, candidates_preferences[(i * cols) + j])){
                    printf("INVALID Cadidate number, please reenter this candidate number.\n");
                    j--;
                }
            }
        }
    return candidates_preferences;
}
void print_pref_list(int *matrix, int rows, int cols)
{
    int i,j;
    for(i=0; i< rows; i++){
        for(j=0; j<cols; j++)
            printf("%u ", matrix[(i * cols) + j]);
        printf("\n");
    }
}

double percent(double fraction){return fraction * 100;}
void print_percentages(double *frequencies, int size, int voters)
{
    int i;
    double percentage;
    for(i=0; i<size; i++)
    {
        percentage = (double)frequencies[i]/(double)voters;
        printf("Candidate [%u] got %.2lf%%\n", i+1, percent(percentage));
    }

}

double* round1(int *sub_candidates_pref, int candidates, int portion_voters)
{
    double *local_frequencies = malloc(candidates * sizeof(double));
    memset(local_frequencies, 0, candidates*sizeof(double));
    int i;
    for(i = 0; i<portion_voters; i++)
    {
        int candidate = sub_candidates_pref[(i * candidates) + 0];
        local_frequencies[candidate-1]+= 1.0;
    }
    return local_frequencies;
}

double* round2(int *sub_candidates_pref, _Bool *marking_array, int candidates, int portion_voters)
{
   
    double *local_frequenceies = malloc (candidates * sizeof(double));
    memset(local_frequenceies, 0, candidates*sizeof(double));
    int i,j;
    for(i=0; i<portion_voters; i++)
    {
        for(j=0; j<candidates;j++)
        {
            int candidate = sub_candidates_pref[(i*candidates) + j];
            if(marking_array[candidate-1] == 1)
            {
                local_frequenceies[candidate-1] +=1.0;
                break;
            }
        }
    }
    return local_frequenceies;

}






/* Get the index of a maximum element in an array */
int max(double *my_array, int size)
{
    int i, max=0;
    double max_index = 0.0;
    for(i=0; i<size; i++)
    {
        if(my_array[i]> max)
        {
            max = my_array[i];
            max_index = i;
        }
    }
    return max_index;
}

//logical error in here
_Bool* get_candidates_equalTo_max(double* global_frequencies, int size)
{
    _Bool *marking_array = malloc( size * sizeof(_Bool));
    memset(marking_array, 0, size * sizeof(_Bool));
    int max_index = max(global_frequencies, size);
    marking_array[max_index] =1;
    int i;
    for(i=0; i<size; i++)
    {
        if(global_frequencies[i] == global_frequencies[max_index])
            marking_array[i] = 1;
    }
    return marking_array;
}

_Bool more_than_one_winner(_Bool* marking_array, int size)
{
    int i, count=0;
    for(i=0; i<size; i++)
    {
        if(marking_array[i]==1)
            count++;
        if(count > 1)
            return 1;
    }
    return 0;
}
_Bool round2_winner(double *global_frequencies, int size)
{
    int max_index = max(global_frequencies, size);
    int i;
    for(i=0; i<size; i++)
    {
        if(global_frequencies[i] == global_frequencies[max_index])
            return 0;
    }
    return 1;
}
int* generate_random_data(int candidates, int portion_voters)
{
    int *sub_candidates_pref;
    int sub_candidates_pref_sz = portion_voters * candidates;
    sub_candidates_pref = malloc(sub_candidates_pref_sz * sizeof(int) );
    int i,j;
    for(i=0; i<portion_voters; i++)
        for(j=0; j<candidates; j++)
            sub_candidates_pref[(i* candidates) + j] = (rand()%candidates) +1;
    return sub_candidates_pref;
}

//Will be called by all processes
void generate_datafile(MPI_File fh, int candidates, int portion_voters, int my_rank)
{
    /* Assume the file is open */
    int sub_candidates_pref_sz = portion_voters * candidates;
    int *sub_candidates_pref;
    sub_candidates_pref = generate_random_data(candidates, portion_voters);

    //Now write this part at the specifies postion of the file
    MPI_Offset offset = 8 + (my_rank*sub_candidates_pref_sz);
    MPI_File_write_at_all(fh, offset, sub_candidates_pref, sub_candidates_pref_sz, MPI_INT, MPI_STATUS_IGNORE);
}

//Will be called by all processes
int *get_preferences_from_file(MPI_File fh, int portion_voters, int candidates, int my_rank)
{
    int *sub_candidates_pref;
    int sub_candidates_pref_sz = portion_voters * candidates;
    sub_candidates_pref = malloc(sub_candidates_pref_sz * sizeof(int) );
    MPI_Offset offset = 8 + (sub_candidates_pref_sz * my_rank);

    MPI_File_read_at_all(fh, offset, sub_candidates_pref, sub_candidates_pref_sz, MPI_INT, MPI_STATUS_IGNORE);

    return sub_candidates_pref;
}


int main(int argc, char** argv)
{
    srand(time(NULL));
    int comm_sz, my_rank;
    MPI_Comm my_comm = MPI_COMM_WORLD;
    MPI_Datatype my_type = MPI_INT;
    MPI_File fh;


    char choice;
    int candidates, voters;
    int *candidates_preferences, *sub_candidates_pref;
    int portion_voters, rem_voters;

    int i,j;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(my_comm, &comm_sz);
    MPI_Comm_rank(my_comm, &my_rank);
    MPI_File_open(my_comm, "file.txt", MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);



    if(my_rank == 0)
    {
        printf("Write 1 if you want input from user, 0 if you want it from file.\n");
        scanf("%c", &choice);
        if(choice == '1'){
            printf("Please enter the no. candidates: \n");
            scanf("%u", &candidates);
            printf("Please enter the no. voters: \n");
            scanf("%u", &voters);
            printf("\n");
            printf("You've entered %u candidates and %u voters.\n", candidates, voters);

            candidates_preferences = pref_list_input(voters, candidates);
            printf("You've entered: \n");
            print_pref_list(candidates_preferences, voters, candidates);
        
            portion_voters = voters / comm_sz;
            rem_voters = voters % comm_sz;
        }
        else if(choice == '0')
        {
            /*code for input from file*/
            MPI_Offset size;
            MPI_File_get_size(fh, &size);
            MPI_File_read_at(fh, 0, &candidates, 1, my_type, MPI_STATUS_IGNORE);
            printf("We have read %u candidates from file.\n", candidates);
            MPI_File_read_at(fh, 4, &voters, 1, my_type, MPI_STATUS_IGNORE);
            printf("We have read %u voters from file.\n", voters);

            portion_voters = voters / comm_sz;
            rem_voters = voters % comm_sz;
            //MPI_File_close(&fh);
            //MPI_Finalize();
            //return 0;
        }
        
    }

    MPI_Bcast(&portion_voters, 1, my_type, 0, my_comm);
    MPI_Bcast(&candidates, 1, my_type, 0, my_comm);

    int sub_candidates_pref_sz = portion_voters * candidates;
    sub_candidates_pref = malloc(sub_candidates_pref_sz * sizeof(int) );
    //I can make every process read its part from the file at this part here
    //generate_datafile(fh, candidates, portion_voters, my_rank);

    sub_candidates_pref = get_preferences_from_file(fh, portion_voters, candidates, my_rank);
    //Start of round 1
    //Scatter the candidates_preferences matrices into smaller matrices for every process 
    //(not used if read from file)
    //MPI_Scatter(candidates_preferences, sub_candidates_pref_sz, my_type, sub_candidates_pref, sub_candidates_pref_sz, my_type, 0, my_comm);

    // Get from each process its local frequencies of each candidate 
    double* local_frequencies;
    local_frequencies = round1(sub_candidates_pref, candidates, portion_voters);

    /*Remainder part
    if(my_rank == 0)
    {

    }
    */
   
    double *global_frequencies;
    if(my_rank == 0)
        global_frequencies = malloc(candidates * sizeof(double));

    //SUM all corresponding frequencies and put the sum in global_frequencies array 
    MPI_Reduce(local_frequencies, global_frequencies, candidates, MPI_DOUBLE, MPI_SUM, 0, my_comm);


    //Broadcasting the marking array that specifies which candidates will get into round 2 
    _Bool *marking_array;
    if(my_rank == 0)
        marking_array = get_candidates_equalTo_max(global_frequencies, candidates);
    else
    {
        marking_array = malloc(candidates * sizeof(_Bool));
    }
    MPI_Bcast(marking_array, candidates, MPI_C_BOOL, 0, my_comm);
    


    //Here we have all the frequencies 
    if(my_rank == 0)
    {
        printf("Start of round 1...\n");
        print_percentages(global_frequencies, candidates, voters);
        printf("\n");
       
        if(! more_than_one_winner(marking_array, candidates)){
            printf("Winner is candidate %d in round 1.\n", max(global_frequencies, candidates)+1);
            MPI_File_close(&fh);
            MPI_Finalize();
            return 0;
        }
    }

    //Finish of round 1
    
    //Round 2
    if(my_rank == 0)
        printf("Start of round 2...\n");
    double *round2_local_frequencies;
   
    round2_local_frequencies = round2(sub_candidates_pref, marking_array, candidates, portion_voters);
    
    
    double *round2_global_frequencies;
    if(my_rank==0)
        round2_global_frequencies = malloc(candidates * sizeof(double));
    
    MPI_Reduce(round2_local_frequencies, round2_global_frequencies, candidates, MPI_DOUBLE, MPI_SUM, 0, my_comm);


    
    if(my_rank == 0)
    {
        print_percentages(round2_global_frequencies, candidates, voters);
        printf("\n");
        printf("Winner is candidate %d in round 2.\n", max(round2_global_frequencies, candidates)+1);
        
        
    }
    

    MPI_File_close(&fh);
    MPI_Finalize();
    return 0;
}