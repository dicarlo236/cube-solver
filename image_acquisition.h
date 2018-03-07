#ifndef _image_acquisition
#define _image_acquisition

void get_webcam_image();
void init_webcam(int image);
void get_file_image(int image, char* filename);
void display_single_frame();
void webcam_video();
void webcam_latency();
void align_all();
void init_polys();
void align_still();
void live_id();
void vision_solve_slow();
int get_unsaved_changes();
#endif
