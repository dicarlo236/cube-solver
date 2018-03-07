#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "mbed.h"

// this file contains all the code used to generate and run the tests
move_t  test_moves[30];
move_t  serial_moves[30];
move_t  recorded_moves[100];

// timestamps for simulated moves
int64_t recorded_moves_ts[100];

// and board status
int8_t and_array[6];

int n_recorded_moves = 0;

sequence_t serial_seq;
sequence_t test_seq;

mbed_info_t mbed_commands[6];


uint8_t serial_data[800];
int fake_time = 0;

// generate random number between min and max, inclusive
// not actually an even distribution.
int random(int min, int max)
{
    int rn = (rand() % (max + 1 - min)) + min;
    if(rn > max || rn < min)
        printf("RANDOM ERROR\n");
    return rn;
}

//*******************************************************
//  SIMULATED FUNCTIONS
//*******************************************************


// rotate face by n_turns
// returns after 10-14 ms
void rotate_f(int8_t n_turns, int8_t face)
{
    //usleep(10000 + random(1,3000));
    // record motor rotation.
    if(n_recorded_moves >= 100)
        printf("TOO MANY MOVES!\n");
    recorded_moves[n_recorded_moves].n_turns = n_turns;
    recorded_moves[n_recorded_moves].face = (face_t)face;
    recorded_moves_ts[n_recorded_moves] = fake_time;
    n_recorded_moves++;
}

// sets the and output for an mbed
void set_and_f(int8_t a, int8_t face)
{
    and_array[face] = a;
}


// reads the and input for an mbed
int8_t get_and_f()
{
    int8_t a;
    a = and_array[0] && and_array[1] && and_array[2] && and_array[3] && and_array[4] && and_array[5];
    return a;
}



// sets up the mbed_command structs
void initialize_mbed_commands(mbed_info_t* cmd, sequence_t* seq, int8_t* and_array, int n_cmds)
{
    n_recorded_moves = 0;
    for(int i = 0; i < n_cmds; i++)
    {
        mbed_commands[i].face = (face_t)i;
        mbed_commands[i].rotate = rotate_f;
        mbed_commands[i].get_and = get_and_f;
        mbed_commands[i].set_and = set_and_f;
        mbed_commands[i].seq = seq;
        and_array[i] = 0;
    }
}

// resets all data in a sequence
void reset_sequence(sequence_t* seq)
{
    seq->n_moves = 0;
    for(int i = 0; i < 30; i ++)
    {
        seq->moves[i].face = (face_t)0;
        seq->moves[i].n_turns = 0;
    }
}

// resets all data in a serial buffer
void reset_serial(uint8_t* b, int n_bytes)
{
    for(int i = 0; i < n_bytes; i++)
        b[i] = 0;
}

// print a sequence for debugging
void print_sequence(sequence_t* seq, uint8_t* buffer)
{
    printf("Sequence of %d moves:\n",seq->n_moves);
    for(int i = 0; i < seq->n_moves; i++)
    {
        printf("    Move %d: face %d, %d turns.\n",i,(int)seq->moves[i].face,seq->moves[i].n_turns); 
        if(buffer != NULL)
            printf("    Buffer: " BYTE_TO_BINARY_PATTERN "\n",BYTE_TO_BINARY(buffer[2+i]));

    }
}



// fill *seq with a random valid sequence
void make_random_sequence(sequence_t* seq)
{
    int n_moves = random(20,26);
    seq->n_moves = n_moves;
    for(int i = 0; i < n_moves; i++)
    {
        seq->moves[i].face = (face_t)random(0,5);
        int n_turns = random(-2,1);
        if(n_turns==0)n_turns=2;
        seq->moves[i].n_turns = n_turns; 
    }
}

void make_random_sequence_2(sequence_t* seq)
{
    int n_moves = random(20,26);
    seq->n_moves = n_moves;
    for(int i = 0; i < n_moves; i++)
    {
        seq->moves[i].face = (face_t)random(0,1);
        int n_turns = random(-2,1);
        if(n_turns==0)n_turns=2;
        seq->moves[i].n_turns = n_turns; 
    }
}

// compare two sequences for equality
int compare_sequences(sequence_t* s1, sequence_t* s2)
{
    if(s1->n_moves != s2->n_moves)
    {
        printf("sequence compare error: number of moves mismatch.\n");
        return 1;
    }
    for(int i = 0; i < s1->n_moves; i++)
    {
        if(s1->moves[i].face != s2->moves[i].face)
        {
            printf("sequence compare error: move %d face mismatch %d, %d.\n",i,s1->moves[i].face,s2->moves[i].face);
            return 1;
        }
        if(s1->moves[i].n_turns != s2->moves[i].n_turns)
        {
            printf("sequence compare error: move %d face mismatch %d, %d\n",i,s1->moves[i].n_turns, s2->moves[i].n_turns);
            return 1;
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int n_serial_tests = 1000000;
    int fail = 0;
    // set up the test sequences
    test_seq.moves = test_moves;
    serial_seq.moves = serial_moves;
    srand(time(NULL));
    int noprint = argc>1;


    printf("Cube Solver.\n");    


    // run a million serial tests
    for(int i = 0; i < n_serial_tests; i++)
    {
        if(i%(n_serial_tests/10)==0&&!noprint)
            printf("finished %d of %d serial tests.\n",i,n_serial_tests);
        reset_sequence(&test_seq);
        reset_sequence(&serial_seq);
        reset_serial(serial_data,800);

        make_random_sequence(&test_seq);
        fail |= sequence_to_serial(&test_seq,serial_data,800); // nucleo function
        fail |= serial_to_sequence(&serial_seq,serial_data,800); // nucleo function
        fail |= compare_sequences(&test_seq,&serial_seq);
        if(fail)
        {
            printf("ERROR!\n");
            print_sequence(&test_seq,serial_data);
            printf("was decoded to:\n");
            print_sequence(&serial_seq,NULL);
            break;
        }

    }
    if(fail)
        printf("FAILED.\n");
    else
        printf("serial passed.\n");

    // test sequences
    fail = 0;
    int n_seq_tests = 1000;
    for(int t = 0; t < n_seq_tests; t++)
    {
        // reset things at the beginning of the test
        reset_mbed();
        fake_time = 0;
        if(t%(n_seq_tests/10)==0&&!noprint)
            printf("finished %d of %d sequence tests.\n",t,n_seq_tests);
        reset_sequence(&test_seq);
        make_random_sequence(&test_seq);
        initialize_mbed_commands(mbed_commands, &test_seq, and_array, 6);

        // randomly call mbed functions.
        for(int iter = 0; iter < 100000; iter++)
        {
            int s = random(0,5);
            fake_time++;
            run_sequence_2((void*)(&mbed_commands[s]));
        }

        // check if the correct number of moves were executed
        if(n_recorded_moves != test_seq.n_moves)
        {
            printf("exptected %d moves, got %d.\n",test_seq.n_moves, n_recorded_moves);
            fail= 1;
        }

        int64_t start_time = recorded_moves_ts[0];
        for(int i = 0; i < n_recorded_moves; i++)
        {
            if(recorded_moves[i].face != test_seq.moves[i].face)
            {
                printf("Faces wrong.\n");
                fail = 1;
            }
            if(recorded_moves[i].n_turns != test_seq.moves[i].n_turns)
            {
                printf("Number of turns wrong.\n");
                fail = 1;
            }
            int dt = i > 0?recorded_moves_ts[i] - recorded_moves_ts[i-1]:12000;
            if((dt > 6000 || dt < 3000)&&dt != 12000)
            {

                printf("Timing wrong: %d\n",dt);
                fail = 1;
            }
        }
    }
    if(fail)
        printf("failed timing tests.\n");
    else
        printf("passed timing tests.\n");

    return fail;




}
