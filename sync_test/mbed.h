#ifndef _mbed_h
#define _mbed_h

#include <stdint.h>
// special serial commands
#define K_START (0x1<<6)
#define K_MOVES (0x3<<6)
#define K_END   (0x2<<6)

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 
// cube faces
enum face_t
{
    U = 0,
    D = 1,
    L = 2,
    R = 3,
    F = 4,
    B = 5
};

// a single move, consisting of a face
// and number of CW rotations
struct move_t
{
    face_t face;
    int8_t n_turns;
};

// a sequence of moves:
// pointer to start of an array of moves
// length of the array
struct sequence_t
{
    move_t* moves;
    uint8_t n_moves;
};

// all the information that the simulated board needs to run the sequence
// for the actual implementation, keep only seq and face
// calls to small_delay, rotate, {get,set}_and should be
// replaced with calls to Ben's functions
struct mbed_info_t
{
    face_t face; // the face that this board corresponds to
    sequence_t* seq; // the sequence to follow
    void(*rotate)(int8_t, int8_t); // rotate a face n turns, second argument is board number (needed for simulation)
    void(*set_and)(int8_t, int8_t); // sets and output, second argument is board number (needed for simulation)
    int8_t(*get_and)(); // gets and board status
};

int sequence_to_serial(sequence_t* seq, uint8_t* buffer, int n_bytes);
int serial_to_sequence(sequence_t* seq, uint8_t* buffer, int n_bytes);

//void* run_sequence(void* command);
void* run_sequence_2(void* command);
void reset_mbed();
#endif
