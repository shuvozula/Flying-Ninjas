/*
 * auctioneer.h
 */

#ifndef AUC_H
#define AUC_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <fields.h>
#include <dllist.h>
#include <jval.h>
#include <pthread.h>
#include <communicate.h>

#define TOUT1 10 // 15 timeout in seconds
#define TOUT2 6  // 10
#define TOUT3 5  // 5
#define TOUT4 2
#define MSG_SIZE 100
#define INT_SIZE 10

// the bid struct
typedef struct
{
  int sid; // the solution id
  int numrobot; // the number of robots in the coalition
  Dllist coalition;
  float utility; // the coalition's inherent cost
  float nav_time; // time needed to navigate
  float push_time; // time needed to push the box
  int cost; // combined cost, normalized to (1 -- 10)
  int leaderID; // leader robot ID
  int awarded; // 0 = not awarded, 1 = awarded
} Bid;

// the box pushing task definition
typedef struct
{
  char *name;
  int x, y, ori; // (x, y, orientation) of the box
  int gx, gy; // (x, y) of the goal position
  int numrobot; // number of robots required to push the box
  int timeofarr; // time of arrival of the task to the auctioneer
  int timeofacc; // time of acceptance of the coalition leader
  int timeofcomp; // time of completion of the task
} Task;

// the message struct
typedef struct
{
  int type; // T = task, B = bid, W = winner, A = accept
  Task *t; // the task to be announced
  int cost; // the cost of the coalition
  int numrobot; // number of robots in a coalition
  int leaderID; // the id of the coalition leader
  int timeofarr; // the time of arrival of the task
  Dllist coalition;  // the list of IDs
} Message;

Dllist taskList; // the list of tasks ranked based on time of arrival
Dllist monitorList; // the list of tasks being executed
Dllist completionList; // the listo of tasks being completed
Dllist bidList; // the list of bid submitted for each task

int endoftask; // end of task signal
int endofauc; // end of auction signal
int auc_st; // the starttime of the auctioneer

#endif
