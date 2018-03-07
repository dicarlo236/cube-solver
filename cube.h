#ifndef _cube
#define _cube

#include <opencv2/opencv.hpp>

#include "settings.h"

// a single polygon
struct facelet_roi
{
    int n_pts;
    float weight;
    float x_vals[K_PTS_ROI];
    float y_vals[K_PTS_ROI];
    cv::Point* opencv_pts;
};

// a facelet, containing some number of polygons
struct facelet_rois
{
    int n_rois;
    facelet_roi rois[K_ROI_FACELET];
};

// a cube of facelets, each containing polygons
struct cube_rois
{
    facelet_rois rois[54];
    int ref_color[18];
};

bool get_valid();
void build_solve_string(bool verbose);
void init_solver();
int name_to_number(char* name);
char* number_to_name(int facelet);
void test_facelet_functions();
void vomit_to_file(char* name);
void kill_pointers();
void create_opencv_points();
void read_file(char* name);
int* get_facelet_codes();
char* get_solution();
void init_solver2();
void start_solve_timer();

#endif
