#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#include "mbed.h"

int sequence_to_serial(sequence_t* seq, uint8_t* buffer, int n_bytes)
{
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
        buffer[i+2] = (0x7 & (seq->moves[i].n_turns)) + (0x38 & ((uint8_t)seq->moves[i].face<<3));
    return 0;
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



