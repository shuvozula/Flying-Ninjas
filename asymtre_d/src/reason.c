/*
 * reason.c -- a distributed solution configuration for multi-robot teams
 * Fang (Daisy) Tang @ DILAB
 * May, 2006
 */

#include <reason.h>

int starttime;
FILE *result;
char fn[20];

/*
 * make request for information
 */
void make_request(int fd, int opt, Dllist dll, int mid)
{
  Message *m;
  char *msg, *tmp;
  Dllist ptr;
  int nbytes, i = 0;

  m = (Message *) new_msg();
  m->type = 'I';
  m->from = local->rid;
  m->opt = opt;
  m->info = new_dllist();
  dll_traverse(ptr, dll) {
    tmp = (char *) jval_s(dll_val(ptr));
    dll_append(m->info, new_jval_s(tmp));
    i++;
  }	
  m->numinfo = i;
  m->mid = mid;

  msg = (char *) gen_msg(m);
  nbytes = talk_to_all(fd, msg, R);
}

/*
 * confirm help
 * two types of confirmation 'C' or 'E'
 */
void confirm_help(int fd, char type, int fid, int rid)
{
  Message *m;
  char *msg;
  int nbytes;

  m = (Message *) new_msg();
  m->type = type;
  m->from = fid;
  m->to = rid;
  m->mid = 0;

  msg = (char *) gen_msg(m);
  if (type == 'E')
    nbytes = talk_to_all(fd, msg, H); // from help thread to request thread
  else nbytes = talk_to_all(fd, msg, R); // from request thread to help thread
}

/*
 *  submit help
 */
void submit_help(int fd, int nid, float utility, int mid)
{
  Message *m;
  char *msg;
  int nbytes;

  m = (Message *) new_msg();
  m->type = 'H';
  m->from = local->rid;
  m->to = nid;

  // ok, the penalty comes here
  //if (local->help == 1)
    //utility += .9;

  if (utility >= 0) {
    m->utility = (int)(utility*100);
    m->opt = 2;
  } else m->opt = 1;
  m->mid = mid;

  msg = (char *) gen_msg(m);
  nbytes = talk_to_all(fd, msg, H);
  //printf("msg: %s\n", msg);
}

/*
 * insert the help to helplist[i] according to the utility
 * messages with the same mids are grouped together
 */
void insert_help(char *msg)
{
  int i = 0;
  char *ptr, *token;
  Help *h, *tmp;
  Dllist p, schema;

  token = strtok(msg, "!");	
  //printf("help: %s\n", msg);
  while (token != NULL) {
    h = (Help *) malloc(sizeof(Help));

    if (token[0] != 'H') return;
    if (token[1] != '2') {
      token = strtok(NULL, "!");
      continue;
    }
    ptr = strstr(token, "$");
    ptr++;
    h->hid = atoi(ptr);
    if (h->hid == local->rid) {
      token = strtok(NULL, "!");
      continue;
    }
    ptr = strstr(ptr, "$");
    ptr++;
    if (atoi(ptr) != local->rid) return;
    ptr = strstr(ptr, "U");
    ptr++;
    h->utility = (float)atoi(ptr)/100;
    //printf("h->utility: %.2f\n", h->utility);
    ptr = strstr(ptr, "$");
    ptr++;
    i = atoi(ptr);
    h->solution_id = i;
    // insert the helper into helplist according to decreasing utilities
    h->final_utility = 0.0;
    schema = new_dllist();
    if (check_schema(&schema, i) == 0) { // don't need to insert the help
      token = strtok(NULL, "!");
      continue;
    }
    if (dll_empty(helplist[i])) {
      dll_append(helplist[i], new_jval_v(h));
    } else {
      dll_traverse(p, helplist[i]) {
        tmp = (Help *) jval_v(dll_val(p));
        if (tmp->hid == h->hid && tmp->solution_id == h->solution_id && tmp->utility == h->utility) break;
        if (tmp->utility > h->utility) {// the best utility is at the front
          dll_insert_b(p, new_jval_v(h));
          break;
        }
        if (p == dll_last(helplist[i])) {// append it to the last
          dll_append(helplist[i], new_jval_v(h));
          break;
        } 
      }
    }
    token = strtok(NULL, "!");
  }
}

/*
 * rank the helplist into rankedlist based on updated final utility
 */
void rank_help()
{
  Dllist ptr, ptr1, schema, tmps;
  Help *h, *newh;
  char *dummy, *tmpb; // behavior
  float myu = 0.0, myfinalu = 0.0; // utility
  int i;

  // update final_utility
  schema = new_dllist();
  for (i = 0; i < maxway; i++) {
    myfinalu = 0; myu = 0;
    if (dll_empty(helplist[i])) continue;
    tmps = new_dllist();
    if (check_schema(&schema, i) == 0) continue;
    dll_traverse(ptr, schema) { // calculates self utility
      tmpb = (char *) jval_s(dll_val(ptr));
      single_cost(tmpb, &dummy, &myu);
      my_append_s(tmps, tmpb);
    }
    dll_traverse(ptr, tmps) {
      tmpb = (char *) jval_s(dll_val(ptr));
      single_cost(tmpb, &dummy, &myu);
      //printf("%s,%s: %.2f\n", tmpb, dummy, myu);
      myfinalu += myu;
    }
    dll_traverse(ptr, helplist[i]) { // combined (self+help)
      h = (Help *) jval_v(dll_val(ptr));
      if (h->final_utility > 0) continue;
      h->final_utility = h->utility + myfinalu;
      //printf("%d: help: %.2f + mine: %.2f = total: %.2f\n", i, h->utility, myfinalu, h->final_utility);
    }
  }

  // rank the solutions according to their utilities
  for (i = 0; i < maxway; i++) {
    if (dll_empty(helplist[i])) continue;
    //h = (Help *) jval_v(dll_val(dll_first(helplist[i])));
    if (dll_empty(rankedlist)) {
      dll_traverse(ptr1, helplist[i]) {
        h = (Help *) jval_v(dll_val(ptr1));
        dll_append(rankedlist, new_jval_v(h));
      }
    } else {
      dll_traverse(ptr1, helplist[i]) {
        h = (Help *) jval_v(dll_val(ptr1));
        dll_traverse(ptr, rankedlist) {
          newh = ( Help *) jval_v(dll_val(ptr));
          if (h->hid == newh->hid && h->solution_id == newh->solution_id && h->utility == newh->utility) break;
          if (newh->final_utility > h->final_utility) {
            dll_insert_b(ptr, new_jval_v(h));
            break;
          }
          if (ptr == dll_last(rankedlist)) {
            dll_append(rankedlist, new_jval_v(h));
            break;
          }
        }
      }
    }
  }
 /* 
  dll_traverse(ptr, rankedlist) {
    h = (Help *) jval_v(dll_val(ptr));
    printf("solution %d helper %d help_u %.2f utility %.2f\n", h->solution_id, h->hid, h->utility, h->final_utility);
  }*/
}

/*
 * select the ith best helper, the maximum combined utility (self+help)
 * return utility
 */
Help *select_help(int ith)
{
  Dllist ptr;
  Help *h = NULL;
  int i = 0;

  numranked = 0; 
  dll_traverse(ptr, rankedlist) 
    numranked++;
  if (ith > numranked) return h;

  // find the ith best solution
  dll_traverse(ptr, rankedlist) {
    h = (Help *) jval_v(dll_val(ptr));
    //printf("solution %d helper %d utility %.2f\n", h->solution_id, h->hid, h->final_utility);
    if (i+1 == ith) {
      h = (Help *) jval_v(dll_val(ptr));
      //printf("selects %d: %.2lf\n", h->solution_id, h->final_utility);
      break;
    }
    i++;
  }
  return h;  
}

/*
 * canIprovide: check whether I can provide the information
 * at the same time, calculates the utility and activeB
 */
float canIprovide(Dllist *ActiveB, Dllist info)
{
  Dllist ptr, ptr1, ptr2, activeb;
  char *tmp, *tmp1;
  Infonode *node;
  float u = 0;

  activeb = new_dllist();

  dll_traverse(ptr, info) {
    tmp = (char *) jval_s(dll_val(ptr));
 
    dll_traverse(ptr1, local->infolist) {
      node = (Infonode *) jval_v(dll_val(ptr1));
      if (strcmp(node->name, tmp) == 0) {
        u += node->utility;
        dll_traverse(ptr2, node->schema) {
          tmp1 = (char *) jval_s(dll_val(ptr2));
          my_append_s(activeb, tmp1);
        }
        break;
      }
    }
  }

  *ActiveB = activeb;
  return u;
}


/*
 * see if I can help someone
 */
void *help(void *v)
{
  int listen_fd, broadcast_fd;
  int nbytes = 0;
  char msg[MAXBUF];

  Dllist info; // list of needed information
  Dllist activeB, p; // list of schemas that need to be activated
  Needy *n; // the needy robot

  char *token, *ptr, *tmp;

  int cid; // confirm id
  int nid, mid; // the needy's id and the message id
  int i;

  listen_fd = create_listen(PORT_H, H);
  broadcast_fd = create_broadcast(PORT_R, H);

  tohelp = new_dllist();

  starttime = time(0); 
  while (1) {
    if (time(0) - starttime > TIMEOUT2) {
      //printf("Timeout, I am done with helping others.\n");
      break;
    }
    nbytes = listen_to_robot(listen_fd, msg);
    // local->capable: in the case of navigation?
    //if (nbytes == 0 || local->capable == 0) continue;
    if (nbytes == 0) continue;

    token = strtok(msg, "!");
    while (token != NULL) {		
      if (token[0] == 'I') { // it's about request-info
        // what information do you need? 
        what_info(&info, &nid, &mid, token);
		
        // If it's my own request, ignore it
        if (nid == local->rid) {
          token = strtok(NULL, "!");
          continue;
        }

        // initialize the new needy robot
        n = (Needy *) malloc(sizeof(Needy));
        n->nid = nid;
        n->activeB = new_dllist();

        // am I eligible to help you?
        if ((n->utility = canIprovide(&(n->activeB), info)) == 0) {
          //printf("Sorry, I can't\n");
          token = strtok(NULL, "!");
          free_dllist(n->activeB);
          free(n);			
          continue;
        }
        //printf("Yes, I can provide. n->utility = %.2f\n", n->utility);
        if (token[1] == '1') { // it's a simple request
          submit_help(broadcast_fd, nid, -1, mid);
        } else if (token[1] == '2') { // it's a complex request
          if ((Needy *) find_needy(n->nid) == NULL)
            dll_append(tohelp, new_jval_v(n));
          // there is no limitation on the number of help message
          submit_help(broadcast_fd, nid, n->utility, mid);
        }
      } else if (token[0] == 'C') { //it's a confirmation
        //printf("confirm msg: %s\n", msg);
        ptr = strstr(token, "C");
        ptr++;
        nid = atoi(ptr);
        ptr = strstr(token, "$");
        ptr++;
        cid = atoi(ptr);
        n = (Needy *) find_needy(nid);	
        if (cid == local->rid) { // it's for me
          if (n == NULL) {token = strtok(NULL, "!"); continue;}
        } else { // it's for others
          if (n != NULL) delete_needy(nid);
        } 
      } 
      token = strtok(NULL, "!");
    }
  }
  //printf("returning from help thread...\n");
  pthread_exit(0);
}

int check_list(Dllist ids)
{
  Dllist ptr1, ptr2;
  Solution *s1;
  int id, sum1 = 0, mul1 = 1, sum2 = 0, mul2 = 1;
  
  dll_traverse(ptr2, ids) {
    id = (int) jval_i(dll_val(ptr2));
    sum2 += id;
    mul2 *= id;
  }
  dll_traverse(ptr1, local->solutionlist) {
    s1 = (Solution *) jval_v(dll_val(ptr1));
    dll_traverse(ptr2, s1->coalition) {
      id = (int) jval_i(dll_val(ptr2));
      sum1 += id;
      mul1 *= id;
    }
    if (sum1 == sum2 && mul1 == mul2) return 1;
    else {sum1 = 0; mul1 = 1;}
  }
  return 0;
}


/* 
 * The request thread
 */
void *request(void *v)
{
  int listen_fd, broadcast_fd;
  int st, stc; // start time
  int nbytes = 0;
  char *token, *ptr, *tmp;

  char msg[MAXBUF];

  Dllist info, schema, p;
  Dllist tmplist;
  int i, to, from, helpid = 0, turn = 0, ith_helper = 1;
  int cond = 0;
  int ps; //potential solution
  Help *helper;
  Solution *sol, *newsol; // sol represents a solution

  float u = 0.0;
  char *dummy, *tmpb;
  FILE *fu, *ft, *f_recv; // utility file
  int mytime;
  int num_recv = 0;

  Infonode *infon;

  listen_fd = create_listen(PORT_R, R);
  broadcast_fd = create_broadcast(PORT_H, R);

  // check whether I can accomplish the task by myself
  tmplist = new_dllist();
  for (i = 0; i < maxway; i++) {
    if (check_info(&info, i) == 0) {
      if (check_schema(&schema, i) == 0) continue;
      // find a new solution 
      newsol = (Solution *) malloc(sizeof(Help));
      newsol->sid = i;
      newsol->utility = 0;
      newsol->coalition = new_dllist();
      my_append_i(newsol->coalition, local->rid);
      dll_traverse(p, schema) {
        tmpb = (char *) jval_s(dll_val(p));
        single_cost(tmpb, &dummy, &u);
        newsol->utility += u;
      }
      // insert the solution by the order of utility
      if (dll_empty(tmplist)) {
        dll_append(tmplist, new_jval_v(newsol));
      } else {
        dll_traverse(p, tmplist) {
          sol = (Solution *) jval_v(dll_val(p));
          if (sol->utility > newsol->utility) {
            dll_insert_b(p, new_jval_v(newsol));
            break;
          }
          if (p == dll_last(tmplist)) {
            dll_append(tmplist, new_jval_v(newsol));
            break;
          }
        }
      }
    }
  }
  if (!dll_empty(tmplist)) {
    sol = (Solution *) jval_v(dll_val(dll_first(tmplist)));
    dll_append(local->solutionlist, new_jval_v(sol));
    local->s_i++;
  }
  /*
  dll_traverse(p, tmplist) {
    sol = (Solution *) jval_v(dll_val(p));
    printf("Solution %d: %f\n", sol->sid, sol->utility); 
  }*/
  if (local->s_i >= MNS) pthread_exit(0);

  // check if others can help me by providing information
  starttime = time(0);
  while (1) {
    if (time(0) - starttime > TIMEOUT2) {
      //printf("ok, I am done with sending help...\n");
      break;
    }
    // make request (B)
    //printf("\nmake request...\n");

    for (i = 0; i < maxway; i++) {
      if (check_info(&info, i)) {
        if (check_schema(&schema, i)) {
          make_request(broadcast_fd, 2, info, i);
        }
      }
    }
    //printf("wait for the helpers\n");
    st = time(0);
    num_recv = 0;
    while ((time(0) - st) <= TIMEOUT) { // listen for help and rank them
      nbytes = listen_to_robot(listen_fd, msg);
      if (nbytes > 0) {
        num_recv++;
        token = strtok(msg, "!");
        while (token != NULL) {
          if (token[0] == 'H') 
            insert_help(token);
          token = strtok(NULL, "!");
        }
      }
    }
    for (i = 0; i < maxway; i++) { 
      if (!dll_empty(helplist[i])) {
        ps = i; cond = 1; 
        break; // break when there is at least one helper
      }
    }
    if (cond != 1) continue;
    rank_help();
    while (1) {
      //printf("selects the %dth best ...\n", ith_helper);
      // select and confirm the best help (B)
      helper = select_help(ith_helper);
      if (helper == NULL) {
        if (ith_helper > numranked) {
          //printf("returning from request thread...\n");
          pthread_exit(0);
        }
        break;
      } 
      //confirm_help(broadcast_fd, 'C', local->rid, helper->hid);
	    // a new solution generated
      sol = (Solution *) malloc(sizeof(Solution));
      sol->coalition = new_dllist();
      my_append_i_inc(sol->coalition, local->rid);
      my_append_i_inc(sol->coalition, helper->hid);
      sol->sid = helper->solution_id;
      sol->utility = helper->final_utility;
      if (check_list(sol->coalition) == 1) { // this coalition has been considered
        ith_helper++;
        free(sol);
        continue;
      }
      // confirm help
      confirm_help(broadcast_fd, 'C', local->rid, helper->hid);
      // append solution
      if (local->s_i < MNS) {
        dll_append(local->solutionlist, new_jval_v(sol));
        local->s_i++;
      }
      if (local->s_i >= MNS) {
        break;
      } else {// didn't get confirmation after TIMEOUT
        ith_helper++; // try to find the next available helper
      }
    } // end of while (1)
    if (local->s_i >= MNS) {
      //printf("I am ready...\n"); 
      break;
    }
  }
  //printf("returning from request thread...\n");
  pthread_exit(0);
}


/*
 * find out the robot's capability
 */
void robot_capability(int id, int type)
{
  IS robot, common;
  Dllist ptr1, ptr2;
  Atomic *a;	
  char *s, fn1[20], fn2[20];
	
  sprintf(fn1, "../config/robot%d.cfg", type);
  robot = new_inputstruct(fn1);
  if (robot == 0) {
    perror("robot.cfg");
    exit(1);
  }
  sprintf(fn2, "../config/tmpcs%d.cfg", id);
  common = new_inputstruct(fn2);
  if (common == 0) {
    perror("tmpcs.cfg");
    exit(1);
  }

  // parse robot.cfg
  robot_cfg(robot, id);

  // parse commonsense.cfg
  common_cfg(common, id);

  //printf("local behavior: ");
  dll_traverse(ptr1, local->sensors) {
    s = (char *) jval_s(dll_val(ptr1));
    dll_traverse(ptr2, Alist->behaviors) {
      a = (Atomic *) jval_v(dll_val(ptr2));
      if (strcmp(a->sensor, s) == 0) {
        my_append_s(local->behaviors, a->name);
        //printf("%s ", a->name);
      }
    }
  } 
  //printf("\n");

  gen_info();

  jettison_inputstruct(robot);
  jettison_inputstruct(common);
}

void free_all()
{
  free_dllist(Costlist);
  free(Clist);
  free(Alist);

  free(hashway);
  free(helplist);
  free_dllist(tohelp);
  free_dllist(rankedlist);
  free_dllist(Needlist);
  free_dllist(Provlist);
  free_dllist(Commlist);
  
  free(roots);
  free(dummyroots);
}

//int main(int argc, char **argv)
Robot *reason(int argc, char **argv)
{
  pthread_t t[2];
  pthread_attr_t attr[2];
  void *retval, *arg;
  int i;
  Dllist ptr, ptr1;
  Solution *sol;

  myindex = 0;
  rankedlist = new_dllist();

  if (argc != 4) {
    printf("USAGE: reason <rid> <type> <task_name>\n");
    //printf("opt = 1, write rules\n");
    exit(1);
  }
  goal = strdup(argv[3]);
  //sprintf(goal, "clear%d", atoi(argv[4]));
  //printf("goal is (%s)\n", goal);

  // 0. process information and generate solutions
  if (atoi(argv[1]) <= 0) {
    printf("rid should be > 0\n");
    exit(1);
  }
  // 1. intialize local robot
  new_robot(atoi(argv[1]));
  
  // 2. process the information configuration file
  infoproc(1, atoi(argv[1]), atoi(argv[2]));

  // 3. find out the robot's capability
  robot_capability(atoi(argv[1]), atoi(argv[2]));

  // initiate helplist
  helplist = (Dllist *) malloc(sizeof(Dllist) * maxway);
  for (i = 0; i < maxway; i++) {
    helplist[i] = new_dllist();
  }

  for (i = 0; i < 2; i++) {
    pthread_attr_init(attr+i);
    pthread_attr_setscope(attr+i, PTHREAD_SCOPE_SYSTEM);
    if (i == 0) pthread_create(t+i, attr+i, request, (void *)arg);
    else pthread_create(t+i, attr+i, help, (void*)arg);
  }

  for (i = 0; i < 2; i++)	
    pthread_join(t[i], &retval);

  /*
  dll_traverse(ptr, local->solutionlist) {
    sol = (Solution *) jval_v(dll_val(ptr));
    printf("Solution %d (%.2f): ", sol->sid, sol->utility);
    dll_traverse(ptr1, sol->coalition)
      printf("%d ", (int) jval_i(dll_val(ptr1)));
    printf("\n");
  }
  */
  free_all();
  //printf("returning from reason()...\n");
  return local;
}
