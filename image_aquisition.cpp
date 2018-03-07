#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#include "image_acquisition.h"
#include "settings.h"
#include "webcam_config.h"
#include "serial.h"
#include "cube.h"

using namespace cv;

int webcam_init = 0;
char align_done = 0;
int current_facelet = 0;
int current_poly = 0;

//int ref_color[18]; // reference colors

// colors used in the UI for cube colors
int default_color[18] = {255,0,0, //blue
                         0,0,255, //red
                         255,255,255, //white
                         0,255,0, //green
                         0,128,255, //orange
                         0,255,255}; //eyllow

// id'd facelets
int facelet_codes[54];

// reference color scalars
Scalar ref_scalars[6];

Scalar default_scalars[6];

float ref_magnitues[6];


Mat image1, image2;

Mat* ims[] = {&image1, &image2};
Mat wide_image, wide_image_converted;
VideoCapture cap1,cap2;

VideoCapture* caps[] = {&cap1,&cap2};

cube_rois cube_polys, file_cube_polys;

Scalar facelet_colors[54];


struct timespec stop, start;

int unsave_changes = 0;

int get_unsaved_changes()
{
    int rv = unsave_changes;
    unsave_changes = 0;
    return rv;
}
// write polygons to file
void vomit_to_file(char* name)
{
    unsave_changes = 0;
    printf("[File Write]\n");
    printf("\tname: %s\n",name);
    printf("\tsize: %ld bytes\n",sizeof(cube_rois));
    //copy into separate structure so we can strip out pointers
    memcpy(&file_cube_polys,&cube_polys,sizeof(cube_rois));
    kill_pointers();

    //write to file in binary mode
    FILE* fptr = fopen(name,"wb");
    fwrite(&file_cube_polys,sizeof(file_cube_polys),1,fptr);
    fclose(fptr);
    printf("\tdone!\n");
}

// read polygons from file
void read_file(char* name)
{
    unsave_changes = 0;
    printf("[File Read]\n");
    printf("\tname: %s\n",name);
    printf("\tsize: %ld bytes\n",sizeof(cube_rois));

    //read
    FILE* fptr = fopen(name,"rb");
    fread(&cube_polys,sizeof(cube_polys),1,fptr);
    fclose(fptr);     

    //create opencv point objects
    create_opencv_points();
}

// initialize webcams
void init_webcam(int i)
{
    //memcpy(ref_color,default_color,18*sizeof(int));
    int index = 0;
    for(int image = i; image < i+2; image++)
    {
        printf("[Webcam] attempting to open webcam %d.\n",image);
        if(! (caps[index])->open(image))
        {
            printf("[Error] Failed to open webcam %d\n",image);
            return;
        }
        // set resolution
        caps[index]->set(CV_CAP_PROP_FRAME_WIDTH,320);
        caps[index]->set(CV_CAP_PROP_FRAME_HEIGHT,240);
        printf("[Webcam] Opened camera %d\n",image);
        index++;
    }
    printf("[Webcam] Configuring cameras...\n");
    // load and execute config file
    load_config_file(i);
    webcam_init+=2;
    // get one image to force the driver to start talking to the cameras
    get_webcam_image();
}

// webcam latency tool
void webcam_latency()
{
    Mat red(120,100,CV_8UC3,Scalar(255,0,0));
    Mat blu(120,100,CV_8UC3,Scalar(0,0,255));
    Mat wc;
    int counter = 0;
    int has_printed = 0;
    for(;;)
    {
        counter++;
        int is_r = 0;
        if(counter < 50)
        {
            is_r = 1;
            imshow("target",red);
            has_printed = 0;
        }
        else if(counter < 100)
            imshow("target",blu);
        else 
            counter = 0;

        if(counter == 50)
            clock_gettime(CLOCK_MONOTONIC_RAW,&start);

        cap1 >> wc;
        Scalar avg = mean(wc);
        int see_r = (avg[0] > avg[2]);

        if( (counter > 50) && (see_r == is_r) && !has_printed )
        {
            has_printed = 1;
            clock_gettime(CLOCK_MONOTONIC_RAW,&stop);
            uint64_t us = (stop.tv_sec - start.tv_sec) * 1000000 + (stop.tv_nsec - start.tv_nsec) / 1000;
            printf("time: %ld\n",us);
        }

        waitKey(1);
    }

}

// webcam video display
void webcam_video()
{
    printf("[Webcam] Opening video stream... Press any key to stop.\n");
    int f_count = 0;
    uint64_t t_count = 0; 
    int n_frame_avg = 20;
    float fps = 0.f;
    float msf = 0.f;
    for(;;)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW,&start);
        f_count++;

        *(caps[0]) >> *(ims[0]);
        *(caps[1]) >> *(ims[1]);
        hconcat(*(ims[0]),*(ims[1]),wide_image); //combine cameras

        char overlay[200];
        sprintf(overlay,"FPS: %.2f, ms/frame: %.2f",fps,msf);
        putText(wide_image,overlay,cvPoint(30,30),FONT_HERSHEY_SIMPLEX,0.8,cvScalar(0,255,0),1,CV_AA);

        imshow("webcam stream",wide_image);
        if(waitKey(1) > 0) break;
        clock_gettime(CLOCK_MONOTONIC_RAW,&stop);
        t_count += (stop.tv_sec - start.tv_sec) * 1000000 + (stop.tv_nsec - start.tv_nsec) / 1000;
        if(f_count > 20)
        {
            //printf("FPS: %ld, ms/frame: %ld.\n",10000000/t_count,t_count/(10*1000));
            fps = 20000000.f/((float)t_count);
            msf = ((float)t_count)/(20000.f);
            t_count = 0;
            f_count = 0;
        }

    }
    destroyAllWindows();
    printf("Done!\n");
}


void compute_colors(bool fill_poly)
{
    // colorspace conversion first!
    cvtColor(wide_image,wide_image_converted,CV_BGR2Lab);
    imshow("converted",wide_image_converted);
    // set reference scalars
    for(int i = 0; i < 6; i++)
    {
        int base = (i-0)*3;
        ref_scalars[i] = Scalar(cube_polys.ref_color[base],cube_polys.ref_color[base+1],cube_polys.ref_color[base+2]);
        default_scalars[i] = Scalar(default_color[base],default_color[base+1],default_color[base+2]);
        //ref_magnitues[i] = sqrtf(ref_color[base]*ref_color[base] + ref_color[base+1]*ref_color[base+1] + 
                               //ref_color[base+2]*ref_color[base+2]);

        //printf("ref %d: %d %d %d\n",i,ref_color[base],ref_color[base+1],ref_color[base+2]);
        //printf("def %d: %d %d %d\n",i,default_color[base],default_color[base+1],default_color[base+2]);
    }

    cv::Mat1b mask(wide_image.rows, wide_image.cols, uchar(0));
    for(int i = 0; i < 54; i++)
    {
        facelet_rois* fl = &cube_polys.rois[i];

        if(fl->n_rois < 1)
        {
            //printf("[Warning] Facelet %d (%s) has no polygons!\n",i,number_to_name(i));
            continue;
        }
        Scalar facelet_color(0,0,0);
        int n_rois = fl->n_rois;

        for(int j = 0; j < fl->n_rois; j++)
        {
            mask.setTo(Scalar(0));
            const cv::Point* pts_1[1] = {&fl->rois[j].opencv_pts[0]};
            fillPoly(mask,pts_1,&(fl->rois[j].n_pts),1,cv::Scalar(255));

            Scalar mean_color = cv::mean(wide_image_converted,mask);

            facelet_color += mean_color / n_rois;

           // if(fill_poly)
                //fillPoly(wide_image,pts_1,&(fl->rois[j].n_pts),1,mean_color);

        } // end polygon loop
        facelet_colors[i] = facelet_color;

        float facelet_magnitude = norm(facelet_color);

        // find best match (current rgb dot product)
        float best_match = 255*100;
        facelet_codes[i] = -1;
        for(int j = 0; j < 6; j++)
        {
            //float current_match = (float)(ref_scalars[j].dot(facelet_color))/(facelet_magnitude * ref_magnitues[j]);
            float current_match = fabs(ref_scalars[j](0) - facelet_color(0)) + 
                                  fabs(ref_scalars[j](1) - facelet_color(1)) + 
                                  fabs(ref_scalars[j](2) - facelet_color(2));
            if(current_match > best_match)
                continue;

            best_match = current_match;
            facelet_codes[i] = j;
        }
        //printf("best match: %f\n",best_match);
        //if(i == 0)
            //printf("bgr: %f %f %f\n",facelet_color(0),facelet_color(1),facelet_color(2));
        //printf("facelet code: %d\n",facelet_codes[i]);
        //std::cout<<default_scalars[facelet_codes[i]]<<"\n";

        // draw outline
        if(fill_poly)
        {
            for(int j = 0; j < fl->n_rois; j++)
            {
                const cv::Point* pts_1[1] = {&fl->rois[j].opencv_pts[0]};
                polylines(wide_image,pts_1,&(fl->rois[j].n_pts),1,1,default_scalars[facelet_codes[i]],2);
            }
        }


    } //end facelet loop
}

int* get_facelet_codes()
{
    return facelet_codes;
}

void null_func(int,void*)
{
    ;
}


void live_id()
{
    int f_count = 0;
    uint64_t t_count = 0; 
    int n_frame_avg = 20;
    float fps = 0.f;
    float msf = 0.f;
    printf("[Webcam] Opening video stream...\n");

    namedWindow("webcam stream");
    char color_names[6][10] = {"blue","red","white","green","orange","yellow"};
    char channel_names[3][2] = {"a","b","c"};

    for(int color = 0; color < 6; color++)
    {
        namedWindow(color_names[color],CV_WINDOW_NORMAL);
        for(int channel = 0; channel < 3; channel++)
            createTrackbar(channel_names[channel],color_names[color],cube_polys.ref_color + color*3 + channel,255,null_func);
        createTrackbar(color_names[color],color_names[color],NULL,255,null_func);
        resizeWindow(color_names[color],200,100);
        moveWindow(color_names[color],220*color,0);
    }


   // createTrackbar("blu a","webcam stream",cube_polys.ref_color + 0,255,null_func);
   // createTrackbar("blu b","webcam stream",cube_polys.ref_color + 1,255,null_func);
   // createTrackbar("blu c","webcam stream",cube_polys.ref_color + 2,255,null_func);
   // createTrackbar("red a","webcam stream",cube_polys.ref_color + 3,255,null_func);
   // createTrackbar("red b","webcam stream",cube_polys.ref_color + 4,255,null_func);
   // createTrackbar("red c","webcam stream",cube_polys.ref_color + 5,255,null_func);
   // createTrackbar("wht a","webcam stream",cube_polys.ref_color + 6,255,null_func);
   // createTrackbar("wht b","webcam stream",cube_polys.ref_color + 7,255,null_func);
   // createTrackbar("wht c","webcam stream",cube_polys.ref_color + 8,255,null_func);
   // createTrackbar("grn a","webcam stream",cube_polys.ref_color + 9,255,null_func);
   // createTrackbar("grn b","webcam stream",cube_polys.ref_color + 10,255,null_func);
   // createTrackbar("grn c","webcam stream",cube_polys.ref_color + 11,255,null_func);
   // createTrackbar("orn a","webcam stream",cube_polys.ref_color + 12,255,null_func);
   // createTrackbar("orn b","webcam stream",cube_polys.ref_color + 13,255,null_func);
   // createTrackbar("orn c","webcam stream",cube_polys.ref_color + 14,255,null_func);
   // createTrackbar("yel a","webcam stream",cube_polys.ref_color + 15,255,null_func);
   // createTrackbar("yel b","webcam stream",cube_polys.ref_color + 16,255,null_func);
   // createTrackbar("yel c","webcam stream",cube_polys.ref_color + 17,255,null_func);

    for(;;) 
    {
        clock_gettime(CLOCK_MONOTONIC_RAW,&start);
        f_count++;
        *(caps[0]) >> *(ims[0]);
        *(caps[1]) >> *(ims[1]);
        hconcat(*(ims[0]),*(ims[1]),wide_image); //combine cameras
        //resize(wide_image,wide_image,Size(),3,3,INTER_NEAREST);
        compute_colors(true);
        char overlay[200];
        sprintf(overlay,"FPS: %.2f, ms/frame: %.2f",fps,msf);
        putText(wide_image,overlay,cvPoint(30,30),FONT_HERSHEY_SIMPLEX,0.8,cvScalar(0,255,0),1,CV_AA);
        imshow("webcam stream",wide_image);

        if(waitKey(1) > 0) break;
        clock_gettime(CLOCK_MONOTONIC_RAW,&stop);
        t_count += (stop.tv_sec - start.tv_sec) * 1000000 + (stop.tv_nsec - start.tv_nsec) / 1000;
        if(f_count > 20)
        {
            //printf("FPS: %ld, ms/frame: %ld.\n",10000000/t_count,t_count/(10*1000));
            fps = 20000000.f/((float)t_count);
            msf = ((float)t_count)/(20000.f);
            t_count = 0;
            f_count = 0;
        }
    }


}

void vision_solve_slow()
{
    printf("\tpress key to capture and start solve\n");
    for(;;) 
    {
        start_solve_timer();
        *(caps[0]) >> *(ims[0]);
        *(caps[1]) >> *(ims[1]);
        hconcat(*(ims[0]),*(ims[1]),wide_image); //combine cameras
        //resize(wide_image,wide_image,Size(),3,3,INTER_NEAREST);
        compute_colors(true);
        imshow("webcam stream",wide_image);
        if(waitKey(1) > 0) break;
    }
    set_rts_pin(1);
    *(caps[0]) >> *(ims[0]);
    *(caps[1]) >> *(ims[1]);
    hconcat(*(ims[0]),*(ims[1]),wide_image); //combine cameras
    compute_colors(true);
}

//mouse button handler
void align_callback(int event, int x, int y, int flags, void* junk)
{
    //if right button, delete last point (if possible)
    if(event == EVENT_RBUTTONDOWN)
    {
        unsave_changes = 1;
        printf("-->Mouse Clicked!\n");
        printf("\tcurrent facelet: %d\n",current_facelet);
        printf("\tcurrent polygon: %d\n",current_poly);
        facelet_roi* c_roi = &cube_polys.rois[current_facelet].rois[current_poly];
        if(c_roi->n_pts > 0) c_roi->n_pts--;
    }

    if(event != EVENT_LBUTTONDOWN) return;

    unsave_changes = 1;
    printf("-->Mouse Clicked!\n");
    printf("\tgot click at   : (%d, %d)\n",x,y);
    printf("\tcurrent facelet: %d\n",current_facelet);
    printf("\tcurrent polygon: %d\n",current_poly);

    //check to see if facelet_roi exists
    if(current_poly >= cube_polys.rois[current_facelet].n_rois) cube_polys.rois[current_facelet].n_rois++;

    //current region of interest
    facelet_roi* c_roi = &cube_polys.rois[current_facelet].rois[current_poly];

    //check to make sure we aren't exceeding maximum number of points
    if(c_roi->n_pts >= K_PTS_ROI)
    {
        printf("[ERROR] too many points. use less.\n");
        return;
    }

    //its safe to add another point
    c_roi->n_pts++;
    c_roi->opencv_pts[c_roi->n_pts-1].x = x;
    c_roi->opencv_pts[c_roi->n_pts-1].y = y;
    c_roi->x_vals[c_roi->n_pts-1] = x;
    c_roi->y_vals[c_roi->n_pts-1] = y;
}

// get a reasonable position for the label
void label_pos(facelet_roi* fr, cv::Point* pt)
{
    float x_sum = 0.f;
    float y_sum = 0.f;

    for(int i = 0; i < fr->n_pts; i++)
    {
        x_sum+= fr->opencv_pts[i].x;
        y_sum+= fr->opencv_pts[i].y;
    }

    pt->x = x_sum/fr->n_pts;
    pt->y = y_sum/fr->n_pts;
}

// draw stuff on image for align tool
void draw_align(Mat& im,char show_text)
{
    // currently selected polygon in top corner
    if(show_text)
    {
        char overlay[200];
        sprintf(overlay,"fl: %d (%s), poly: %d",current_facelet,number_to_name(current_facelet),current_poly);
        putText(wide_image,overlay,cvPoint(30,30),FONT_HERSHEY_SIMPLEX,0.8,cvScalar(0,255,0),1,CV_AA);
    }

    //loop through facelets
    for(int i = 0; i < 54; i++)
    {
        //loop through rois
        for(int j = 0; j < cube_polys.rois[i].n_rois; j++)
        {
            facelet_roi* fr = &cube_polys.rois[i].rois[j];
            //dont draw anything if no points...
            if(fr->n_pts <= 0) continue;

            // color based on active/inactive
            Scalar color;
            if( (i==current_facelet) && (j==current_poly) )
                color = Scalar(0,255,0);
            else if (i == current_facelet)
                color = Scalar(100,180,100);
            else
                color = Scalar(255,0,0);

            // draw points/lines
            for(int k = 0; k < fr->n_pts; k++)
            {
                circle(im,fr->opencv_pts[k],3,color,3);
                if(k == 0) continue;
                line(im,fr->opencv_pts[k-1],fr->opencv_pts[k],color,1);
            }

            //dont forget the last line!
            if(fr->n_pts > 1)
                line(im,fr->opencv_pts[fr->n_pts - 1],fr->opencv_pts[0],color,1);
        }
    }

    //draw labels at the end so they show up on top.
    for(int i = 0; i < 54; i++)
    {
        for(int j = 0; j < cube_polys.rois[i].n_rois; j++)
        {
            facelet_roi* fr = &cube_polys.rois[i].rois[j];
            //dont draw anything if no points...
            if(fr->n_pts <= 0) continue;
            Point label(0,0);
            label_pos(fr,&label);
            char label_s[200];
            sprintf(label_s,"%s_%d",number_to_name(i),j);
            putText(im,label_s,label,FONT_HERSHEY_SIMPLEX,0.4,Scalar(0,0,255),1,CV_AA);
        }
    }
}

// handle keypresses during align
void handle_align_keys(int a)
{

    if((a&0xff)=='f') { current_facelet++; current_poly = 0; }
    if((a&0xff)=='d') { current_facelet--; current_poly = 0; }
    if((a&0xff)=='p') current_poly++;
    if((a&0xff)=='o') current_poly--;

    if(current_poly < 0) current_poly = 0;
    if(current_poly > K_ROI_FACELET-1) current_poly = K_ROI_FACELET - 1;
    if(current_facelet < 0) current_facelet = 0;
    if(current_facelet > 53) current_facelet = 53;
}

// display video for alignment mode
void align_video()
{
    align_done = 0;
    int first_run = 1;
    printf("[Webcam] Opening video stream for alignment sequence... Press q to abort.\n");
    for(;;)
    {
        *(caps[0]) >> *(ims[0]);
        *(caps[1]) >> *(ims[1]);
        hconcat(*(ims[0]),*(ims[1]),wide_image);
        //resize(wide_image,wide_image,Size(),3,3,INTER_NEAREST);
        draw_align(wide_image,1);


        imshow("webcam stream",wide_image);
        if(first_run)
        {
            first_run = 0;
            setMouseCallback("webcam stream",align_callback,NULL);
        }
        int a = waitKey(1);
        //if(a>0)
        //printf("key: %d\n",a&0xff);

        if((a&0xff)=='q' || align_done) break;
        handle_align_keys(a);
    }
}

void align_still()
{
    get_webcam_image(); 
    Mat clean_image = wide_image.clone();
    align_done = 0;
    int first_run = 1;
    printf("[Webcam] Opening video stream for alignment sequence... Press q to abort.\n");
    for(;;)
    {
        wide_image = clean_image.clone();
        draw_align(wide_image,1);


        imshow("webcam stream",wide_image);
        if(first_run)
        {
            first_run = 0;
            setMouseCallback("webcam stream",align_callback,NULL);
        }
        int a = waitKey(10);
        //if(a>0)
        //printf("key: %d\n",a&0xff);

        if((a&0xff)=='q' || align_done) break;
        handle_align_keys(a);
    }
}

// get a single frame from webcam
void get_webcam_image()
{
    printf("[Image Acquisition] Getting frame from webcams.\n");
    if(webcam_init <= 1)
    {
        printf("[Error] webcam not initialized!\n");
        return;
    }

    *(caps[0]) >> *(ims[0]);
    *(caps[1]) >> *(ims[1]);
    hconcat(*(ims[0]),*(ims[1]),wide_image);
}


// get a single frame from file
void get_file_image(int image, char* filename)
{
    printf("[Image Acquisition] Opening file with name %s\n",filename);
    *(ims[image]) = cv::imread(filename);
    if(!(ims[image])->data)
        printf("[Error] Failed to get image data!\n");
}


// display single frame
void display_single_frame()
{
    if(!image1.data )//|| !image2.data)
    {
        printf("[Error] Attempted to display empty image!\n");
        return;
    }

    printf("[Image Acquisition] Displaying Single Frame.  Press any key to close.\n");
    namedWindow("Image 1",WINDOW_AUTOSIZE);
    imshow("Image 1",wide_image);
    //namedWindow("Image 2",WINDOW_AUTOSIZE);
    //imshow("Image 2",image2);
    waitKey(0);
    destroyAllWindows();
    waitKey(1);
    //  destroyWindow("Image 1");
    //  //destroyWindow("Image 2");
    //  destroyAllWindows();
    //  waitKey(0);
}

// run align tool
void align_all()
{
    align_video();
}


//initializes the data structure for a totally new alignment sequence
void init_polys()
{
    for(int i = 0; i < 54; i++)
    {
        cube_polys.rois[i].n_rois = 0;
        for(int j = 0; j < K_ROI_FACELET; j++)
        {
            cube_polys.rois[i].rois[j].n_pts = 0;
            cube_polys.rois[i].rois[j].weight = 0.f;
            cube_polys.rois[i].rois[j].opencv_pts = new cv::Point[K_PTS_ROI];
            for(int k = 0; k < K_PTS_ROI; k++)
            {
                cube_polys.rois[i].rois[j].x_vals[k] = -1.f;
                cube_polys.rois[i].rois[j].y_vals[k] = -1.f;
            }
        }
    }
}

//deletes all pointers in the data structure
void kill_pointers()
{
    for(int i = 0; i < 54; i++)
        for(int j = 0; j < K_ROI_FACELET; j++)
            file_cube_polys.rois[i].rois[j].opencv_pts = 0;
}

//if a structure has the point lists but not the opencv points
//this function allocates and sets pointers
void create_opencv_points()
{
    int n_pts = 0;

    for(int i = 0; i < 54; i++)
        for(int j = 0; j < K_ROI_FACELET; j++)
            cube_polys.rois[i].rois[j].opencv_pts = new cv::Point[K_PTS_ROI];


    for(int i = 0; i < 54; i++)
    {
        for(int j = 0; j < cube_polys.rois[i].n_rois; j++)
        {
            int n_cvp = cube_polys.rois[i].rois[j].n_pts;
            //cube_polys.rois[i].rois[j].opencv_pts = new cv::Point[K_PTS_ROI];
            for(int k = 0; k < n_cvp; k++)
            {
                cube_polys.rois[i].rois[j].opencv_pts[k].x = cube_polys.rois[i].rois[j].x_vals[k]/3;
                cube_polys.rois[i].rois[j].opencv_pts[k].y = cube_polys.rois[i].rois[j].y_vals[k]/3;
                n_pts++;
            }
        }
    }
    printf("[create opencv points] created %d points\n",n_pts);
}

//checks to see if point lists match opencv points
int check_consistency_save()
{

}

//checks to see if sizes matches
int check_consistency_load()
{

}


