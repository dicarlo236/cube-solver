#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "webcam_config.h"

int camera = -1;

const char cmd_prefix[]  = "v4l2-ctl -d /dev/video";
const char ctrl_prefix[] = " --set-ctrl=";
const char fps_prefix[] = " -p ";


void parse_line(char* line, int first_camera)
{
    char* pch = strtok(line," ");

    // don't do anything for a comment (starts with /)
    if(pch[0] == '/' || strlen(pch) < 3)
        return;

    //set camera to modify
    if(!strcmp("CAMERA",pch))
    {
        pch = strtok(NULL," ");
        // camera 0,1, or invalid
        if(pch[0] == '0') camera = first_camera;
        else if(pch[0] == '1') camera = first_camera + 1;
        else printf("Invalid camera number %s\n",pch);
        printf("Now modifying camera %d\n",camera);
        return;
    }

    // if camera was never set or somehow became invalid
    if(camera < 0)
    {
        printf("[Webcam Config] Error! Configuration file does not specify camera.\n");
        return;
    }


    //set fps command
    if(!strcmp("FPS",pch))
    {
       pch = strtok(NULL," ");
       char command[500];  
       //fps has a different prefix, -p
       sprintf(command,"%s%d%s%s\n",cmd_prefix,camera,fps_prefix,pch);
       //execute command
       system(command);
       return;
    }

    //otherwise its a command,value pair
    //get value
    char* value = strtok(NULL," ");

    char command[500];
    sprintf(command,"%s%d%s%s=%s\n",cmd_prefix,camera,ctrl_prefix,pch,value);
    printf("RUNNING COMMAND: %s",command);
    system(command);
}

// load configuration file
void load_config_file(int first_camera)
{
    FILE* file = fopen("../webcam.cfg","r");
    char line[500];

    // parse lines
    while(fgets(line,sizeof(line),file))
        parse_line(line,first_camera);

    fclose(file);
}

