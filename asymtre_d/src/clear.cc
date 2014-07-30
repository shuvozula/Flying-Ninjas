/*************************************************************
 * Name: site_clearing.cc
 * Version: 1.0
 * Description: Lower level building blocks and
                higher level control
 * Developed by: Fang (Daisy) Tang, DILab, dilab@cs.utk.edu
 
 Copyright (C) 2006 UT Distributed Intelligence Laboratory
**************************************************************/

// Libraries

#include "clear.h"
#include "comm-robot.h"

/**************************
 * useful functions       *
 **************************/
//=== from follow protocol
void copy_pos(Position from, Position *to)
{
  to->x = from.x;
  to->y = from.y;
  to->ori = from.ori;
}

//=== end

void robot_read()
{
  if (robot->RequestData())
    exit(1);
  if (robot->Read())
    exit(1);
}

// combine vectors with specified weights
Vector combine_vector(Vector v1, double w1, Vector v2, double w2)
{
  Vector combined_v;

  combined_v.speed = v1.speed*w1 + v2.speed*w2;
  combined_v.turnrate = v1.turnrate*w1 + v2.turnrate*w2;

  return combined_v;
}

// write message to the log file
void write_log(int mytime, char *msg)
{
  FILE *fn;
  fn = fopen(logfile, "a");
  if (fn < 0) {
    printf("cannot open %s to write\n", logfile);
    exit(1);
  }
  fprintf(fn, "%d: %s\n", mytime, msg);
  fclose(fn);
}

/*************************
 * Environmental Sensors *
 *************************/

/* -----------------------  ES_1 -------------------------
 * input: laser signal
 * output: robot's position, laser ranges and intensities
 * -------------------------------------------------------*/
void ES1_laser::compute_output() {
  int i;

  //=== from follow protocol
  robot_read();
  localize_count = localp->hypoth_count;
  for (i = 0; i < localize_count; i++) {
    output_localize[i] = localp->hypoths[i]; // x, y position and orientation
  }
  //=== end 

  robot_read();
  laser_count = lp->scan_count; //new version
  //laser_count = lp->range_count; 
 
  robot_read();
  for (i = 0; i < laser_count; i++) {
    output_laser[0][i] = lp->scan[i][0]; // ranges (m) new version
    //output_laser[0][i] = lp->ranges[i]/1000.0; // ranges in meters
    output_laser[1][i] = lp->intensities[i]; // intensities
  }
}  

/* -----------------  ES_2 ------------------------
 * func: find the blob
 * input: camera signal
 * output: blob data
 * -----------------------------------------------*/
void ES2_camera::compute_output()
{
  int i;
  int area = 0;

  // intialize
  blob_index = -1; 
  num_no_blob = 0;
  succ = 0; 
  // find the index of the blob which has the largest area
  while (blob_index < 0) {// while didn't find the blob
    robot_read();
    for (i = 0; i < bp->blob_count; i++) {
      if (area < bp->blobs[i].area && bp->blobs[i].area > MINBLOB) {
        area = (int)bp->blobs[i].area;
        blob_index = i;
      }
    }
    if (blob_index >= 0) {
      blob_history[blob_top%40] = area;
      blob_top = (blob_top + 1) % 40;
      break;
    } else {
      printf("can't see blob\n");
      if (num_no_blob > 0) {
        if (pos == 0) {//left pusher pans right
          pp->SetSpeed(0, DTOR(-15));
        } else {//right pusher pans left
          pp->SetSpeed(0, DTOR(15));
        }
	sleep(1);
      }
      num_no_blob++;
      if (num_no_blob > 6) break; 
    }
  } 
  if (num_no_blob > 6 && blob_index < 0) {
    // no blob detected. two reasons: (1) the pusher arrives at 
    // at the goal; (2) the goal is out of the fov of the camera
    if ((succ = check_blob_history()) == 1) {
      printf("Camera Test Succeeds.\n");
      return;
    }
  }
}

void ES3_sonar::compute_output()
{
  int i;

  //=== from follow protocol
  robot_read();
  localize_count = localp->hypoth_count;
  for (i = 0; i < localize_count; i++) {
    output_localize[i] = localp->hypoths[i]; // x, y position and orientation
  }
  //=== end

  robot_read();
  sonar_count = sp->range_count;
  
  robot_read();
  for (i = 0; i < sonar_count; i++) {
    output_sonar[i] = sp->ranges[i]; // ranges in meters
  }
}

/*********************
 * Perceptual Schema *
 *********************/


/* ------------------------- PS_1 ---------------------------------
 * func(): calculate the orientation of box relative to the pusher
           in degrees by law of sines and cosines
 * input: use either laser
 * output: box orientation "n"
 * assume the pushers will not fall out of the bound of the box
 * retv = 0 found
------------------------------------------------------------------*/ 
void PS1_box_ori::compute_output()
{
  int i, j;
  double laser_ranges[laser_count];
  double sum = 0;
  double min = 999.0;
  int index = 0;

  for (i = THRESHOLD; i < laser_count-THRESHOLD; i++) {
    for (j = 0; j < 5; j++) {
      sum += input_laser[0][i+j];
    }
    laser_ranges[i] = sum/5;
    if (min > laser_ranges[i]) {
      min = laser_ranges[i];
      index = i;
    }
    sum = 0;
  }

  min = 999.0;
  for (i = index; i < index+5; i++) {
    if (min > input_laser[0][i]) {
      min = input_laser[0][i];
      index = i;
    }
  }
  //printf("\nindex: %d, min: %.2lf\n", index, input_laser[0][index]);
  //printf("n: %d\n", index);
  output_n = index; 
}


/* ----------------------- PS_2 -------------------------
 * func: calculate pushing direction
 * input: current goal position (blob data)
 * output: direction "p", in degrees
 * 70 < p < 110 ( camera's field of view)
 * if pushing direction changes (within some variance), 
 * then a new theta_0 should be calculated
--------------------------------------------------------*/
void PS2_pushing::compute_output()
{
  static int prev_p = 0; // the previous pushing direction
  int e = 5; // the error range of the pushing direction
  int left, right;
  //int incr = bp->blobs[blob_index].bottom-bp->blobs[blob_index].top;
  int incr = abs(bp->blobs[blob_index].left-bp->blobs[blob_index].right)/2;
  //printf("incr = %d\n", incr);

  update = 0;

  //bp->Print();
  if (pos == 0) { // the left pusher
    left = bp->blobs[blob_index].left - incr;
    if (left < 2) left = 2;
    left = bp->width - left;
    //printf("left:%d centroid:%d\n", left, bp->width-bp->blobs[blob_index].y);
    output_p = FOV + (int)(((double)left/bp->width)*(180-2*FOV));
  } else { // the right pusher
    right = bp->blobs[blob_index].right + incr;
    if (right > bp->width-1) right = bp->width-1;
    right = bp->width - right;
    //printf("bp->right: %d, right:%d centroid:%d\n", bp->blobs[blob_index].right, right, bp->width-bp->blobs[blob_index].y);
    output_p = FOV + (int)(((double)right/bp->width)*(180-2*FOV));
  }
  if (prev_p == 0 || output_p < prev_p-e || output_p > prev_p+e) {
    update = 1;
    prev_p = output_p;
  }
  //printf("p: %d\n", output_p); 

}

/* ------------------------  PS_3 ------------------------------
 * func: check to see if the robot has lost contact with the box
 * input: laser range data
 * output:
 * output_retv: 1: ok, 2: should align, 3: lost
 *-------------------------------------------------------------*/
void PS3_contact::compute_output()
{
  int i, j;
  double laser_ranges[laser_count];
  double sum = 0;
  double diff = 0.5; // difference in meters (sum)
  double min = 999.0;
  int min_index = 0;
  int edge = 0;

  for (i = THRESHOLD; i < laser_count-THRESHOLD; i++) {
    for (j = 0; j < 3; j++) {
      sum += input_laser[0][i+j];
    }
    laser_ranges[i] = sum/3;
    if (min > laser_ranges[i]) {
        min = laser_ranges[i];
        min_index = i;
    }
    //printf("%d: %.2lf ", i, laser_ranges[i]);
    //if ((i+1)%6 == 0) printf("\n");
    sum = 0;
  }
  // new
  if (laser_ranges[min_index] > 0.6) { // the box is not close to me
    output_retv = 2;
    output_min_index = 0;
    return;
  }
  // end
  
  //printf("---min_index: %d\n", min_index);

  // find the gap from THRESHOLD to min
  for (i = min_index; i > THRESHOLD+2; i--) {
    if (fabs(laser_ranges[i] - laser_ranges[i-2]) > diff) {
      edge = i - 1;
      break;
    }
  }

  if (edge == 0) {
    for (i = min_index; i < laser_count-THRESHOLD-2; i++) {
      if (fabs(laser_ranges[i+2] - laser_ranges[i]) > diff) {
        edge = i + 1;
        break;
      }
    }
  }
  //printf("edge: %d\n", edge);

  // check conditions
  if (min_index <= 70 || min_index >= 110) { // need to align
    output_retv = 2;
    output_min_index = min_index;
  } else output_retv = 1; // I am fine
}


/* -------------------- PS_4 ----------------------------
 * PS4_switch(): switch between motor schemas
 * input: n, p, theta_0, pos (0=left/1=right)
 * theta_t = n - p
 * output: 
 * speed_opt: 0 low speed, 1 high speed
 * output_opt: 1: push
---------------------------------------------------------*/
void PS4_switch::compute_output()
{
  int theta_t = input_n - input_p;
  int diff = 10;
  speed_opt = 0; // initially low speed

  //printf("p = %d, n = %d, theta_t = %d, theta_0 = %d, delta = %d\n", input_p, input_n, theta_t, theta_0, theta_t-theta_0);
  // if everything is normal, just push the box, Donald's protocol's variance
  if (theta_t - theta_0 > 5) {
    if (pos == 0) { // left pusher
      printf("left robot should push harder\n");
      speed_opt = 1;
    }
  } else if (theta_t - theta_0 < -5) {
    if (pos == 1) { // right pusher
      printf("right robot should push harder\n");
      speed_opt = 1;
    }
  } else {
    //printf("push normally\n");
  }
  output_opt = 1;
}

/* ------------------------ PS_5 --------------------------------
 * PS5_min(): calculate the min range of two continuous sonars
              and see if the robot needs to align with the box 
 * input: sonar range data
 * output: min_index, option (1: push, 2: align)
 * -------------------------------------------------------------*/
void PS5_min::compute_output()
{
  int i;
  for (i=0;i<sonar_count;i++) range_sum[i] = 0;
  /*
  printf("\n", sonar_count);
  for (i = 0; i < sonar_count; i++) {
    printf("%d: %.2lf  ", i, input_sonar[i]);
    if ((i+1)%4 == 0) printf("\n");
  }*/

  double min = 99.0;
  for (i = 0; i < sonar_count; i++) {
    range_sum[i] = input_sonar[i%sonar_count] + input_sonar[(i+1)%sonar_count];
    if (range_sum[i] < min) {
      min = range_sum[i];
      min_index = i;
    }
  }
  //printf("min=%.2lf, min_index = %d\n", min, min_index);
  if ((min_index < 1 || min_index > 6) && range_sum[min_index] < 1) {
    output_retv = 2; //needs to align
  } else output_retv = 1;
}

//=== from follow protocol
/* ----------------------- PS_13 -----------------------------
 * Func: calculate current global position
 * input: laser or sonar output
 * output: current self global position
 * orientation is within the range of (0, 360)
------------------------------------------------------------*/
void PS13_self_pos::compute_output()
{
  double max = 0.0;
  int i = 0, index = 0;

  // find the position with the highest weight
  for (i = 0; i < localize_count; i++) {
    if (input_localize[i].weight > max) {
      max = input_localize[i].weight;
      index = i;
    }
  }
  output_self_pos.x = input_localize[index].mean[0]; // meters
  output_self_pos.y = input_localize[index].mean[1]; // meters
  output_self_pos.ori = RTOD(input_localize[index].mean[2]); // orientation

  if (output_self_pos.ori < -180) output_self_pos.ori += 360;
  if (output_self_pos.ori < 0 && output_self_pos.ori >= -180)
    output_self_pos.ori += 360; // range 0 - 360
  while (output_self_pos.ori > 360) output_self_pos.ori -= 360;

  //printf("(x, y, ori) = (%.2lf %.2lf %d)\n", output_self_pos.x, output_self_pos.y, (int)output_self_pos.ori);
}
//==== end

/**************************
 * Communication schemas *
 *************************/

/*
 * converts an integer base 10 to ascii (string)
 */
void itoa(int n, char s[])
{ 
  short i, j, k;
  int c, sign;

  if((sign = n) < 0) n = -n;
  i = 0;
  do { 
    s[i++] = n % 10 + '0';
  } while((n/=10) > 0);

  if(sign < 0) s[i++] = '-';
  s[i] = '\0';
  k = i-1;
  for(j=0; j<k; j++) {
    c = s[j];
    s[j] = s[k];
    s[k] = c;
    k--;
  }
}

/*
 * send message:
 * S: to stop
 * R: to resume
 */
void send_cmd(int from_id, int to_id, char *type)
{
  char *msg = NULL;
  char *from = NULL, *to = NULL;

  msg = (char *) malloc(50*sizeof(int));
  from = (char *) malloc(15*sizeof(int));
  to = (char *) malloc(15*sizeof(int));

  itoa(from_id, from);
  itoa(to_id, to);

  strcpy(msg, type);
  strcat(msg, from);
  strcat(msg, "$");
  strcat(msg, to);
  strcat(msg, "!");

  printf("send msg: %s\n", msg);
  send_msg(rfd, msg, to_id);
}

/*
 * receive commands from other robots
 */
int recv_cmd()
{
  char *token, *ptr;
  char msg[MAXSIZE];
  int nbytes = 0;
  int from, to;

  while (1) {
    nbytes = recv_msg(rfd, msg);
    if (nbytes == 0) return 0; 
    //printf("recv_msg: %s\n", msg);

    token = strtok(msg, "!");
    while (token != NULL) {
      if (token[0] == 'S') { // stop signal
        ptr = strstr(token, "S");
        ptr++;
        from = atoi(ptr);
        if (from != friendid) continue;
        ptr = strstr(ptr, "$");
        ptr++;
        to = atoi(ptr);
        if (to != myid) continue;
        stopsignal = 1; // set the stop signal
        printf("%d gets a stop request from %d\n", to, from);
        break;
      } else if (token[0] == 'C') {
        ptr = strstr(token, "C");
        ptr++;
        from = atoi(ptr);
        if (from != friendid) continue;
        ptr = strstr(ptr, "$");
        ptr++;
        to = atoi(ptr);
        if (to != myid) continue;
        printf("%d gets a Complete message from %d\n", to, from);
        complete = 1;
	break;
      } else if (token[0] == 'P') {
        ptr = strstr(token, "P");
        ptr++;
        from = atoi(ptr);
        if (from != friendid) continue;
        ptr = strstr(ptr, "$");
        ptr++;
        to = atoi(ptr);
        if (to != myid) continue;
        pushed = 1; // set the pushed signal
        printf("%d gets a Pushed message from %d\n", to, from);
      } else if (token[0] == 'Y') {
        ptr = strstr(token, "Y");
        ptr++;
        from = atoi(ptr);
        if (from != friendid) continue;
        ptr = strstr(ptr, "$");
        ptr++;
        to = atoi(ptr);
        if (to != myid) continue;
        syn = 1;
        printf("%d gets a Synchronization message from %d\n", to, from);
      }
      token = strtok(NULL, "!");
    }
  }
  return nbytes;
}


/*****************
 * Motor schemas *
 *****************/

/* --------------- MS_1 --------------------
 * push(): push the box along direction p
 * input: n, p, position (0=left/1=right)
 * output: motor control vectors
------------------------------------------*/
void MS1_push::compute_vector()
{
  static int maxarea = 0; 
  static int decr_times = 0; // the number of times the area decreases
  double speed, turnrate;
  int area = bp->blobs[blob_index].area;

  if (maxarea < area) {
    maxarea = area;
    decr_times = 0;
  } else {
    decr_times++;
  }
  //printf("rows: %d, area = %d decr_times = %d\n", bp->height, area, decr_times);

  int diff = abs(abs(bp->blobs[blob_index].top - bp->blobs[blob_index].bottom) - bp->height);
  //printf("diff = %d\n", diff);
  // there are three conditions to determine whether the goal has been
  // achieved: 1) the area is continuously decreasing to 1/3 of the
  // maximum area detected so far; 2) the area is large enough to stop;
  // 3) the width of the blob is almost the width of the image
  if ((decr_times > 39 && area < maxarea/3) || area > 30000 || diff < 100) {
    speed = 0;
    turnrate = 0;
    output_retv = 1;
  } else {
    speed = SPEED[speed_opt]; // meters per second
  }
  turnrate = (dir - 90)/2; // (dir-90)/3 for app1, left: +, right: -

  //printf("dir: %d, speed: %f, turnrate: %f\n", dir, speed, turnrate);
  output_v.speed = speed;
  output_v.turnrate = DTOR(turnrate);
}

/*
 * -------------------------- MS_2 ------------------------------
 * func: when the robot is lost, try to acquire the end of
 *       the box and align with it
 * input: laser range data
 * output: 
 * if laser_min_index is less than 70, back up and turn right
 * if laser_min_index is greater than 110, back up and turn left  
 * -------------------------------------------------------------*/
void MS2_align::compute_vector()
{
  int min_index;
  static int dir = 1;

  if (laser_min_index < 0) min_index = sonar_min_index; // use sonar
  else if (laser_min_index == 0) {// choose one direction to turn
    pp->SetSpeed(0, DTOR(10));
    sleep(1);
    pp->SetSpeed(0, 0);
  } else min_index = laser_min_index; // user laser

  int turnrate = (min_index - 90)/4; 
  printf("min_index = %d, turnrate = %d\n", min_index, turnrate);
  if (min_index > 200) {
    printf("wrong min_index\n");
    pp->SetSpeed(0, DTOR(dir*8));
    if (dir > 0) dir = -1;
    else dir = 1;
    sleep(1);
    pp->SetSpeed(0, 0);
    return;
  }

  if (turnrate < 8 && turnrate > 0) turnrate = 8;
  else if (turnrate < 0 && turnrate > -8) turnrate = -8;

  if (min_index >= 70 && min_index <= 110) pp->SetSpeed(0, 0);
  else {
    pp->SetSpeed(-0.0, DTOR(turnrate));
    sleep(1);
  }
  /*
  int turns = 3;
  while (turns-- > 0) {
    pp->SetSpeed(-0.05, DTOR(turnrate));
    sleep(1);
  }*/
  //pp->SetSpeed(0, 0);
}

/*
 * ---------------------- MS_4  ----------------------------------
 *  func: move left or right along the box and align with the box
 *  input: sonar range data
 *  output: motor control vectors 
 * --------------------------------------------------------------*/
void MS4_move::compute_vector()
{
  static double prev_sum = -1.0;
  static int reached = 0; // whether the pusher has reached the end of the box
  int min; // min is 15 for left pusher, min is 7 for right pusher
  int dir; // dir is 1 for left pusher (turn left), -1 for right pusher
  if (pos == 0) {min = 15; dir = 1;}
  else {min = 7; dir = -1;}
  static int count = 0;

  output_retv = 0;
  //printf("min_index=%d, min=%d, range_sum[%d] = %.2lf, range_sum[min] = %.2lf\n", min_index, min, min_index, range_sum[min_index], range_sum[min]);

  // turn back to face the box when the pusher has reached the other end
  if (reached) {
    if ((min_index == 3 || min_index == 4) && range_sum[min_index] < 1.5) {
      //printf("aligned on the other side\n");
      if (range_sum[min_index] > 0.3) {
        pp->SetSpeed(0.25, 0);
	sleep(1);
      }
      pp->SetSpeed(0, 0);
      output_retv = 1;// done
      reached = 0; // set the value back
      count = 0;
      prev_sum = -1.0;
      // change pushing positions
      if (pos == 0) {pos = 1; printf("left-->right\n");}
      else if (pos == 1) {pos = 0; printf("right->left\n");}
      return;
    } else {
      //printf("turn back to face the box\n");
      pp->SetSpeed(0, DTOR(25*dir));
    }
    return;
  }
  // turn right until sonar[0]+sonar[15] is the min
  if (min_index == min && range_sum[min] <= 1) {
    //printf("move straight along the box\n");
    count++;
    prev_sum = range_sum[min_index];
    if (range_sum[min_index] <= .6)
      pp->SetSpeed(0.25, 0);
    else if (range_sum[min_index] > .6 && range_sum[min_index] <= 1) 
      pp->SetSpeed(0.2, DTOR(3*dir)); // closer to the box
  } else if (prev_sum > 0 && range_sum[min] > 1) {
    if (count < 15) {// not regular
      pp->SetSpeed(0, 0);
      return;
    }
    printf("reach the end of the box, count=%d\n", count);
    pp->SetSpeed(-0.21, 0);
    sleep(3);
    pp->SetSpeed(0, 0);
    reached = 1;
  } else if (range_sum[min_index] < 1 && prev_sum > 0 && (min_index == (min+1)%16 || min_index == (min-1)%16)) { 
    if (min_index == (min+1)%16) {
      pp->SetSpeed(0.15, DTOR(-10));
    } else { 
      pp->SetSpeed(0.15, DTOR(10));
    } 
  } else {
    pp->SetSpeed(0, DTOR(25*(-dir))); 
  } 
}

//=== from follow protocol
/* ----------------- MS_14 ------------------------
 * MS14_goto: navigate from current pos to goal pos
 * input: goal position, current position
 * output: motor control vector
-------------------------------------------------*/
void MS14_goto::compute_vector()
{
  double angle, dist;
  double ori; // current robot's orientation

  output_retv = 0;
  dist = DIST(goal_pos.x, goal_pos.y, input_self.x, input_self.y);
  printf("dist: %.2lf\n", dist);
  if (dist < MINDIST) { // return if arriving at goal
    output_retv = 1;
    return;
  }
  angle = RTOD(ANGLE(goal_pos.x, goal_pos.y, input_self.x, input_self.y));
  if (angle < 0) angle += 360.0;
  //printf("angle: %.1lf, orientation: %.1lf\n", angle, input_self.ori);
  
  ori = input_self.ori;

  double turnrate = 0.0, speed = 0.0;
  double mint = 1.0;

  // where is my goal?
  int diff = (int)abs((int)angle-(int)ori);
  if (diff <= 3) {
    turnrate = 0;
  } else if (diff <= 15) {
    turnrate = mint*2;
  } else if (diff <= 30) {
    turnrate = mint*5;
  } else if (diff <= 60 && diff > 30) {
    turnrate = mint*10;
  } else if (diff <= 90 && diff > 60) {
    turnrate = mint*15;
  } else if (diff < 145 && diff > 90) {
    turnrate = mint*20;
  } else if (diff < 180) { turnrate = mint*25;}

  if (ori < angle + 180 && ori > angle) {
    turnrate = -turnrate;
  } else {
    turnrate = turnrate;
  }
  //printf("diff: %d, turnrate: %.1lf\n", diff, turnrate);
  if (abs((int)turnrate) >= 10) output_v.speed = 0.02;
  else output_v.speed = 0.1;
  output_v.turnrate = DTOR(turnrate);
}
//=== end
 
/*************************
 * connection functions  *
 *************************/
//=== from follow protocol
void Pioneer::connect_es1_ps13()
{
  int i;
  ps13_self.localize_count = es1_laser.localize_count;
  for (i = 0; i < ps13_self.localize_count; i++)
    ps13_self.input_localize[i] = es1_laser.output_localize[i];
}

void Pioneer::connect_es3_ps13()
{
  int i;
  ps13_self.localize_count = es3_sonar.localize_count;
  for (i = 0; i < ps13_self.localize_count; i++)
    ps13_self.input_localize[i] = es3_sonar.output_localize[i];
}

void Pioneer::connect_ps13_ms14()
{
  copy_pos(ps13_self.output_self_pos, &(ms14_goto.input_self));
}

//=== end

void Pioneer::connect_es1_ps1()
{  
  ps1_box_ori.laser_count = es1_laser.laser_count;
  int i;
  for (i = 0; i < ps1_box_ori.laser_count; i++) {
    ps1_box_ori.input_laser[0][i] = es1_laser.output_laser[0][i];
    ps1_box_ori.input_laser[1][i] = es1_laser.output_laser[1][i];
  }
}

void Pioneer::connect_es1_ps3()
{
  ps3_contact.laser_count = es1_laser.laser_count;
  int i;
  for (i = 0; i < ps3_contact.laser_count; i++) {
    ps3_contact.input_laser[0][i] = es1_laser.output_laser[0][i];
    ps3_contact.input_laser[1][i] = es1_laser.output_laser[1][i];
  }
}

void Pioneer::connect_es3_ps5()
{
  ps5_min.sonar_count = es3_sonar.sonar_count;
  int i;
  for (i = 0; i < ps5_min.sonar_count; i++) {
    ps5_min.input_sonar[i] = es3_sonar.output_sonar[i];
  }
}

void Pioneer::connect_es2_ps2()
{
  ps2_pushing.blob_index = es2_camera.blob_index;
}

void Pioneer::connect_es2_ms1()
{
  ms1_push.blob_index = es2_camera.blob_index;
}

void Pioneer::connect_ps1_ps4()
{
  ps4_switch.input_n = ps1_box_ori.output_n;
}

void Pioneer::connect_ps2_ps4()
{
  ps4_switch.input_p = ps2_pushing.output_p;
  ps4_switch.blob_index = ps2_pushing.blob_index;
}

void Pioneer::connect_ps3_ms2()
{
  ms2_align.laser_min_index = ps3_contact.output_min_index;
  ms2_align.sonar_min_index = -1;
}

void Pioneer::connect_ps5_ms2()
{
  ms2_align.sonar_min_index = ps5_min.min_index*22 + 11;
  ms2_align.laser_min_index = -1;
}

void Pioneer::connect_ps2_ms1()
{
  ms1_push.dir = ps2_pushing.output_p;
}

void Pioneer::connect_ps4_ms1()
{
  ms1_push.speed_opt = ps4_switch.speed_opt;
  ms1_push.dir = ps4_switch.input_p;
  ms1_push.blob_index = ps4_switch.blob_index;
}

void Pioneer::connect_ps5_ms4()
{
  ms4_move.min_index = ps5_min.min_index;
  ms4_move.sonar_count = ps5_min.sonar_count;
  int i;
  for (i = 0; i<ms4_move.sonar_count; i++) {
    ms4_move.range_sum[i] = ps5_min.range_sum[i];
  }
}

/*
 * check_blob_history: check if the pushers have arrived at goal
 */
int check_blob_history()
{
  int i, end;
  int tmp[40];
  int decr_times = 0;

  if (blob_history[blob_top] > 0) {
    for (i = blob_top; i < 40; i++)
      tmp[i-blob_top] = blob_history[i];
    for (i = 0; i < blob_top; i++) 
      tmp[i+40-blob_top] = blob_history[i];
    end = 39;
  } else {
    for (i = 0; i < blob_top; i++)
      tmp[i] = blob_history[i];
    end = blob_top-1;
  }
  // if the area has been continueously decreasing for a while
  // return succ
  while (end > 0) {
    if (tmp[end] < tmp[end-1])
      decr_times++;
    else break;
    end--;
  }
  if (decr_times > 20) return 1;
  return 0;
}

void wait_patiently()
{
  int i;

  while (i < 15) {
    recv_cmd();
    if (pushed == 1) break;
    usleep(500000);
    printf("I am waiting patiently...\n");
    i++;
  }
}

/* connection made in Donald's approach 
 * a robot uses laser to push on one side of the box
 * return 1 means success
 */
int Pioneer::push_laser(int size)
{
  while (1) {
    // check whether the robot is in the right position
    es1_laser.compute_output();
    connect_es1_ps3();
    ps3_contact.compute_output();
    // different options
    if (ps3_contact.output_retv == 2) { // align
      //speech_p->Say("alignment");
      if (size == 2) send_cmd(myid, friendid, "S"); // tell my partner to stop
      connect_ps3_ms2();
      ms2_align.compute_vector();
    } else if (ps3_contact.output_retv == 1) { // push
      if (size == 2) {
        recv_cmd();
	// if mission is complete
	if (complete == 1) return 1;
        // if my friend needs me to wait on her
        if (stopsignal == 1) {
          pp->SetSpeed(0, 0);
          sleep(1); // waiting for the other robot to align
          stopsignal = 0;
        }
        // if my friend has pushed the other side
        if (pushed == 1) {
          pushed = 0; // reset the "pushed" signal
        } else { // wait till I am not patient
          wait_patiently();
          if (pushed == 1) { // finally got the message
            pushed = 0; // reset the "pushed" signal
          } 
        }
      }
      // 1. calculate pushing direction
      es2_camera.compute_output();
      if (es2_camera.succ) {
        printf("Goal arrived\n");
        return 1;
      }
      connect_es2_ps2();
      ps2_pushing.compute_output();

      // 2. calculate orientation	
      es1_laser.compute_output();
      connect_es1_ps1();
      ps1_box_ori.compute_output();

      // 3. update theta_0
      if (ps2_pushing.update == 1) {
        theta_0 = ps1_box_ori.output_n - ps2_pushing.output_p;
      }
      
      // 4. move towards the goal
      connect_ps1_ps4();
      connect_ps2_ps4();
      ps4_switch.compute_output();
      connect_ps4_ms1();
      ms1_push.compute_vector();
      pp->SetSpeed(ms1_push.output_v.speed, ms1_push.output_v.turnrate);
      //printf("speed: %.2f\n", ms1_push.output_v.speed);
      if (ms1_push.output_retv == 1) {
        if (size == 2) send_cmd(myid, friendid, "C");
        return 1;
      }
      printf("pushing ...\n");
      sleep(1);
      pp->SetSpeed(0, 0);
      if (size == 2) send_cmd(myid, friendid, "P"); // pushed
    }
  }
}

/* parker's approach on box pushing
 * a robot uses sonar to push on one side of the box
 * return 1 means success
 */
int Pioneer::push_sonar(int size) 
{
  while (1) {
    // check whether the robot is in the right position
    es3_sonar.compute_output();
    connect_es3_ps5();
    ps5_min.compute_output();
    // different options
    if (ps5_min.output_retv == 2) { // align
      speech_p->Say("alignment");
      if (size == 2) send_cmd(myid, friendid, "S"); // tell my partner to wait
      connect_ps5_ms2();
      ms2_align.compute_vector();
    } else if (ps5_min.output_retv == 1) { // push
      if (size == 2) {
        recv_cmd();
        // if my friend needs me to wait on her
        if (stopsignal == 1) {
          pp->SetSpeed(0, 0);
          sleep(4); // waiting for the other robot to align
          stopsignal = 0;
        }
        // if my friend has pushed the other side
        if (pushed == 1) {
          pushed = 0; // reset the "pushed" signal
        } else { // wait till I am not patient, and I'll push on both ends
          wait_patiently();
          if (pushed == 1) { // finally got the message
            pushed = 0; // reset the "pushed" signal
          } else { // still no message from the other robot
            // need to reconfigure solution
            printf("push_sonar: needs to reconfigure?\n");
            return 0;
          }
        }
      }
      // 1. calculate pushing direction
      es2_camera.compute_output();
      if (es2_camera.succ) {
        printf("Goal arrived\n");
        return 1;
      }
      connect_es2_ps2();
      ps2_pushing.compute_output();

      // 2. move towards the goal
      connect_es2_ms1();
      connect_ps2_ms1();
      ms1_push.speed_opt = 1; // high speed
      ms1_push.compute_vector();
      //printf("speed: %.2f\n", ms1_push.output_v.speed);
      pp->SetSpeed(ms1_push.output_v.speed, ms1_push.output_v.turnrate);
      if (ms1_push.output_retv == 1) {
        if (size == 2) send_cmd(myid, friendid, "C");
        return 1;
      }
      printf("pushing ...\n");
      sleep(1);
      pp->SetSpeed(0, 0);
      if (size == 2) send_cmd(myid, friendid, "P"); // pushed
    }
  }
}

//=== from follow protocol
int Pioneer::navigate()
{
  int succ;
  int i;

  // initialize localization
  pp->SetSpeed(0.05, 0);

  int index;
  double max = 0.0, threshold = 0.9;
  while (1) {
    robot_read();
    if (localp->hypoth_count == 0) continue;
    for (i = 0; i < localp->hypoth_count; i++) {
      if (localp->hypoths[i].weight >= max) {
        max = localp->hypoths[i].weight;
        index = i;
      }
    }
    if (max > threshold) break;
  }   
  printf("my initial position is (%.2lf, %.2lf, %d)\n", localp->hypoths[index].mean[0], localp->hypoths[index].mean[1], (int)RTOD(localp->hypoths[index].mean[2]));

  double dist = 0;   
  dist = DIST(goal_pos.x, goal_pos.y, localp->hypoths[index].mean[0], localp->hypoths[index].mean[1]);
  printf("Goal: (%.2lf, %.2lf), total dist = %.2lf\n", goal_pos.x, goal_pos.y, dist);

  pp->SetSpeed(0, 0);
  while (1) {
    // calculate self global position
    if (strcmp(option, "laser") == 0) {
      es1_laser.compute_output(); 
      connect_es1_ps13();
    } else if (strcmp(option, "sonar") == 0) {
      es3_sonar.compute_output();
      connect_es3_ps13();
    }
    ps13_self.compute_output();
    connect_ps13_ms14();
    ms14_goto.compute_vector();
    if (ms14_goto.output_retv == 1) {
      pp->SetSpeed(0, 0);
      succ = 1;
      break;
    } 
    pp->SetSpeed(ms14_goto.output_v.speed, ms14_goto.output_v.turnrate);
    //printf("speed: %.2lf, turnrate: %.2lf\n", ms14_goto.output_v.speed, ms14_goto.output_v.turnrate);
  }
  pp->SetSpeed(0, 0);
  return succ;
}
//=== end

/* ----------------------------------------------------------
 * initialize the connections of the schemas
 * ----------------------------------------------------------
 */
void synchronize()
{
  // wait for the message to start, here "Y" is a synchornization signal
  syn = 0;
  send_cmd(myid, friendid, "Y");
  while (1) {
    recv_cmd();
    if (syn == 1) break;
  }
}

int Pioneer::init_connection()
{
  int succ = 0;

  stopsignal = 0; // initially, there is no stop signal
  pushed = 1; // initially, assume both robots have pushed the box
  pp->SetSpeed(0, 0);

  if (strcmp(command, "clear1") == 0) {
    succ = navigate();
    printf("arrived at box\n");
    pp->SetSpeed(0, 0);
    if (strcmp(option, "sonar") == 0) {
      succ = push_sonar(1);
    } else if (strcmp(option, "laser") == 0) {
      succ = push_laser(1);
      printf("pushed box to the goal\n");
    }
  } else if (strcmp(command, "clear2") == 0) {
    succ = navigate();
    printf("arrived at the box\n");
    synchronize();
    if (strcmp(option, "sonar") == 0) {
      push_sonar(2);
    } else if (strcmp(option, "laser") == 0) {
      succ = push_laser(2);
      printf("pushed box to the goal\n");
    }
  } else {
    printf("wrong option!\n");
    pp->SetSpeed(0, 0);
  }
  pp->SetSpeed(0.1, 0);
  sleep(2);
  pp->SetSpeed(-0.12, 0);
  sleep(3);
  //pp->SetSpeed(0, DTOR(30));
  //sleep(6);
  pp->SetSpeed(0, 0);
  return succ;
}

/*
 * configure the robot to activate the needed schemas
 */
void Pioneer::configure()
{ 
}

/*
 * change the current robot configuration file
 * and reason again on the new solution
 */
void Pioneer::reconfigure()
{
}


void parse_args(int argc, char **argv)
{
  if (argc != 7) {
    printf("site clearing USAGE:clear1/clear2 sonar/laser pos(0=left/1=right) my_id friend_id goal_x goal_y\n");
    exit(1);
  }
  option = strdup(argv[1]);
  pos = atoi(argv[2]);
  myid = atoi(argv[3]);
  friendid = atoi(argv[4]); 
  if (friendid == 0) command = strdup("clear1");
  else command = strdup("clear2");
  goal_pos.x = atoi(argv[5]);
  goal_pos.y = atoi(argv[6]);

  complete = 0;

  // init communication
  rfd = open_socket();
  bind_socket(rfd, COMMPORT);
  fcntl(rfd, F_SETFL, O_NONBLOCK);

  // instantiate player proxies
  robot = new PlayerClient(host, port);
  robot->SetDataMode(PLAYER_DATAMODE_PULL_ALL);

  pp = new PositionProxy(robot, 0, 'a');
  if (pp->SetMotorState(1)) {
    printf("Problems with pp->SetMotorState\n");
    exit(1);
  }
  lp = new LaserProxy(robot, 0, 'r');
  //sp = new SonarProxy(robot, 0, 'r');
  speech_p = new SpeechProxy(robot, 0, 'w');
  localp = new LocalizeProxy(robot, 0, 'r');

  // setting up blob finder proxy
  // track the orange color, which is set up in the configuration file
  bp = new BlobfinderProxy(robot, 0, 'r');
 
  return;
}

int main(int argc, char **argv)
{
  Pioneer pioneer;
  int succ = 0;

  parse_args(argc, argv);
  logst = time(0);

  // open an empty logfile
  FILE *fn;
  sprintf(logfile, "log%d", myid);
  /*
  fn = fopen(logfile, "w");
  if (fn < 0) {
    printf("Cannot open %s to write\n", logfile);
    exit(1);
  }
  fclose(fn);*/

  pioneer.configure();

  while (!succ) {
    succ = pioneer.init_connection();
    if (succ != 1) pioneer.reconfigure();
    else break;
  }
}
