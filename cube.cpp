#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "SolverConnection.h"
#include "cube.h"
#include "mbed.h"


char sides[] = "urfdlb"; //face names, lowercase, in order
char usides[] = "URFDLB";  //face names, uppercase, in order
char f_name[] = "  "; //output face name
char solve_string[9*6 + 1]; // output solve string
char color_names[6][10] = {"blue","red","white","green","orange","yellow"};

char ref_solve_string[] = "FBRUUFRRLFDDURBBRFFFDFFFLDLDBURDDRRDLLULLLBUBBLUUBDRBU";
char ref_solve_string2[] = "FLBRUBFBDBRUDRRDUDUULBFFBDRDRFLDFBLFLFRBLDLDRRFUUBLLUU";

SolverConnection solver;

bool valid_cube = false;
struct timespec stopa, starta;
char* solution;

bool get_valid()
{
    return valid_cube;
}

void build_solve_string(bool verbose)
{
    valid_cube = true;
    printf("Build Solve String\n");
    int* f = get_facelet_codes();
    f[4] = 0;
    f[13] = 1;
    f[22] = 2;
    f[31] = 3;
    f[40] = 4;
    f[49] = 5;
    if(verbose)
    {
        //printf("\tchecking configuration.\n");
        int sums[6] = {0,0,0,0,0,0};
        for(int i = 0; i < 54; i++)
        {
            if(f[i] < 0 || f[i] > 5)
            {
                printf("[ERROR] facelet %d (%s) has invalid code %d\n",i,number_to_name(i),f[i]);
                valid_cube = false;
            }
            else
                sums[f[i]]++;
        }

       for(int i = 0; i < 6; i++)
       {
          if(sums[i] != 9)
          {
              printf("[ERROR] found %d of color %d (%s)\n",sums[i],i,color_names[i]);
              valid_cube = false;
          }
       }
    }

    if(valid_cube)
    {
        for(int i = 0; i < 54; i++)
            solve_string[i] = usides[f[i]];
        solve_string[54] = 0;
        printf("\tsolve string: %s\n",solve_string);
    }
    else
    {
        printf("\tgot invalid cube, using reference string: %s\n",ref_solve_string);
        strcpy(solve_string,ref_solve_string);
    }
}

void init_solver()
{
    solver.initConnection(9090);
    //char* soln = solver.solve(ref_solve_string2);
    //printf("Test solve solution: %s\n",soln);
    //string_to_sequence(soln);
    //print_sequence();
}

void init_solver2()
{
    solver.initConnection(9090);
}

void start_solve_timer()
{
    clock_gettime(CLOCK_MONOTONIC_RAW,&starta);
}

char* get_solution()
{
    solution = solver.solve(solve_string);
    clock_gettime(CLOCK_MONOTONIC_RAW,&stopa);
    uint64_t us = (stopa.tv_sec - starta.tv_sec) * 1000000 + (stopa.tv_nsec - starta.tv_nsec) / 1000;
    printf("time: %f ms\n",((float)us)/1000.f);
    printf("solution string: %s\n",solution);
    return solution;
}

// convert facelet name to number
int name_to_number(char* name)
{
    // check length
    int len = strlen(name);
    if(len != 2)
    {
        printf("name_to_number: length is %d, expected 2.\n",len);
        return -1;
    }

    // coordinate 1: side
    int c1 = -1;

    for(int i = 0; i < 6; i++)
        if((name[0]|0x20) == sides[i]) //the 0x20 sets the lowercase bit
            c1 = i;

    if(c1 == -1)
    {
        printf("name_to_number: first character was %c/%c.  unrecognized.\n",name[0],name[0]|0x20);
        return -1;
    }

    // coordinate 2: facelet
    int c2 = atoi(name+1);
    if(c2 < 0 || c2 > 8)
    {
        printf("name_to_number: second character was %c. unrecognized.\n",name[1]);
        return -1;
    }

    return c2 + c1*9;
}


char* number_to_name(int facelet)
{
    // check if in range
    if(facelet < 0 || facelet > 53)
    {
        f_name[0] = ')';
        f_name[1] = ';';
        return f_name;
    }

    // c2 is remainder, c1 is quotient
    int c2 = facelet;
    while(c2 > 8) c2-=9;
    int c1 = (facelet - c2)/9;

    f_name[0] = usides[c1];
    f_name[1] = c2 + 0x30;
    return f_name;
}

void test_facelet_functions()
{
    for(int i = 0; i < 54; i++)
    {
        number_to_name(i);
        int r = name_to_number(f_name);
        if(r != i)
            printf("FAIL i = %d\n",i);
    }
}
