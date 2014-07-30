
/*
 * robotrader.c -- a robot trader program
 * Fang (Daisy) Tang @ DILAB
 * May, 2006
 */

#include <robotrader.h>

/*
 * negotiation process by calling asymtre
 * return the current task if there is at least one solution
 */
Task *negotiate(char *token)
{
  Task *t;
  char *p, *p_end;
  int i, myargc = 4;
  char *myargv[4];
  char id[10], type[10];

  // create a new task
  t = (Task *) malloc(sizeof(Task));
  p = strstr(token, "$");
  p++;
  p_end = strstr(p, "$");
  t->name = (char *)strndup(p, p_end-p);
  p = strstr(p, "$");
  p++;
  t->x = atoi(p);
  p = strstr(p, "$");
  p++;
  t->y = atoi(p);
  p = strstr(p, "$");
  p++;
  t->ori = atoi(p);
  p = strstr(p, "$");
  p++;
  t->numrobot = atoi(p);
  p = strstr(p, "$");
  p++;
  t->timeofarr = atoi(p); 
  p = strstr(p, "$");
  p++;
  t->gx = atoi(p);
  p = strstr(p, "$");
  p++;
  t->gy = atoi(p);

  // call the ASyMTRe reasoning process
  myargv[0] = "reason";
  sprintf(id, "%d", myid);
  myargv[1] = id;
  sprintf(type, "%d", mytype);
  myargv[2] = type;
  myargv[3] = (char *)strndup(t->name, strlen(t->name));
/*
  for (i = 0; i < myargc; i++) printf("%s ", myargv[i]);
  printf("\n");
*/

  // call the reason() function
  local = (Robot *) reason(myargc, myargv);

  if (!dll_empty(local->solutionlist)) return t;
  else return 0;
}

/*
 * query the robot's position -- the simplest way
 * could be changed later
 */
void query_info()
{
  if (myid == 56) {
    local->x = 27; local->y = 1; local->speed = 0.1;
  } else if (myid == 57) {
    local->x = 28; local->y = -1; local->speed = 0.1;
  }
  return; 
}

/*
 * evaluate the coalition cost, which is the combination of team utility
 * and the time cost of traveling to the box
 * whatever function you would like to plug in :)
 * pos = 0, evaluate orginal cost
 * pos != 0, update cost according another coalition member
 */
void eval_cost(Task *t, Bid *bid, R_pos *pos) 
{
  float tmp, dist, dist2;
  float maxtime = MAXDIST/0.5; // normalized
  float weight = 0.7;
  int numrobot = 0;
  Dllist ptr;

  dll_traverse(ptr, bid->coalition) numrobot++;

  dist = DIST(local->x, local->y, t->x, t->y);
  if (pos != 0) {
    dist2 = DIST(pos->x, pos->y, t->x, t->y);
    if (dist2 > dist) dist = dist2;
    //printf("--------------update cost\n");
  }

  bid->nav_time = dist/local->speed;
  dist = DIST(t->x, t->y, t->gx, t->gy);
  bid->push_time = dist/(local->speed);
  // if I am pushing the box alone, time will double, could be changed later
  if (numrobot == 1 && t->numrobot == 2) bid->push_time *= 3;
  tmp = bid->nav_time + bid->push_time;
  //printf("time: %.1f + %.1f = %.1f\n", bid->nav_time, bid->push_time, tmp);
  tmp = tmp/maxtime;
  bid->cost = (int) ((weight*tmp + (1-weight)*bid->utility)*1000);
}

/*
 * create a list of bids according to the solutionlist
 */
void create_bidlist(Task *t)
{
  Dllist ptr, ptr1;
  Solution *sol;
  Bid *bid;
  int id, count = 0;

  bidlist = new_dllist();
  dll_traverse(ptr, local->solutionlist) {
    sol = (Solution *) jval_v(dll_val(ptr));
    //printf("Solution %d (%.2f): ", sol->sid, sol->utility);
    bid = (Bid *) malloc(sizeof(Bid));
    bid->sid = sol->sid;
    bid->utility = sol->utility;
    bid->coalition = new_dllist();
    dll_traverse(ptr1, sol->coalition) {
      id = (int) jval_i(dll_val(ptr1));
      dll_append(bid->coalition, new_jval_i(id));
      count++;
      //printf("%d ", id);
    }
    bid->numrobot = count;
    count = 0;
    eval_cost(t, bid, 0);  // calcualte the final cost
    dll_append(bidlist, new_jval_v(bid));
  }
}

/*
 * consult other coalition members and update the bidlist
 */
void consult_other(int b_fd, int l_fd, Task *t)
{
  Message *m;
  char *msg;
  int id;
  Dllist ptr, ptr1;
  Bid *bid; 
  int st;

  int nbytes = 0;
  char mymsg[MAXBUF];
  char *token, *p;

  Dllist poslist; // list of positions
  R_pos *pos;
  
  poslist = new_dllist();

  // inform others of my current position
  m = (Message *) new_msg_auc();
  m->type = '#';
  m->t->x = local->x;
  m->t->y = local->y;
  m->leaderID = local->rid;
  msg = (char *) gen_msg_auc(m);
  talk_to_all(b_fd, msg, R); 

  st = time(0);
  while (time(0) - st < TOUT4) {// 2 seconds
    nbytes = listen_to_robot(l_fd, mymsg);
    if (nbytes <= 0) continue;
    token = strtok(mymsg, "!"); 
    while (token != NULL) {
      if (token[0] == '#') {
        // extract the new position
        pos = (R_pos *) malloc(sizeof(R_pos));
        p = strstr(token, "#"); p++;
        pos->id = atoi(p);
        p = strstr(p, "$"); p++;
        pos->x = atoi(p);
        p = strstr(p, "$"); p++;
        pos->y = atoi(p);
        dll_append(poslist, new_jval_v(pos)); 
      }
      token = strtok(NULL, "!");
    } 
  }
/*
  dll_traverse(ptr, poslist) {
    pos = (R_pos *) jval_v(dll_val(ptr));
    printf("R%d: (%d %d)\n", pos->id, pos->x, pos->y);
  }
*/
  // update coalition cost when needed
  dll_traverse(ptr1, poslist) {
    pos = (R_pos *) jval_v(dll_val(ptr1));
    if (local->x == pos->x && local->y == pos->y) continue;
    dll_traverse(ptr, bidlist) {
      bid = (Bid *) jval_v(dll_val(ptr));
      if (find_int(bid->coalition, pos->id)) {
        eval_cost(t, bid, pos);
        // update
      }
    }
  }
}

/*
 * submit bid to auctioneer
 */
void submit_bid(int fd)
{
  Dllist ptr, ptr1;
  Bid *bid;
  Message *m;
  char *msg;
  int id;

  dll_traverse(ptr, bidlist) {
    bid = (Bid *) jval_v(dll_val(ptr));
    m = (Message *) new_msg_auc();
    m->type = 'B';
    m->cost = bid->cost;
    m->numrobot = bid->numrobot;
    m->coalition = new_dllist(); 
    dll_traverse(ptr1, bid->coalition) {
      id = (int) jval_i(dll_val(ptr1));
      dll_append(m->coalition, new_jval_i(id));
    }
    msg = (char *) gen_msg_auc(m);
    talk_to_all(fd, msg, H);
    printf("submit bid: %s\n", msg);
    free(msg);
  }
}

/*
 * accept the award if coalitions match
 */
Bid *accept_award(int fd, char *token)
{
  char *p;
  Dllist ptr, ptr1, tmplist;
  int i, id, numrobot, sum1 = 0, sum2 = 0, mul1 = 1, mul2 = 1;
  Bid *bid;
  Message *m;
  char *msg;

  // extract info from the message
  //printf("%d: %s\n", time(0)-auc_st, token);
  p = strstr(token, "W");
  p++;
  numrobot = atoi(p);
  tmplist = new_dllist();
  for (i = 0; i < numrobot; i++) {
    p = strstr(p, "$");
    p++;
    id = atoi(p);
    dll_append(tmplist, new_jval_i(id));
    sum1 += id;
    mul1 *= id;
  } 
 
  // match with any coalition? 
  dll_traverse(ptr, bidlist) {
    bid = (Bid *) jval_v(dll_val(ptr));
    bid->awarded = 0; 
    dll_traverse(ptr1, bid->coalition) {
      id = (int) jval_i(dll_val(ptr1));
      sum2 += id;
      mul2 *= id;
    }
    if (sum1 == sum2 && mul1 == mul2) {
      // I am awarded this task, accept it
      bid->awarded = 1;
      m = (Message *) new_msg_auc();
      m->type = 'A';
      m->leaderID = local->rid;
      msg = (char *) gen_msg_auc(m);
      usleep(5000);
      talk_to_all(fd, msg, H);
      //talk_to_all(fd, msg, H);
      printf("%d: Accept the award: %s\n", time(0)-auc_st, msg);
      return bid; 
    } else {sum2 = 0; mul2 = 1;}
  }
  return 0;
}

/*
 * execute the task
 */
void exec_task(Task *t, Bid *b)
{
  int friend_id = 0, pos = 0, i;
  int status;
  int myargc = 8;
  char *myargv[8];
  char c_myid[10], c_friend_id[10], c_pos[10], c_goal_x[10], c_goal_y[10];

  // temporarily set up like this, needs to be fixed later
  if (myid == 56 && t->numrobot == 2) {
    friend_id = 57; // can extract from b->coalition
    pos = 0; // left pusher
  } else if (myid == 57 && t->numrobot == 2) {
    friend_id = 56;
    pos = 1; // right pusher
  }
  myargv[0] = "clear";
  myargv[1] = "laser";
  sprintf(c_pos, "%d", pos);
  myargv[2] = c_pos;
  sprintf(c_myid, "%d", myid);
  myargv[3] = c_myid;
  sprintf(c_friend_id, "%d", friend_id);
  myargv[4] = c_friend_id;
  if (t->numrobot == 2 && pos == 0)
    sprintf(c_goal_x, "%d", t->x-1);
  else sprintf(c_goal_x, "%d", t->x);
  myargv[5] = c_goal_x;
  sprintf(c_goal_y, "%d", t->y);
  myargv[6] = c_goal_y;
  myargv[7] = NULL;

  for (i = 0; i < myargc; i++)
    printf("%s ", myargv[i]);
  printf("\n");
  //sleep(30);

  if (fork() == 0) {// child process
    if (execvp("./clear", myargv) < 0) {
      perror("exec_task()");
      exit(1);
    }
    exit(1);
  }
  wait(&status);
  printf("Task Completion\n");
}

/*
 * send completion message to the monitor thread
 */
void complete(int fd, Task *t)
{
  Message *m;
  char *msg;

  m = (Message *) new_msg_auc();
  m->t = t;
  m->type = 'P';
  m->timeofarr = t->timeofarr;
  msg = (char *) gen_msg_auc(m);
  talk_to_all(fd, msg, R);
}

/*
 * robot trader
 */
void trader()
{
  Bid *b;
  Task *t;
  int free_to_neg; // 1 = free to negotiate, 0 = otherwise
  int listen_fd, b_fd, m_fd, b2_fd;
  int nbytes = 0;
  char msg[MAXBUF], msg_dummy[MAXBUF];
  char *token, *p, tmps[30];
  static int x, y;
  static float speed = -1.0;
  
  listen_fd = create_listen(PORT_H, H); // listen to the auction thread
  b_fd = create_broadcast(PORT_H, H); // broadcast to the auction thread
  b2_fd = create_broadcast(PORT_H, R); // broadcast to other robots
  m_fd = create_broadcast(PORT_M, R); // broadcast to the monitor thread

  free_to_neg = 1;
  // while (not_the_end_of_auction) wait for task
  while (1) {
    // listen to incoming task
    nbytes = listen_to_robot(listen_fd, msg);
    if (nbytes <= 0) continue;
    token = strtok(msg, "!"); 
    while (token != NULL) {
      if (token[0] == 'Z') exit(0); // end of auction
      if (token[0] == 'S') { // synchronization
        p = strtok(token, "S");
        //auc_st = atoi(p); 
	auc_st = time(0);
        //printf("auc starttime: %d, curr time: %d\n", auc_st, time(0));
        token = strtok(NULL, "!");
        continue;
      }
      if (token[0] == 'T') { // if it's a task announcement
        // start ASyMTRe reasoning to generate coalitions
        if (free_to_neg) {
          printf("%d: recv new task: %s\n", time(0)-auc_st, token);
          t = (Task *) negotiate(token);
          if (t == 0) {
            token = strtok(NULL, "!");
            free_to_neg = 1; 
            printf("%d: no solution\n", time(0)-auc_st);
            continue;
          } 
        } else {token = strtok(NULL, "!"); continue;}
        free_to_neg = 0;
        printf("%d: end of reasoning...\n", time(0)-auc_st); 
        // query my current position and speed if I have no clue
        if (speed < 0) {
          query_info();
          speed = local->speed;
          x = local->x; y = local->y;
        } else {
          local->x = x; local->y = y; local->speed = speed;
        }
        
        // create a list of bids for every coalition
        create_bidlist(t);

        // consult other coalition members, update bid if needed
        consult_other(b2_fd, listen_fd, t); 

        // submit bid(s) for all coalitions that I might be in
        submit_bid(b_fd);
      } else if (token[0] == 'W') { // if it's a winner announcement
        // accept award if it's for me
        if ((b = accept_award(b_fd, token)) == 0) {
          token = strtok(NULL, "!");
          free_to_neg = 1;
          continue;
        }

        // execute and monitor task until it is finished
        exec_task(t, b);
        /*
        printf("%d: navigating %.1f...\n", time(0)-auc_st, b->nav_time);
        sleep(b->nav_time);
        printf("%d: pushing %.1f...\n", time(0)-auc_st, b->push_time);
        sleep(b->push_time);
        */
        complete(m_fd, t);
        printf("%d: task completion\n", time(0)-auc_st);

        // update my current (x, y) position
        x = t->gx; y = t->gy;
        // ignore previous message
        while (nbytes > 0) {
          nbytes = listen_to_robot(listen_fd, msg_dummy);
          if (nbytes > 0) {
            //printf("msg_dummy: %s\n", msg_dummy);
            if (msg_dummy[0] == 'Z') exit(0);
          }
        }
        free_to_neg = 1;
      }
      token = strtok(NULL, "!");
    }
  }
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    printf("USAGE: robotrader id type\n");
    exit(1);
  }
  myid = atoi(argv[1]);
  mytype = atoi(argv[2]);
  printf("robot %d\n", myid);
  trader();
}
