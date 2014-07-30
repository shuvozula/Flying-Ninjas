/*
 * reason.h -- distributed version
 * Fang (Daisy) Tang
 * Oct. 21, 2004
 */

#ifndef REASON_H
#define REASON_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <fields.h>
#include <dllist.h>
#include <jval.h>
#include <pthread.h>
#include <communicate.h>
#include <and_or_tree.h>

#define MSG_SIZE 100
#define INT_SIZE 10
#define MSS      5    // maximum subteam size
#define MTH      10    // maximum number of robot to provide help	
#define MAXCOST	 20.0 // maximum cost of a sensor
#define MNS      5    // maximum number of solutions accepted

typedef struct
{
  int type;  // H, I, C
  int from;  // from rid
	int to;	   // to rid
  int numinfo; // number of the information needed
  int opt;   // option
  int timer; // time stamp
  int utility; 
	int mid;   // the message id
  Dllist info;
} Message;


typedef struct {
  int line; /* line number */
  int num; /* any number */
  char *s; /* any string */
} Node;

typedef struct {
  int sid;
  float utility;
  Dllist coalition;
  int self;
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

typedef struct {
  char *name; /* the name of the atomic behavior */
  int type; /* type = 0 (self), type = 1 (other, information) */
  int way; /* the ith way to implement */
  //Dllist sensors; /* the sensor list */
  char *sensor; /* sensor type */
  float prob; /* success probability */
} Atomic;

typedef struct {
  char *name; /* the name of the composite behavior */
  int num; /* number of ways of implementation */
  Dllist blist; /* list of ways to implement this composite behavior */
} Composite;

typedef struct {
  int num; /* number of atomic or composite behaviors */
  Dllist behaviors;
} Behaviors;

typedef struct {
  char *sensor;
  int cost;
} Cost;

typedef struct
{
  int rid;
  char *activeB;
} Way;

typedef struct
{
  int hid; // the helper's id
  int solution_id; // which solution this helper is responding to
  float utility; // the utility the helper could provide
  float final_utility; // the final utility (helper+self)
} Help;

typedef struct
{
  int nid; // the needy's id
  float utility; // my utility of helping needy   
  Dllist activeB; // what should I activate in order to help
  int active; // =1, confirmed
} Needy;



/************************  PARSE.H  *****************************/

Behaviors *Clist;   /* pointer to a list of composite behaviors */
Behaviors *Alist;   /* pointer to a list of atomic behaviors */
Dllist    Costlist; /* cost list */

int myindex; // for the combine_list function in parse.c

/************************* REASON.H ***************************/

Dllist *hashway; /* a list of different solutions, item is Atomic */ 

int maxway; /* the current maximum number of ways */ 
Dllist ways;

Robot *local; /* the local robot */

Dllist *helplist; // I need help
Dllist tohelp;  

Dllist rankedlist; // ranked help list
int numranked; // the number of ranked help

float weight; // the weight factor

/*****************  INFOPROC.H ******************************/

typedef struct
{
  char *name; // name of this information type
  Dllist multinput;
  char *input;
  char *output;
  Dllist schema;
	float utility; // the best utility of generating this information type
} Infonode;

Dllist Needlist; /* need list */
Dllist Provlist; /* provider list */
Dllist Commlist; /* communication list */

//char goal[10];
char *goal;

// to analyze the communication overhead
int start_time;
FILE *f;
char *myfile;

// the and or tree processing
AO_node *roots; // for goal-related information types
int numroot;

AO_node *dummyroots; // for the information types that are not in the tree
int dummynum;

#endif
