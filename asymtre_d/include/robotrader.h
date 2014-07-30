/*
 * auctioneer.h
 */

#ifndef ROB_H
#define ROB_H

#include <auctioneer.h>

#define DIST(x1, y1, x2, y2) sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2))
#define MAXDIST DIST(0, 0, 10, 10)

typedef struct {
  int sid;
  float utility;
  Dllist coalition;
} Solution;

typedef struct {
  int rid; /* robot id */

  int x, y; /* (x, y) position of the robot */
  float speed; /* meters per second */

  int num; /* number of sensors */
  Dllist sensors; /* list of sensors this robot has */
  Dllist behaviors; /* list of behaviors this robot can do */

  int maxhelpers; /* maximum number of robots that one robot could help */
  int maxsubgroup; /* maximum sub group size */
  int numhelpers; /* number of helpers that can provide me help */

  Dllist infolist; /* what kind of information I can provide */
  Dllist *pslist; /* list of potential solutions */

  // record up to certain number of solutions for auction
  Dllist solutionlist; /* list of selected solutions (Solution *) */
  int s_i; /* solution i, = -1 if no solution is found*/
  int best_i; /* best solution i*/

  int currh; /* the number robots I am currently helping */
  int help; /* 1: helped by others; 0: otherwise */
  int capable; /* 1: capable to help other in current status; 0: otherwise */

  Dllist activeS; /* sensors that need to be activated */
  Dllist activeB; /* current active behaviors */

  pthread_mutex_t *lock; /* lock when writing certain data, such as activeB */
} Robot;

typedef struct { // other robot's position and id info
  int id;
  int x;
  int y;
} R_pos;


Robot *local; // the local robot
int myid; // robot id
int mytype; // my type
Dllist bidlist; // the list of bids
char fn[20] = "rec-auc.txt"; // output file that records auction process

#endif
