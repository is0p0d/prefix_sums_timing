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

void print_arr(uint64_t* array, uint64_t size);
void prefix_sums_recur(uint64_t* array, uint64_t start, uint64_t end);
void prefix_sums_tree(uint64_t* array, uint64_t n);
void _upsweep(uint64_t* array, uint64_t n);
void _downsweep(uint64_t* array, uint64_t n);

int main(int argc, char* argv[])
{
    
    if (argc < 2)
    {
        printf ("ERROR: You must specify a value and a output file as arguments.\n");
        printf ("USAGE: prefix_sum_test n output.dat \n Where n is an integer problem size and output.dat is your output file.\n");
        return 1;
    }

    uint64_t maxProblemSize = atoi(argv[1]);
    char *outputFileString = argv[2];
    FILE *outputCSV = NULL;
    clock_t timer;
    uint64_t* prefixArr = NULL;
    uint64_t currProblemSize = 0;
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
    for (uint64_t probIndex = 1; probIndex <= maxProblemSize; probIndex++)
    {
        printf ("RUN: Generating dataset (2^%d) ... ", probIndex);
        uint64_t setSize = (uint64_t)(pow(2, probIndex));
        printf ("\naloc_prefixArr ptr: %p\n", (void *) &prefixArr);
        // dynamically allocate a heap for the dataset
        // malloc is used here over calloc to save clock cycles, as the array is immediately populated.
        prefixArr = (uint64_t*)malloc(setSize * sizeof(uint64_t));
        if (prefixArr == NULL)
        {
            printf ("ERROR: Could not allocate memory!");
            return 1;
        }
        for (uint64_t arrIndex = 0; arrIndex < setSize; arrIndex++)
        {
            prefixArr[arrIndex]=arrIndex+1;
        }
        printf ("\ninit_prefixArr ptr: %p\n", (void *) &prefixArr);
        printf ("done.\n");

        //print_arr(prefixArr, setSize);
        printf ("* prefix_sums_recur ... ");
        timer = clock();
        prefix_sums_recur(prefixArr,0,setSize);
        printf ("\nprcr_prefixArr ptr: %p\n", (void *) &prefixArr);
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
        printf ("\nprst_prefixArr ptr: %p\n", (void *) &prefixArr);
        timer = clock() - timer;
        treeTime = ((double)timer)/CLOCKS_PER_SEC;
        printf ("time: %f\n", treeTime);
        //print_arr(prefixArr, setSize);

        printf ("* writing data ... ");
        fprintf(outputCSV, "%d,%f,%f\n",probIndex,recurTime,treeTime);
        printf ("done.\n");

        printf("* freeing dataset ... ");
        printf ("\nfree_prefixArr ptr: %p\n", (void *) &prefixArr);
        free(prefixArr);
        printf("done.\n");
    }
    printf("RUN: Closing file ... ");
    fclose(outputCSV);
    printf("done.\n");
    return 0;
}

void print_arr(uint64_t* array, uint64_t size)
{
    for (uint64_t i = 0; i < size; i++)
        printf ("%d ", array[i]);
    printf ("\n");
}

void prefix_sums_recur(uint64_t* array, uint64_t start, uint64_t end)
{
    //printf ("** prefix_sums_recur start, end: %d, %d\n", start, end);
    if (start >= end) return;
    uint64_t mid = (start + end) / 2;
    #pragma omp parallel sections num_threads(2)
    {
        #pragma omp section
            prefix_sums_recur(array, start, mid);
        #pragma omp section
            prefix_sums_recur(array, mid+1, end);
    }

    #pragma omp parallel for
        for (uint64_t k = mid+1; k <= end; k++)
        {
            array[k] = array[k]+array[mid];
        }
}

void prefix_sums_tree(uint64_t* array, uint64_t n)
{
    //upsweep
    uint64_t power = log2(n);
    for (uint64_t d = 0; d < power-1; d++)
    {
        uint64_t power2pl = (uint64_t)(pow(2, d+1));
        uint64_t power2 = (uint64_t)(pow(2, d));
        #pragma omp parallel for
            for (uint64_t k = 0; k < n; k += power2pl)
                array[k+power2pl-1]=array[k+power2-1]+array[k+power2pl-1];
    }

    //downsweep
    array[n-1] = 0;
    for (uint64_t d = power-1; d > 0; d--)
    {
        uint64_t power2pl = (uint64_t)(pow(2, d+1));
        uint64_t power2 = (uint64_t)(pow(2, d));
        #pragma omp parallel for
            for (uint64_t k = 0; k < n; k+=power2pl)
            {
                uint64_t t = array[k+power2-1];
                array[k+power2-1] = array[k+power2pl-1];
                array[k+power2pl-1] = t+array[k+power2pl-1];
            }
    }
}

// void prefix_sums_tree(uint64_t* array, uint64_t end)
// {
//     _upsweep(array, end);
//     _downsweep(array,end);
// }
// void _upsweep(uint64_t* array, uint64_t n)
// {
//     uint64_t power = log2(n-1);
//     for (uint64_t d = 0; d < power; d++)
//     {
//         uint64_t power2pl = (uint64_t)(pow(2, d+1));
//         uint64_t power2 = (uint64_t)(pow(2, d));
//         #pragma omp parallel for
//             for (uint64_t k = 0; k < n; k += power2pl)
//                 array[k+power2pl-1]=array[k+power2-1]+array[k+power2pl-1];
//     }
// }
// void _downsweep(uint64_t* array, uint64_t n)
// {
//     uint64_t power = log2(n-1);
//     array[n-1] = 0;
//     for (uint64_t d = 0; d > power; d--)
//     {
//         uint64_t power2pl = (uint64_t)(pow(2, d+1));
//         uint64_t power2 = (uint64_t)(pow(2, d));
//         #pragma omp parallel for
//             for (uint64_t k = 0; k < n; k+=power2pl)
//             {
//                 uint64_t t = array[k+power2-1];
//                 array[k+power2-1] = array[k+power2pl-1];
//                 array[k+power2pl-1] = t+array[k+power2pl-1];
//             }
//     }
// }