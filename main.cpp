#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "image_acquisition.h"
#include "webcam_config.h"
#include "cube.h"
#include "settings.h"
#include "serial.h"
#include "mbed.h"


char in_buffer[1024];
char filename_1[1024];
char filename_2[1024];
char* serial_name;
char default_serial[] = "/dev/ttyS0";

int im_source_type = -1;
int cv_mode = -1;
int solve_server_present = 0;
int first_camera = 0;


//display a single frame from either file or webcam
void display_frame()
{
    if(im_source_type == K_SOURCE_FILE)
    {
        get_file_image(0,filename_1);
        get_file_image(1,filename_2);
        display_single_frame();
        return;
    }
    else if(im_source_type == K_SOURCE_WEBCAM)
    {
        get_webcam_image();
        display_single_frame();
        return;
    }

    printf("No image source configured.  Run the 'source' command.\n");
}

//quick setup function
void comp_setup()
{
    printf("Webcams Selected\n");
    cv_mode = K_CV_MODE_FAST;
    im_source_type = K_SOURCE_WEBCAM;
    init_webcam(first_camera);
    init_solver();
    init_serial(serial_name);
    set_rts_pin(0);
}

void test_rts_pin()
{
    printf("Testing RTS pin...\n");
    init_serial(serial_name);

    uint8_t k = 0;
    for(;;)
    {
        usleep(1000);
        k = !k;
        set_rts_pin(k);
    }
}


//print image source (webcame or filenames) and mode (fast/slow)
void print_status()
{
    printf("STATUS:\n");
    printf("    Compiled %s on %s\n",__TIME__,__DATE__);
    if(im_source_type == K_SOURCE_FILE)
        printf("    Image Source - File: %s, %s\n",filename_1,filename_2);
    else if(im_source_type == K_SOURCE_WEBCAM)
        printf("    Image Source - Webcam\n");
    else
        printf("    No Image Source Selected!\n");

    if(cv_mode == K_CV_MODE_FAST)
        printf("    Mode - Fast\n");
    else if (cv_mode == K_CV_MODE_SETUP)
        printf("    Mode - Setup\n");
    else
        printf("    No Mode Selected\n");
}

void print_startup()
{
    printf("Rubik's Cube Solver - Main Control Program\n");
}

//print list of all commands
void print_help()
{
    printf("Commands:\n");
    //DONE!
    printf(" COMMAND   | SHORTCUT | DESCRIPTION\n");
    printf("  mode     |    m     |  set the mode (fast, debug)\n");
    printf("  status   |    s     |  print status information\n");
    printf("  help     |    h     |  print this help page\n");
    printf("  source   |    i     |  set the image source (custom file, default file, webcams)\n");
    printf("  quit     |    q     |  exit the program\n");
    printf("  frame    |   cf     |  capture a single frame\n");
    printf("  open     |   ld     |  get ready to capture from webcams\n"); 
    printf("  video    |   cv     |  open live preview of cameras\n");
    printf("  latency  |   la     |  test camera latency\n");
    printf("  get-num  |   gn     |  look up number of facelet\n");
    printf("  get-name |   gna    |  look up name of facelet\n");
    printf("  align    |    a     |  alignment sequence\n");
    printf(" live-id   |   lid    |  run identification live\n");
    printf("  config   |    c     |  configure everything for solving\n"); //stub
    printf(" solve-slow|   ss     |  solve with lots of debugging messages.\n");
    printf(" random    |    r     |  generate random serial string.\n");
    printf("           |  rts     |  test the rts pin on the serial\n");
    printf("           |   ts     |  test sequences\n");


    printf("*****NOT DONE YET*****\n");
    //TODO: Webcam
    //printf(" calibrate, ca      autocalibrate cube colors\n");
    printf(" identify,  id      identify colors on the cube\n");
    printf("    save,   sa      save frame\n");

    //TODO: Motor/serial
    printf("    motor,  m       connect to motor drivers\n"); //todo
    printf("    testm,  tm      motor test sequence\n"); //todo

    //TODO: Solve related
    printf("    connect,o       connect to java solve server\n"); //todo
    printf("    solve,  go      solve the cube\n"); //todo

    //STUB
}


int main(int argc, char** argv)
{
    if(argc > 1)
        first_camera = atoi(argv[1]);

    if(argc > 2)
        serial_name = argv[2];
    else
        serial_name = default_serial;

    

    printf("[CAMERA] First camera %d.\n",first_camera);
    printf("[Serial] name: %s\n",serial_name);
    //test_facelet_functions();

    //initializes data structure for regions of interest
    init_polys();
    print_startup();
    print_help();
    print_status();

    //parse commands
    for(;;)
    {
        printf("\n\n>>");
        scanf("%s",in_buffer);

        // exit program
        if(!strcmp("exit",in_buffer) || !strcmp("quit",in_buffer) || !strcmp("q",in_buffer))
        {
            printf("bye.\n");
            return 0;
        }


        // display single frame
        if(!strcmp("frame",in_buffer) || !strcmp("cf",in_buffer))
        {
            printf("Displaying single frame...\n");
            display_frame();
            continue;
        }

        if(!strcmp("random",in_buffer) || !strcmp("r",in_buffer))
        {
            move_t random_moves[30];
            sequence_t random_seq;
            random_seq.moves = random_moves;
            reset_sequence(&random_seq);
            make_random_sequence(&random_seq);
            print_sequence_2(&random_seq,NULL);
            sequence_t* seq = &random_seq;
            uint8_t buffer[300];
            int size = sequence_to_serial(seq,buffer,300);

            send_sequence(buffer,size);
        }

        if(!strcmp("rts",in_buffer))
        {
            test_rts_pin();
        }

        if(!strcmp("ts",in_buffer))
        {
            printf("TEST SEQUENCE\n");
            char seqs[] = "U2 D2 F2 B2 L2 R2 U  U  U  U  ";
            string_to_sequence(seqs);
            sequence_t* seq = get_sequence();
            uint8_t buffer[300];
            int size = sequence_to_serial(seq,buffer,300);

            send_sequence(buffer,size);

            printf("serial sequence:\n");
            for(int i = 0; i < size; i++)
                printf("\tcmd %d: 0x%hhx\n",i,buffer[i]);

        }

        if(!strcmp("ss",in_buffer))
        {
            printf("SLOW SOLVE\n");
            printf("cv\n");
            init_solver2(); //set up socket
            vision_solve_slow(); //preview and wait for key press
            build_solve_string(true); //convert cube to string
            if(!get_valid())
            {
                printf("invalid\n");
                continue;
            }
            char* soln = get_solution(); //send/receive from solve server
            string_to_sequence(soln); //convert to sequence type
            print_sequence(); //print sequence type
            sequence_t* seq = get_sequence();
            uint8_t buffer[300];
            int size = sequence_to_serial(seq,buffer,300);

            send_sequence(buffer,size);
            set_rts_pin(0);
            printf("serial sequence:\n");
            for(int i = 0; i < size; i++)
                printf("\tcmd %d: 0x%hhx\n",i,buffer[i]);



        }

        if(!strcmp("live-id",in_buffer) || !strcmp("lid",in_buffer))
        {
            printf("Live ID\n");
            live_id();
            continue;
        }

        // alignment sub-menu
        if(!strcmp("align",in_buffer) || !strcmp("a",in_buffer))
        {
            for(;;)
            {
                printf("Alignment type:\n");
                printf("\topen: open alignment file\n");
                printf("\tsave: save alignment to file\n");
                printf("\tedit: polygon editor\n");
                printf("\tedits: polygon still editor (last image from video mode)\n");
                printf("\tq:   go back.\n");
                printf("align>>");
                scanf("%s",in_buffer);

                // prompt for file name and open
                if(!strcmp("open",in_buffer))
                {
                    if(get_unsaved_changes())
                    {
                        printf("WARNING: unsaved changes. to throw them away use open again.\n");
                        continue;
                    }
                    printf("file name>>");
                    scanf("%s",in_buffer);
                    read_file(in_buffer);
                }

                // prompt for file name and save
                else if(!strcmp("save",in_buffer))
                {
                    printf("file name>>");
                    scanf("%s",in_buffer);
                    vomit_to_file(in_buffer);
                }

                // open alignment tool
                else if(!strcmp("all",in_buffer) || !strcmp("edit",in_buffer))
                    align_all();

                // open still alignment tool
                else if(!strcmp("edits",in_buffer))
                    align_still();

                //exit submeni
                else if(!strcmp("q",in_buffer))
                {
                    if(get_unsaved_changes())
                    {
                        printf("WARNING: unsaved changes. to throw them away use q again.\n");
                        continue;
                    }
                    break;
                }

                continue; 
            }
        }

        // convert facelet name to facelet number
        if(!strcmp("get-num",in_buffer) || !strcmp("gn",in_buffer))
        {
            printf("name_to_number>>");
            scanf("%s",in_buffer);
            int facelet = name_to_number(in_buffer);
            if(facelet == -1)
            {
                printf("Invalid name.\n");
                continue;
            }
            printf("Facelet: %d\n",facelet);
        }

        // convert facelet number to name
        if(!strcmp("get-name",in_buffer) || !strcmp("gna",in_buffer))
        {
            printf("number_to_name>>");
            scanf("%s",in_buffer);
            int facelet = atoi(in_buffer);
            printf("Facelet name: %s\n",number_to_name(facelet));
        }

        // initialize webcams
        if(!strcmp("open",in_buffer) || !strcmp("ld",in_buffer))
        {
            init_webcam(first_camera);
            continue;
        }

        // open latency test tool
        if(!strcmp("latency",in_buffer) || !strcmp("la",in_buffer))
        {
            webcam_latency();
            continue;
        }

        // display video
        if(!strcmp("video",in_buffer) || !strcmp("cv",in_buffer))
        {
            webcam_video();
            continue;
        }

        // status
        if(!strcmp("status",in_buffer) || !strcmp("s",in_buffer))
        {
            print_status();
            continue;
        }

        if(!strcmp("help",in_buffer) || !strcmp("h",in_buffer))
        {
            print_help();
            continue;
        }

        if(!strcmp("comp",in_buffer) || !strcmp("c",in_buffer))
        {
            comp_setup();
            continue;

        }

        if(!strcmp("mode",in_buffer) || !strcmp("m",in_buffer))
        {
            for(;;)
            {
                printf("Select mode: [1 - Debug, 2 - Fast]: ");
                scanf("%s",in_buffer);
                if(!strcmp("1",in_buffer))
                {
                    printf("Debuge Selected.\n");
                    cv_mode = K_CV_MODE_SETUP;
                    break;
                }
                else if(!strcmp("2",in_buffer))
                {
                    printf("GOTTA GO FAST\n");
                    cv_mode = K_CV_MODE_FAST;
                    break;
                }
                printf("Invalid mode\n");
            }
        }

        if(!strcmp("source",in_buffer) || !strcmp("i",in_buffer))
        {
            for(;;)
            {
                printf("Select source: [1 - Files, 2 - Default Files /home/jared/cube_cv/build/cube{1,2}.jpg, 3 - Webcams]: ");
                scanf("%s",in_buffer);
                if(!strcmp("1",in_buffer))
                {
                    printf("File Selected.\n");
                    im_source_type = K_SOURCE_FILE;
                    printf("Enter cube 1 image filename:\n");
                    scanf("%s",in_buffer);
                    strcpy(filename_1,in_buffer);
                    printf("Using file 1: %s\nEnter cube 2 image filename: \n",filename_1);
                    scanf("%s",in_buffer);
                    strcpy(filename_2,in_buffer);
                    printf("Using file 2: %s\n",filename_2);
                    break;
                }
                else if(!strcmp("2",in_buffer))
                {
                    printf("Default file selected\n");
                    im_source_type = K_SOURCE_FILE;
                    strcpy(filename_1,"/home/jared/cube_cv/build/cube1.jpg");
                    strcpy(filename_2,"/home/jared/cube_cv/build/cube2.jpg");
                    break;
                }
                else if(!strcmp("3",in_buffer))
                {
                    printf("Webcams Selected\n");
                    im_source_type = K_SOURCE_WEBCAM;
                    break;
                }
                printf("Invalid mode\n");
            }
        }
    }
}
