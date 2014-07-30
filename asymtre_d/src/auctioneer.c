/*
 * auctioneer.c -- an auctioneer program
 * Fang (Daisy) Tang @ DILAB
 * May, 2006
 */

#include <auctioneer.h>

char fn[20] = "rec-auc.txt"; // output file that records auction process

/*
 * task announcement
 */
void announce_task(int fd, Task *t)
{
  Message *m;
  char *msg;

  m = (Message *) new_msg_auc();
  m->type = 'T';
  m->t = t; 
  m->numrobot = t->numrobot;

  msg = (char *) gen_msg_auc(m);
  talk_to_all(fd, msg, R);
  printf("%d: Task Announcement: %s\n", time(0)-auc_st, msg);
  write_file(fn, time(0)-auc_st, msg);
}

/*
 * insert bid in descending cost
 */
void insert_bid(char *msg)
{
  char *p;
  Bid *b, *tb;
  int i, id;
  Dllist ptr;

  // create a new bid
  b = (Bid *) malloc(sizeof(Bid));
  p = strstr(msg, "B");
  p++;
  b->cost = atoi(p);
  p = strstr(p, "$");
  p++;
  b->numrobot = atoi(p);
  b->coalition = new_dllist();
  for (i = 0; i < b->numrobot; i++) {
    p = strstr(p, "$");
    p++;
    id = atoi(p);
    if (dll_empty(b->coalition)) b->leaderID = id;
    dll_append(b->coalition, new_jval_i(id)); 
  }

  printf("insert bid: %s\n", msg);
  // insert the bid to bidList, increasing cost
  if (dll_empty(bidList)) {
    dll_append(bidList, new_jval_v(b));
  } else {
    dll_traverse(ptr, bidList) {
      tb = (Bid *) jval_v(dll_val(ptr));
      if (tb->cost > b->cost) {
        dll_insert_b(ptr, new_jval_v(b));
        break;
      }
      if (ptr == dll_last(bidList)) {// append it to the last
        dll_append(bidList, new_jval_v(b));
        break;
      } 
    }
  }
}

/*
 * select the ith best bid
 */
Bid *select_bid(int ith)
{
  Bid *b;
  Dllist ptr;
  int i = 1, numbid = 0;

  dll_traverse(ptr, bidList) numbid++;
  if (ith > numbid) return 0;
  
  dll_traverse(ptr, bidList) {
    if (i == ith) {
      b = (Bid *) jval_v(dll_val(ptr));
      return b;
    }
    i++;
  }
}

/*
 * inform the coalition leader of the winner bid
 */
void inform_winner(int fd, Bid *b)
{
  Message *m;
  char *msg;
  Dllist ptr;

  m = (Message *) new_msg_auc();
  m->type = 'W';
  m->numrobot = b->numrobot;
  m->coalition = new_dllist();
  dll_traverse(ptr, b->coalition) {
    dll_append(m->coalition, new_jval_i(jval_i(dll_val(ptr))));
  }
  msg = (char *) gen_msg_auc(m);
  talk_to_all(fd, msg, R);
  printf("%d: Winner: %s\n", time(0)-auc_st, msg);
}


/*
 * monitor_t thread: monitors the execution of the task
 */
void *monitor_t(void *v)
{
  int listen_fd;
  int nbytes = 0;
  char msg[MAXBUF];
  char *token, *p, tmps[30];
  Task *tt;
  int timeofarr = 0;
  Dllist ptr;

  completionList = new_dllist();
  listen_fd = create_listen(PORT_H, H);
  while (1) {
    if (endofauc && dll_empty(monitorList)) break;

    nbytes = listen_to_robot(listen_fd, msg);
    if (nbytes <= 0) continue;
    token = strtok(msg, "!");
    while (token != NULL) {
      if (token[0] == 'P') {
        p = strstr(token, "P");
        p++;
        timeofarr = atoi(p);

        // look up task t in the monitorList
        dll_traverse(ptr, monitorList) {
          tt = (Task *) jval_v(dll_val(ptr));
          if (tt->timeofarr == timeofarr) {
            tt->timeofcomp = time(0) - auc_st;
            printf("%d: %s completes\n", tt->timeofcomp, tt->name);
            sprintf(tmps, "%s(%d) completes at %d", tt->name, tt->timeofarr, tt->timeofcomp);
            write_file(fn, time(0)-auc_st, tmps);
            dll_delete_node(ptr);
            dll_append(completionList, new_jval_v(tt));
            break;
          }
        }
      } 
      token = strtok(NULL, "!");
    }
  }
  printf("END OF MONITORING\n");
}

/*
 * the make_auction thread: makes an auction on the first task in the task_list
 */
void *make_auction(void *v)
{
  int listen_fd, b_fd; // broadcast fd and point-to-point fd
  int nbytes = 0;
  char msg[MAXBUF];
  char *token, *p, tmps[40], charid[10], *char_coalition;

  Dllist first, ptr, tmplist;
  Task *t;
  int currt, currt2; // current time
  Bid *ith_bid;
  int id, ith; // the ith bid

  listen_fd = create_listen(PORT_R, R);
  b_fd = create_broadcast(PORT_H, R);

  monitorList = new_dllist();
  // start with the first task in the list
  while (1) {
    if (endoftask && dll_empty(taskList)) break;
    // start with the first task in the list
    if (dll_empty(taskList)) continue;
    first = dll_first(taskList);
    t = (Task *) jval_v(dll_val(first));

    // broadcast task to the robots
    announce_task(b_fd, t);

    // collect bids
    printf("wait for incoming bids...\n");
    bidList = new_dllist();
    currt = time(0);
    while (time(0) - currt <= TOUT1) { // 20 seconds
      // listen to incoming bids 
      nbytes = listen_to_robot(listen_fd, msg);
      if (nbytes <= 0) continue;

      // insert bid(s) to a sorted array, based on increasing cost
      token = strtok(msg, "!");
      while (token != NULL) {
        if (token[0] == 'B') // it's a bid
          insert_bid(token);
        token = strtok(NULL, "!");
      }
    }
    if (dll_empty(bidList)) {sleep(0); continue;}
    printf("selects the best bid...\n");
    ith = 1;
    currt = time(0);
    while (time(0) - currt <= TOUT2) { // 10 seconds
      // select the best bid of the rest of the bid
      ith_bid = (Bid *) select_bid(ith);
      ith++;
      if (ith_bid == 0) break;

      // copy coalition to tmplist
      tmplist = new_dllist();
      char_coalition = (char*) malloc(sizeof(char)*50);
      dll_traverse(ptr, ith_bid->coalition) {
        dll_append(tmplist, new_jval_i((int)jval_i(dll_val(ptr))));
        sprintf(charid, "%d ", (int)jval_i(dll_val(ptr)));
        strcat(char_coalition, charid);
      }
      strcat(char_coalition, "\0");

      // inform the coalition bidder
      inform_winner(b_fd, ith_bid); 
   
      // wait for confirmation from all team members
      currt2 = time(0);
      while (time(0) - currt2 <= TOUT3) { // 5 seconds
        nbytes = listen_to_robot(listen_fd, msg);
        if (nbytes <= 0) continue;
        token = strtok(msg, "!"); 
        while (token != NULL) {
          if (token[0] == 'A') {
            p = strstr(token, "A");
            p++;
            id = atoi(p);
            printf("%d: Bid accepted by %d\n", time(0)-auc_st, id);
            dll_traverse(ptr, tmplist) {
              if (id == (int) jval_i(dll_val(ptr))) {
                dll_delete_node(ptr);
                break;
              }
            }
            t->timeofacc = time(0) - auc_st;
          }
          token = strtok(NULL, "!");
        }
        // insert task to the monitorList and break
        if (dll_empty(tmplist)) {
          sprintf(tmps, "task (%d) accepted by %s", t->timeofarr, char_coalition);
          write_file(fn, time(0)-auc_st, tmps);
          printf("append current task to monitorList\n"); 
          dll_append(monitorList, new_jval_v(t));
          break;
        }
      }
      if (dll_empty(tmplist)) break;
    }
    // delete current task
    dll_delete_node(first); 
    
    // if task cannot be allocated, reinsert it to the end of the array?
    if (!dll_empty(tmplist)) dll_append(taskList, new_jval_v(t));
  }
  endofauc = 1;
  printf("END OF AUCTION\n");
}

/*
 * the new_task thread: insert a new task to the task_list
 */
void *new_task(void *v)
{
  IS is;
  Task *task;
  char tmps[30];
  // initiate taskList
  taskList = new_dllist();
  // read from standard input
  is = new_inputstruct(0);
  if (is == 0) {
    perror("standard input");
    exit(1);
  }
  printf("Site Clearing Task Format: task_name x y orientation number_of_robot goal_x goal_y\n"); 
  printf("e.g., clear1 5 10 270 1 10 10\n");
  while (get_line(is) >= 0) {
    if (is->NF != 7) {
      printf("Wrong task format, please input again:\n");
      continue;
    }
    // creat a new task
    task = (Task *) malloc(sizeof(Task));
    task->name = strdup(is->fields[0]);
    task->x = atoi(is->fields[1]);
    task->y = atoi(is->fields[2]);
    task->ori = atoi(is->fields[3]);
    task->numrobot = atoi(is->fields[4]);
    task->gx = atoi(is->fields[5]);
    task->gy = atoi(is->fields[6]);
    task->timeofarr = time(0) - auc_st;
    task->timeofacc = -1;
    task->timeofcomp = -1;
    // insert the task into the taskList
    dll_append(taskList, new_jval_v(task));
    sprintf(tmps, "new: %s$%d", task->name, task->timeofarr);
    write_file(fn, time(0)-auc_st, tmps);
  }
  endoftask = 1; // set the end of auction signal
  jettison_inputstruct(is);
}

int main(int argc, char **argv)
{
  pthread_t th[3];
  pthread_attr_t attr[3];
  void *retval, *arg;
  int i;
  
  Dllist ptr;
  Task *t;

  FILE *f;

  char msg_time[20];
  int b_fd;
  b_fd = create_broadcast(PORT_H, R);

  if (argc != 1) {
    printf("USAGE: auctioneer\n");
    exit(1);
  }

  // intialize everything... hopefully :)
  f = fopen(fn, "w");
  if (f == 0) {
    printf("fail to open %s to write...\n", fn);
    exit(1);
  }
  endoftask = 0;
  endofauc = 0;
  // synchronize timing
  auc_st = time(0); // start counting...
  sprintf(msg_time, "S%d!", auc_st);
  talk_to_all(b_fd, msg_time, R);

  // thread time ...
  for (i = 0; i < 3; i++) {
    pthread_attr_init(attr+i);
    pthread_attr_setscope(attr+i, PTHREAD_SCOPE_SYSTEM);
    if (i == 0) pthread_create(th+i, attr+i, new_task, (void *)arg);
    else if (i == 1) pthread_create(th+i, attr+i, make_auction, (void*)arg);
    else pthread_create(th+i, attr+i, monitor_t, (void *)arg);
  }
  for (i = 0; i < 3; i++)	
    pthread_join(th[i], &retval);

  // print out the list of completed tasks at the end
  dll_traverse(ptr, completionList) {
    t = (Task *) jval_v(dll_val(ptr));
    printf("%s %d %d %d %d %d %d: (%d - %d - %d)\n", t->name, t->x, t->y, t->ori, t->numrobot, t->gx, t->gy, t->timeofarr, t->timeofacc, t->timeofcomp);
  } 
  talk_to_all(b_fd, "Z", R); // broadcast end of auction
}
