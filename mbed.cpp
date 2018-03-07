#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <time.h>

#include "mbed.h"

char soln_moves[] = "UDLRFB";
uint8_t board_ids[] = {0,3,4,1,2,5};
sequence_t solve_sequence;
move_t moves[30];

// 0, ... ,5, UDLRFB
// 6, 2
// 7, '
// 8, <space>
// -1 other

int get_char_type(char a)
{
    for(int i = 0; i < 6; i++)
        if(soln_moves[i] == a)
            return i;
    if(a == '2')
        return 6;
    if(a == 39) // '
        return 7;
    if(a == ' ')
        return 8;

    printf("[ERROR] unrecognized character in solution %c (decimal %d)\n",a,a);
}

void string_to_sequence(char* string)
{
    int move = 0;

    for(int i = 0; i < strlen(string); i++)
    {
        int type = get_char_type(string[i]);

        // direction
        if(i%3 == 0)
        {
            if(type>5 || type <0)
            {
                printf("[ERROR] Could not find direction for move %d! Got %c instead.\n",move,string[i]);
                return;
            }
            moves[move].face = (face_t)type;
        }

        if(i%3 == 1)
        {
            if(type == 6)
                moves[move].n_turns = 2;
            else if(type == 7)
                moves[move].n_turns = -1;
            else if(type == 8)
                moves[move].n_turns = 1;
            else
            {
                printf("[ERROR] Not a valid move modifier for move %d: %c\n",move,string[i]);
                return;
            }
        }

        if(i%3 == 2)
        {
            if(type != 8)
            {
                printf("[ERROR] Got invalid space character for move %d: %c\n",move,string[i]);
                return;
            }
            move++;
        }
    }

    solve_sequence.n_moves = move;
    solve_sequence.moves = moves;
}

sequence_t* get_sequence()
{
    return &solve_sequence;
}

void print_sequence()
{
    int n_moves = solve_sequence.n_moves;
    printf("Sequence with %d moves\n",n_moves);
    for(int i = 0; i < n_moves; i++)
        printf("\tMove face %c %d turns.\n",soln_moves[solve_sequence.moves[i].face],solve_sequence.moves[i].n_turns);
}


int sequence_to_serial(sequence_t* seq, uint8_t* buffer, int n_bytes)
{
    printf("[sequence to serial] sequence with %d moves.\n",seq->n_moves);
    if(n_bytes < seq->n_moves)
    {
        printf("SERIAL BUFFER TOO SMALL!\n");
        return 1;
    }

    // i = 0
    buffer[0] = K_START;
    // i = 1
    buffer[1] = K_MOVES + (seq->n_moves & 0x3f);
    // i = 2 .. (n_moves+2)
    for(int i = 0; i < seq->n_moves; i++)
        buffer[i+2] = (0x7 & (seq->moves[i].n_turns)) + (0x38 & ((uint8_t)board_ids[seq->moves[i].face]<<3));
    return seq->n_moves + 2;
}

int serial_to_sequence(sequence_t* seq, uint8_t* buffer, int n_bytes)
{
    int start_index = -1;
    int n_moves = -1;
    for(int i = 0; i < n_bytes; i++)
    {
        if(buffer[i] == K_START)
        {
            start_index = i+2;
            break;
        }
    }

    if(start_index == -1)
    { 
        printf("FAILED TO FIND START OF MOVE!\n");
        return 1;
    }

    if((buffer[start_index-1] & 0xC0) != K_MOVES)
    {
        printf("COULD NOT FIND NUMBER OF MOVES!\n");
        return 1;
    }

    n_moves = buffer[start_index-1]&0x3f;
    seq->n_moves = n_moves;
    if(n_moves < 15)
    {
        printf("NUMBER OF MOVES LESS THAN 15.\n");
        return 1;
    }

    for(int i = 0; i < n_moves; i++)
    {
        uint8_t ser_move = buffer[start_index+i];
        int8_t face = (face_t)((ser_move>>3) & 0x07);
        int8_t n_turns = (int8_t)((ser_move) & 0x07);
        // 2's complement negation
        if(n_turns > 2) n_turns |= 0xf8;
        if(face < 0 || face > 5)
        {
            printf("GOT INVALID FACE.\n");
            return 1;
        }
        if(!(n_turns == -2 || n_turns == -1 || n_turns == 1 || n_turns == 2))
        {
            printf("GOT INVALID NUMBER OF TURNS: %d\n",n_turns);
            printf("Serial data: " BYTE_TO_BINARY_PATTERN "\n",BYTE_TO_BINARY(ser_move));

            return 1;
        }
        seq->moves[i].n_turns = n_turns;
        seq->moves[i].face = (face_t)face;
    }

    return 0;
}


int states[6] = {0,0,0,0,0,0};
int move_counts[6] = {0,0,0,0,0,0};
int wait_counter[6] = {0,0,0,0,0,0};

//states: 
// 0 - start command
// 1 - delay for command
// 2 - wait for and
// 3 - delay for and
// 4 - done.
void reset_mbed()
{
    for(int i = 0; i < 6; i++)
    {
        states[i] = 0;
        move_counts[i] = 0;
        wait_counter[i] = 0;
    }
}
void* run_sequence_2(void* command)
{
    mbed_info_t* cmd = (mbed_info_t*)command;
    int n_moves = cmd->seq->n_moves;
    int n_turns = 0;
    if(states[cmd->face] == 0)
    {
        cmd->set_and(0,cmd->face); // and off

        // check if done
        if(move_counts[cmd->face] > n_moves)
        {
            cmd->set_and(1,cmd->face);
            states[cmd->face] = 4;
            return NULL;
        }

        // check how many turns needed
        if(cmd->face == cmd->seq->moves[move_counts[cmd->face]].face)
        {
            n_turns = cmd->seq->moves[move_counts[cmd->face]].n_turns;
        }
        else n_turns = 0;

        // increment move counter
        move_counts[cmd->face]++;

        // rotate if needed
        if(n_turns) 
            cmd->rotate(n_turns,cmd->face);

        // wait...
        states[cmd->face] = 1;
        wait_counter[cmd->face] = 0;
        return NULL;
    }
    else if(states[cmd->face] == 1)
    {
        //printf("b%d 1\n",cmd->face);
        wait_counter[cmd->face]++;
        if(wait_counter[cmd->face] < 500) return NULL;
        states[cmd->face] = 2;
        cmd->set_and(1,cmd->face);
        return NULL;
    }
    else if(states[cmd->face] == 2)
    {
        //printf("b%d 2\n",cmd->face);
        //printf("b 1 %d\n",cmd->face);
        if(cmd->get_and())
        {
            wait_counter[cmd->face] = 0;
            states[cmd->face] = 3;
            return NULL;
        }
    }
    else if(states[cmd->face] == 3)
    {
        //printf("b%d 3\n",cmd->face);
        if(wait_counter[cmd->face]++ < 100) return NULL;
        states[cmd->face] = 0;
        cmd->set_and(0,cmd->face);
        return NULL;
    }
    else if(states[cmd->face] == 4)
        ;

    return NULL;
    //printf("b%d 4\n",cmd->face);
}


void print_sequence_2(sequence_t* seq, uint8_t* buffer)
{
    printf("Sequence of %d moves:\n",seq->n_moves);
    for(int i = 0; i < seq->n_moves; i++)
    {
        printf("    Move %d: face %d, %d turns.\n",i,(int)seq->moves[i].face,seq->moves[i].n_turns); 
        if(buffer != NULL)
            printf("    Buffer: " BYTE_TO_BINARY_PATTERN "\n",BYTE_TO_BINARY(buffer[2+i]));

    }
    uint8_t buffer_2[300];
    sequence_to_serial(seq,buffer_2,seq->n_moves);
    for(int i = 0; i < seq->n_moves; i++)
        printf("0x%hhx, ",buffer_2[i]);
    printf("\n");
}

void make_random_sequence(sequence_t* seq)
{
    int n_moves = random(15,18);
    seq->n_moves = n_moves;
    for(int i = 0; i < n_moves; i++)
    {
        seq->moves[i].face = (face_t)random(0,5);
        int n_turns = random(-2,1);
        if(n_turns==0)n_turns=2;
        seq->moves[i].n_turns = n_turns; 

        printf("random move %d: %d turns, face %d\n",i,n_turns,seq->moves[i].face);
    }
}

void reset_sequence(sequence_t* seq)
{
    seq->n_moves = 0;
    for(int i = 0; i < 30; i ++)
    {
        seq->moves[i].face = (face_t)0;
        seq->moves[i].n_turns = 0;
    }
}

int random(int min, int max)
{
    int rn = (rand() % (max + 1 - min)) + min;
    if(rn > max || rn < min)
        printf("RANDOM ERROR\n");
    return rn;
}

