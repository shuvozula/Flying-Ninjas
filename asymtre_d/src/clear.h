/*************************************************************
 * Name: site-clearing.h
 * Version: 1.0
 * Description: .h file for the site clearing task
 * In fact: a combination of boxpush.h and follow.h
 * Developed by: Fang (Daisy) Tang, DILab, dilab@cs.utk.edu

 Copyright (C) 2006 UT Distributed Intelligence Laboratory
**************************************************************/

#ifndef _CLEAR_H_
#define _CLEAR_H_

// Libraries
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <playerclient.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <values.h>
#include <cmath>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <limits.h> 

#define PI 3.1415926
#define MINDIST .1 // in meters

#define DIST(x1, y1, x2, y2) sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2))
#define ANGLE(x1, y1, x2, y2) atan2(y1-y2, x1-x2)
#define SQUARE(x) x*x
#ifndef MIN
  #define MIN(a, b) ((a < b) ? (a) : (b))
#endif
#ifndef MAX
  #define MAX(a, b) ((a > b) ? (a) : (b))
#endif

typedef struct {
  double x;
  double y;
  double ori; // orientation
} Position;

typedef struct { // the motor control vector
  double speed;
  double turnrate;
  //double orientation;
} Vector;

/* ----------- ES class and subclasses ------------------*/

class ES {
public:
  virtual void compute_output(){}
}; 

/* laser on the Pioneer robot
 * input: laser signal
 * output: robot's position, ranges and intensities
 */
class ES1_laser : public ES {
public:
  localize_hypoth output_localize[PLAYER_LOCALIZE_MAX_HYPOTHS];
  int localize_count;
  double output_laser[2][PLAYER_LASER_MAX_SAMPLES];
  int laser_count;
  void compute_output();
};

/* camera on the Pioneer robot
 * input: camera signal
 * output: blob data
 */
class ES2_camera: public ES {
public: 
  int  blob_index;
  int  num_no_blob; // number of times where no blob is found
  int  succ; // arrived at the goal?
  void compute_output();
};

// sonar on the Pioneer robot
class ES3_sonar: public ES {
public:
  double output_sonar[PLAYER_SONAR_MAX_SAMPLES];
  int sonar_count;
  // sonar localization
  localize_hypoth output_localize[PLAYER_LOCALIZE_MAX_HYPOTHS];
  int localize_count;

  void compute_output();
};

/*---------------- PS class and subclasses --------------------*/

class PS {
public:
  virtual void compute_output() {}
};

/*---------  from the box pushing protocol  ------------*/
/* 
 * Func: calculate the orientation of the box relative to the pusher
 * input: laser range data
 * output: box orientation n (in degrees)
 */
class PS1_box_ori : public PS {
public:
  int laser_count;
  double input_laser[2][PLAYER_LASER_MAX_SAMPLES];
  
  int output_n;
  int output_retv; // 1=lost, 0=found
  void compute_output();
};


/* 
 * Func: calculate the pushing direction
 * input: goal position relative to the pushers
 * output: pushing direction "p"
 */
class PS2_pushing : public PS {
public:
  int blob_index;
  int output_p;
  int update;
  void compute_output();
};

/* 
 * Func: check to see if the robot has lost contact with the box
 * input: laser range data
 * output: 
 * output_retv: 1: ok, 2: should align, 3: lost
 */
class PS3_contact : public PS {
public:
  int laser_count;
  double input_laser[2][PLAYER_LASER_MAX_SAMPLES];

  int output_retv;
  int output_min_index;
  void compute_output();
};

/*
 * Func: switch between motor schemas
 * input: n, p, theta_0, pos (0=left/1=right)
 * output: push or align
 */
class PS4_switch : public PS {
public:
  int  input_n, input_p;

  int  blob_index;
  int  speed_opt; // high speed or low speed
  int  output_opt; // which option
  void compute_output();
};

/*
 * Func: calculate the minimum of two continuous sonar ranges
 *       and see if the robot needs to align with the box 
 * input: sonar range data
 * output: min index, 0 means sonar 0 and 1
 *         output_retv: 1 push, 2 align
 */
class PS5_min : public PS {
public:
  int    sonar_count;
  double input_sonar[PLAYER_SONAR_MAX_SAMPLES];

  int    min_index;
  double range_sum[PLAYER_SONAR_MAX_SAMPLES];
  int    output_retv;
  void   compute_output();
};

/*----------  from the follow protocol  ---------*/
/*
 * Func: calculate current global position
 * input: laser, or sonar
 * output: current self global position
 */
class PS13_self_pos : public PS {
public:
  int localize_count;
  localize_hypoth input_localize[PLAYER_LOCALIZE_MAX_HYPOTHS];
  Position output_self_pos;
  void compute_output();
};

/*---------------- CS class and subclasses ------------------*/

class CS {
public:
  virtual void communicate() {}
};

/*---------------- MS class and subclasses ------------------*/

class MS {
public:
  virtual void compute_vector() {}
};

/*------  from boxpush protocol ----------*/
/*
 * func: move towards the perceived goal position
 * input: blob data of the goal position
 * output: motor control vectors
 */
class MS1_push : public MS{
public:
  int    dir; // pushing direction
  int    blob_index; // blob's index
  int    speed_opt; // speed option
  Vector output_v;
  int    output_retv;
  void   compute_vector();
}; 

/*
 * Func: try to align with the box
 * input: laser range data 
 * output: motor control vectors
 */
class MS2_align : public MS {
public:
  int    laser_min_index;
  int    sonar_min_index;

  Vector output_v;
  int    output_retv;
  void   compute_vector();
};

/*
 * Func: move along the box and align
 * input: sonar range data
 * output: motor control vectors
 */
class MS4_move : public MS {
public:
  int    min_index;
  int    sonar_count;
  double range_sum[PLAYER_SONAR_MAX_SAMPLES];

  Vector output_v;
  int    output_retv;
  void   compute_vector();
};

/*------ from follow protocol --------------*/

class MS14_goto : public MS {
public:
  Position input_goal;
  Position input_self;
  int output_retv;
  Vector output_v;
  void compute_vector();
};

/*----------------- robot class ------------------*/

class Robot {
public:
  virtual void configure() {};
  virtual void reconfigure() {};
  virtual int  init_connection() {};
};

class Pioneer : public Robot {
  // environmental sensors
  ES1_laser  es1_laser;  
  ES2_camera es2_camera;
  ES3_sonar  es3_sonar;

  // perceptual schemas
  PS1_box_ori    ps1_box_ori;
  PS2_pushing    ps2_pushing;
  PS3_contact    ps3_contact;
  PS4_switch     ps4_switch;  
  PS5_min        ps5_min;

  PS13_self_pos ps13_self;

  // motor schemas 
  MS1_push  ms1_push;
  MS2_align ms2_align;
  MS4_move  ms4_move;

  MS14_goto ms14_goto;

  // others
  int fd;

public:
  void configure();
  void reconfigure();
  int init_connection(); 

  int push_laser(int);
  int push_sonar(int);
  int push_one();
  int navigate();

  void connect_es1_ps1();
  void connect_es1_ps3();
  void connect_es2_ps2();
  void connect_es3_ps5();

  void connect_es1_ps13();
  void connect_es3_ps13();

  void connect_es2_ms1();

  void connect_ps1_ps4();
  void connect_ps2_ps4();
  void connect_ps2_ms1();
  void connect_ps4_ms1();
  void connect_ps3_ms2();
  void connect_ps5_ms2();
  void connect_ps5_ms4();

  void connect_ps13_ms14();
};  

/*----------------  Global variables -------------------*/
char host[256] = "localhost";
int port = PLAYER_PORTNUM;
PlayerClient *robot;
LaserProxy *lp;
SonarProxy *sp;
BlobfinderProxy *bp;
PositionProxy *pp;
SpeechProxy *speech_p;

LocalizeProxy *localp;

bool turnOnMotors = false;

double SPEED[2] = {0.15, 0.15}; //{0.08, 0.11};
int MINBLOB = 100;
int FOV = 70; // has been varified
int THRESHOLD = 6; // how many laser data I am going to ignore 0 to THRESHOLD,
                    // and (180-THRESHOLD) to 180
int theta_0 = 0;

int syn = 0; // the synchornization signal
int stopsignal; // whether my friend has told me to stop and wait
int pushed; // whether my friend has pushed the box
int complete; // whether mission is complete

char *command; // "clear1", "clear2"
char *option; // "sonar", "laser"
int  blob_history[40]; // contains up to 40 most recent blob areas history
int  blob_top = 0; // the most recent blob area is going to be stored in blob_history[blob_top].

// recording the main events
char logfile[10];
int  logst; // log file's starting time

// command line arguments
int pos; //0=left, 1=right
int myid;
int friendid;

// communication stuff
int rfd;
struct sockaddr_in remote_addr;
struct sockaddr_in local_addr;
#define COMMPORT 4953
#define MAXSIZE 1024

/*---------- from follow protocol -----------------*/
Position Goal;
Position goal_pos;


/*-------------- functions -------------------------*/
int  send_msg(int, char *, int);
int  open_socket();
void bind_socket(int, int);
int  check_blob_history();

#endif
