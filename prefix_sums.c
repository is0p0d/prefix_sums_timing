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
 * time.h and prefix_func.h          *
 *************************************/
#include <stdio.h>
#include <math.h> //For log
#include <time.h>
#include <omp.h>

void print_arr(int* array, int size);
void prefix_sums_recur(int* array, int start, int end);
void prefix_sums_tree(int* array, int end);
void _upsweep(int* array, int n);
void _downsweep(int* array, int n);

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

    //skip 0
    for (int probIndex = 1; probIndex <= maxProblemSize; probIndex++)
    {
        printf ("RUN: Generating dataset (2^%d) ... ", probIndex);
        unsigned int setSize = (int)(pow(2, probIndex));
        int prefixArr1[setSize];
        int prefixArr2[setSize];
        for (int arrIndex = 0; arrIndex < setSize; arrIndex++)
        {
            prefixArr1[arrIndex]=arrIndex+1;
            prefixArr2[arrIndex]=arrIndex+1;
        }
        printf ("done.\n");

        print_arr(prefixArr1, setSize);
        printf ("* prefix_sums_recur ... ");
        timer = clock();
        prefix_sums_recur(prefixArr1,0,setSize);
        timer = clock() - timer;
        recurTime = ((double)timer)/CLOCKS_PER_SEC;
        printf ("time: %f\n", recurTime);
        print_arr(prefixArr1, setSize);

        print_arr(prefixArr2, setSize);
        printf ("* prefix_sums_tree ... ");
        timer = clock();
        prefix_sums_tree(prefixArr2,setSize);
        timer = clock() - timer;
        treeTime = ((double)timer)/CLOCKS_PER_SEC;
        printf ("time: %f\n", treeTime);
        print_arr(prefixArr2, setSize);

        printf ("* writing data ... ");
        fprintf(outputCSV, "%d,%f,%f\n",maxProblemSize,recurTime,treeTime);
        printf ("done.\n");
    }
    fclose(outputCSV);
    return 0;
}

void print_arr(int* array, int size)
{
    for (int i = 0; i < size; i++)
        printf ("%d ", array[i]);
    printf ("\n");
}

void prefix_sums_recur(int* array, int start, int end)
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

void prefix_sums_tree(int* array, int end)
{
    _upsweep(array, end);
    _downsweep(array,end);
}
void _upsweep(int* array, int n)
{
    int power = log2(n-1);
    for (int d = 0; d < power; d++)
    {
        int power2pl = (int)(pow(2, d+1));
        int power2 = (int)(pow(2, d));
        #pragma omp parallel for
            for (int k = 0; k < n; k += power2pl)
                array[k+power2pl-1]=array[k+power2-1]+array[k+power2pl-1];
    }
}
void _downsweep(int* array, int n)
{
    int power = log2(n-1);
    array[n-1] = 0;
    for (int d = 0; d > power; d--)
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