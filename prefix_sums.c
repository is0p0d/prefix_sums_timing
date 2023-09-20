/*************************************
 * Jim M                        2023 *
 * CSC-6740 - Parallel Algorithms    *
 * Due: 09.18.23                     *
 * prefix_sums.c                     *
 * A program that demonstrates the   *
 * timing differences when an algo-  *
 * rithm is paralellized             *
 *                                   *
 * Requires: stdio.h, math.h, omp.h, *
 * time.h, stdlib.h, stdint.h        *
 *************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h> //For log
#include <time.h>
#include <omp.h>

void print_arr(uint64_t* array, int size);
void prefix_sums_recur(uint64_t* array, int start, int end);
void prefix_sums_tree(uint64_t* array, int n);
void _upsweep(uint64_t* array, int n);
void _downsweep(uint64_t* array, int n);

int main(int argc, char* argv[])
{
    
    if (argc < 2)
    {
        printf ("ERROR: You must specify a value and a output file as arguments.\n");
        printf ("USAGE: prefix_sum_test n output.dat \n Where n is an integer problem size and output.dat is your output file.\n");
        return 1;
    }

    int maxProblemSize = atoi(argv[1]);
    char *outputFileString = argv[2];
    FILE *outputCSV;
    clock_t timer;
    int currProblemSize = 0;
    double recurTime = 0.0;
    double treeTime = 0.0;

    printf ("WARNING: if \"%s\" exists, it will be overwritten.\n", outputFileString);

    if ((outputCSV = fopen(outputFileString,"w")) == NULL)
    {
        printf("ERROR: Could not open or create file.\n");
        return 1;
    }

    printf ("RUN: Output file set as %s, beginning prefix scan up to %d...\n", outputFileString, maxProblemSize);
    fprintf (outputCSV, "# Intel i9-13900k, 32 cores @ 5.5GHz / 128GB RAM\n");
    fprintf (outputCSV, "# ProbSize, Recursive, Tree,\n");
    //skip 0
    for (int probIndex = 1; probIndex <= maxProblemSize; probIndex++)
    {
        printf ("RUN: Generating dataset (2^%d) ... ", probIndex);
        uint64_t setSize = (uint64_t)(pow(2, probIndex));
        // dynamically allocate a heap for the dataset
        // malloc is used here over calloc to save clock cycles, as the array is immediately populated.
        uint64_t* prefixArr= (uint64_t*)malloc(setSize * sizeof(uint64_t));
        for (uint64_t arrIndex = 0; arrIndex < setSize; arrIndex++)
        {
            prefixArr[arrIndex]=arrIndex+1;
        }
        printf ("done.\n");

        //print_arr(prefixArr, setSize);
        printf ("* prefix_sums_recur ... ");
        timer = clock();
        prefix_sums_recur(prefixArr,0,setSize);
        timer = clock() - timer;
        recurTime = ((double)timer)/CLOCKS_PER_SEC;
        printf ("time: %f\n", recurTime);
        //print_arr(prefixArr, setSize);
        
        //reset arr for next function
        printf ("* reinitializing dataset ... ");
        for (uint64_t arrIndex = 0; arrIndex < setSize; arrIndex++)
        {
            prefixArr[arrIndex]=arrIndex+1;
        }
        printf ("done.\n");
        //print_arr(prefixArr, setSize);
        printf ("* prefix_sums_tree ... ");
        timer = clock();
        prefix_sums_tree(prefixArr,setSize);
        timer = clock() - timer;
        treeTime = ((double)timer)/CLOCKS_PER_SEC;
        printf ("time: %f\n", treeTime);
        //print_arr(prefixArr, setSize);

        printf ("* writing data ... ");
        fprintf(outputCSV, "%d,%f,%f\n",probIndex,recurTime,treeTime);
        printf ("done.\n");

        printf("* freeing dataset ... ");
        free(prefixArr);
        printf("done.\n");
    }
    printf("RUN: Closing file ... ");
    fclose(outputCSV);
    printf("done.\n");
    return 0;
}

void print_arr(uint64_t* array, int size)
{
    for (int i = 0; i < size; i++)
        printf ("%d ", array[i]);
    printf ("\n");
}

void prefix_sums_recur(uint64_t* array, int start, int end)
{
    if (start >= end) return;
    int mid = (start + end) / 2;
    #pragma omp parallel sections num_threads(2)
    {
        #pragma omp section
            prefix_sums_recur(array, start, mid);
        #pragma omp section
            prefix_sums_recur(array, mid+1, end);
    }

    #pragma omp parallel for
        for (int k = mid+1; k <= end; k++)
        {
            array[k] = array[k]+array[mid];
        }
}

void prefix_sums_tree(uint64_t* array, int n)
{
    //upsweep
    int power = log2(n);
    for (int d = 0; d < power-1; d++)
    {
        int power2pl = (int)(pow(2, d+1));
        int power2 = (int)(pow(2, d));
        #pragma omp parallel for
            for (int k = 0; k < n; k += power2pl)
                array[k+power2pl-1]=array[k+power2-1]+array[k+power2pl-1];
    }

    //downsweep
    array[n-1] = 0;
    for (int d = power-1; d >= 0; d--)
    {
        int power2pl = (int)(pow(2, d+1));
        int power2 = (int)(pow(2, d));
        #pragma omp parallel for
            for (int k = 0; k < n; k+=power2pl)
            {
                int t = array[k+power2-1];
                array[k+power2-1] = array[k+power2pl-1];
                array[k+power2pl-1] = t+array[k+power2pl-1];
            }
    }
}

// void prefix_sums_tree(uint64_t* array, int end)
// {
//     _upsweep(array, end);
//     _downsweep(array,end);
// }
// void _upsweep(uint64_t* array, int n)
// {
//     int power = log2(n-1);
//     for (int d = 0; d < power; d++)
//     {
//         int power2pl = (int)(pow(2, d+1));
//         int power2 = (int)(pow(2, d));
//         #pragma omp parallel for
//             for (int k = 0; k < n; k += power2pl)
//                 array[k+power2pl-1]=array[k+power2-1]+array[k+power2pl-1];
//     }
// }
// void _downsweep(uint64_t* array, int n)
// {
//     int power = log2(n-1);
//     array[n-1] = 0;
//     for (int d = 0; d > power; d--)
//     {
//         int power2pl = (int)(pow(2, d+1));
//         int power2 = (int)(pow(2, d));
//         #pragma omp parallel for
//             for (int k = 0; k < n; k+=power2pl)
//             {
//                 int t = array[k+power2-1];
//                 array[k+power2-1] = array[k+power2pl-1];
//                 array[k+power2pl-1] = t+array[k+power2pl-1];
//             }
//     }
// }