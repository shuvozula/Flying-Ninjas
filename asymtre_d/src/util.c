/*
 * my_append
 * append str to the list only when str is not in the list 
 */
#include <reason.h>

/*********************   Dllist related  **************************/

// append a string only if it's not in the list yet
void my_append_s(Dllist l, char *s)
{
    char *t;
    Dllist ptr;

    if (!dll_empty(l)) {
      dll_traverse(ptr, l) {
        t = (char *)jval_s(dll_val(ptr));
        if (strcmp(s, t) == 0)
            return;
      }
    }
    dll_append(l, new_jval_s(strdup(s)));
}

// append an integer only if it's not in the list yet
int my_append_i(Dllist l, int i)
{
    int t;
    Dllist ptr;

    dll_traverse(ptr, l) {
        t = (int) jval_i(dll_val(ptr));
        if (t == i)
            return 1;
    }
    dll_append(l, new_jval_i(i));
    return 0;
}

int my_append_i_inc(Dllist l, int i)
{
  int t;
  Dllist ptr;

  if (dll_empty(l)) {
    dll_append(l, new_jval_i(i));
    return 0;
  }
  dll_traverse(ptr, l) {
    t = (int) jval_i(dll_val(ptr));
    if (t == i) return 1;
    if (t > i) {
      dll_insert_b(ptr, new_jval_i(i));
      break;
    }
    if (ptr == dll_last(l))
      dll_append(l, new_jval_i(i));
  }
  return 0;
}

// append a method only if it's not in the list yet
int my_append_w(Dllist l, Way *w)
{
    Way *tmp;
    Dllist ptr;

    dll_traverse(ptr, l) {
        tmp = (Way *) jval_v(dll_val(ptr));
        if ((tmp->rid == w->rid) && (strcmp(tmp->activeB, w->activeB) == 0))
            return 1;
    }
    dll_append(l, new_jval_v(w));
    return 0;
}

// append an infonode if it's not in the list yet
int my_append_info(Dllist l, Infonode *in)
{
	Infonode *tmp, *newnode;
	Dllist ptr;
	char *s;

	dll_traverse(ptr, l) {
		tmp = (Infonode *) jval_v(dll_val(ptr));
		if (strcmp(tmp->output, in->output) == 0) 
			return 1;
	}
	newnode = (Infonode *) malloc(sizeof(Infonode));
	newnode->output = (char *) malloc(sizeof(char)*strlen(in->output));
	newnode->output = strdup(in->output);
	newnode->utility = in->utility;
	newnode->schema = new_dllist();
	dll_traverse(ptr, in->schema) {
		s = (char *) jval_s(dll_val(ptr));
		dll_append(newnode->schema, new_jval_s(s));
	}	
	dll_append(l, new_jval_v(newnode));
	//printf("added(%s %f)\n", newnode->output, newnode->utility);
	return 0;
}

// return a pointer to the node with str as its value
Dllist find_ptr(Dllist l, char *str)
{
    Dllist ptr;
    char *s;

    dll_traverse(ptr, l) {
        s = (char *) jval_s(dll_val(ptr));
        if (strcmp(str, s) == 0)
            return ptr;
    }
    return 0;
}

int find_int(Dllist l, int i)
{
  Dllist ptr;
  int tmp;

  dll_traverse(ptr, l) {
    tmp = (int) jval_i(dll_val(ptr));
    if (tmp == i) return 1;
  }
  return 0;
}


/*********************  Message related **************************/

// generate a new message
Message *new_msg()
{
  Message *m;

  m = (Message *) malloc(sizeof(Message));
  m->type = -1;
  m->from = -1;
	m->numinfo = -1;
	m->to = -1;
  m->opt = -1;
  m->timer = -1;
  m->utility = -1;
	m->mid = -1;
  m->info = new_dllist();

  return m;
}

// generate the message according to the struct Message
char *gen_msg(Message *m)
{
  Dllist ptr;

  char *msg = NULL;
  char *char_opt = NULL;
  char *char_from = NULL;
	char *char_to = NULL;
	char *char_numinfo = NULL;
  char *char_utility = NULL;
  char *char_timer = NULL;
	char *char_mid = NULL;
  char *tmp = NULL;

  msg = (char *) malloc(MSG_SIZE*sizeof(char));
  char_opt = (char *) malloc(INT_SIZE*sizeof(char));
  char_from = (char *) malloc(INT_SIZE*sizeof(char));
	char_to = (char *) malloc(INT_SIZE*sizeof(char));
	char_numinfo = (char *) malloc(INT_SIZE*sizeof(char));
  char_utility = (char *) malloc(INT_SIZE*sizeof(char));
  char_timer = (char *) malloc(INT_SIZE*sizeof(char));
	char_mid = (char *) malloc(INT_SIZE*sizeof(char));

  itoa(m->opt, char_opt);
  itoa(m->from, char_from);
	itoa(m->to, char_to);
	itoa(m->numinfo, char_numinfo);	
  itoa(m->utility, char_utility);
	itoa(m->mid, char_mid);

  if (m->type == 'I') {
    strcpy(msg, "I");
    strcat(msg, char_opt);
    strcat(msg, "$");
    strcat(msg, char_from);
    strcat(msg, "$");
		strcat(msg, char_numinfo);
		strcat(msg, "$");
    if (dll_empty(m->info)) {
      //printf("request nothing, why bother?\n");
      exit(1);
    }
    dll_traverse(ptr, m->info) {
      tmp = (char *) jval_v(dll_val(ptr));
    	//printf("%s\n", tmp);
      strcat(msg, strdup(tmp));
      strcat(msg, "$");
    }
    if (m->opt == 2) {
	    //strcat(msg, "T");
      //itoa(time(0), char_timer);
      //strcat(msg, char_timer);
			//strcat(msg, "$");
    }
    strcat(msg, char_mid);
  } else if (m->type == 'H') {
    strcpy(msg, "H");
    strcat(msg, char_opt);
    strcat(msg, "$");
    strcat(msg, char_from);
    strcat(msg, "$");
    strcat(msg, char_to);
		strcat(msg, "$");
    if (m->opt == 2) {
      strcat(msg, "U");
      strcat(msg, char_utility);
      strcat(msg, "$");
    }
    strcat(msg, char_mid);
  } else if (m->type == 'C') {
    strcpy(msg, "C");
    strcat(msg, char_from);
    strcat(msg, "$");
    strcat(msg, char_to);
  } else if (m->type == 'E') {
    strcpy(msg, "E");
    strcat(msg, char_from);
    strcat(msg, "$");
    strcat(msg, char_to);
  } 
  strcat(msg, "!");
  strcat(msg, "\0");
  //printf("create msg: \"%s\"\n", msg);

  free(char_opt);
  free(char_from);
  free(char_utility);
  free(char_timer);
  return msg;
}

/********************  Utility related  ***************************/

// return the pure cost of a sensor
int cost(char *s)
{
    Dllist ptr;
    Cost *c;

    dll_traverse(ptr, Costlist) {
        c = (Cost *) jval_v(dll_val(ptr));
        if (strcmp(c->sensor, s) == 0)
            return c->cost;
    }
    return -1;
}

// return the probability of a SCS
float prob(char *schema, char *sensor)
{
    Atomic *a;
    Dllist ptr;

    dll_traverse(ptr, Alist->behaviors) {
        a = (Atomic *) jval_v(dll_val(ptr));
        if ((strcmp(schema, a->name) == 0) && (strcmp(sensor, a->sensor) == 0))
            return a->prob;
    }
    return -1;
}

// can sensor be used to implement certain atomic behavior?
// return 1 if yes. 
int CanSensor(char *sensor, char *atomic)
{
    Dllist ptr1, ptr2;
    Atomic *a;
    char *s;

    dll_traverse(ptr1, Alist->behaviors) {
        a = (Atomic *) jval_v(dll_val(ptr1));
        if (strcmp(atomic, a->name) == 0) {
            if (strcmp(sensor, a->sensor) == 0)
                return 1;
        }
    }
    return 0;
}

// single_cost: return the utility of using a certain sensor to
//              implement a behavior on the local robot
// finalcost = cost*weight + prob.failure*(1-weight)
// weight = .5
void single_cost(char *behavior, char **sensor, float *u)
{
    Dllist ptr;
    char *s;
    float c, p, tmpcost = 1.0;
    float finalcost = 0.0;

    dll_traverse(ptr, local->sensors) {
        s = (char *) jval_s(dll_val(ptr));
        if (CanSensor(s, behavior) > 0) {
            p = prob(behavior, s);
            c = (float)cost(s)/MAXCOST;
            finalcost = .5*c + (1-.5)*(1-p);
            if (tmpcost > finalcost) {
                tmpcost = finalcost;
                *sensor = strdup(s);
            }
        }
    }
	*u = tmpcost;
}


/**********************  Solution related  ********************/

/* find different ways to accomplish the goal
void find_ways()
{
    Dllist ptr1, ptr2;
    Composite *c;
    Atomic *a;

    dll_traverse(ptr1, Clist->behaviors) {
        c = (Composite *) jval_v(dll_val(ptr1));
        if (strcmp(Goal, c->name) != 0)
            continue;
        dll_traverse(ptr2, c->blist) {
            a = (Atomic *) jval_v(dll_val(ptr2));
            if (!invalid_composite(a->name))
                find_ways(a->name);
            else dll_append(hashway[a->way], new_jval_v(a));
        }
    }
}*/

// check_info: check out what types of information are needed
// return 1 if there's needed information	
int check_info(Dllist *info, int i) 
{
	Dllist ptr, tmp, tmp1;
	Atomic *a;

  Dllist p, p1;
  char *next;
  Infonode *in;
	int find = 0;

	tmp = new_dllist();
	tmp1 = new_dllist();
	
	dll_traverse(ptr, hashway[i]) {
		a = (Atomic *) jval_v(dll_val(ptr));
		if (a->type == 0) {
			//printf("%s\n", a->name);
		} else {
			if (!invalid_info(a->name)) {
				//printf("(%s)\n", a->name);
				dll_append(tmp1, new_jval_s(a->name));
				dll_traverse(p, Commlist) {
					in = (Infonode *) jval_v(dll_val(p));
					if (strcmp(in->output, a->name) == 0) {
						//printf("[%s]\n", in->output);	
						dll_append(tmp, new_jval_s(in->input));
						find = 1;
						break;
					}
				}	
				if (!find)
					dll_append(tmp, new_jval_s(a->name));
				find = 0;
			}	
		}
	}
	*info = tmp;
	if (dll_empty(tmp)) return 0;
	else return 1;
}

// is the robot capable of certain behavior?
int capableof(char *behavior)
{
  Dllist ptr;
  char *s;

  dll_traverse(ptr, local->behaviors) {
    s = (char *) jval_s(dll_val(ptr));
    if (strcmp(s, behavior) == 0) return 1;
  }
  return 0;
}

// check_schema: check out what types of schemas are needed
int check_schema(Dllist *schema, int i)
{
  Dllist ptr, tmp;
  Atomic *a;

  tmp = new_dllist();

  dll_traverse(ptr, hashway[i]) {
    a = (Atomic *) jval_v(dll_val(ptr));
    if (a->type == 0) {
      //printf("-->%s\n", a->name);
      if (capableof(a->name))
	      dll_append(tmp, new_jval_s(a->name));
      else {
        //printf("%d is not capable of %s\n", local->rid, a->name);
        return 0;
      }
    } else {
      //printf("(%s)\n", a->name);
			if (invalid_info(a->name))	
       	dll_append(tmp, new_jval_s(a->name));
    }
  }
  *schema = tmp;
  if (dll_empty(tmp)) return 0;
  else return 1;
}


/******************  Information related  ***************************/

// what_info: what information is asked by the needy
// return the message id
void what_info(Dllist *info, int *needyid, int *msgid, char *msg)
{
	int nid, mid; // the needy's id and the message id
	int numinfo, i = 0;
	char tmpi[15]; 
	char *ptr;
	Dllist tmpinfo;

	tmpinfo = new_dllist();

	ptr = strstr(msg, "$");
	ptr++;
	nid = atoi(ptr);
	//printf("needy=%d\n", nid);

	ptr = strstr(ptr, "$");	
	ptr++;
	numinfo = atoi(ptr);
	//printf("numinfo: %d\n", numinfo);

	while (i < numinfo) {
		ptr = strstr(ptr, "f");
		ptr++;
		sprintf(tmpi, "f%d", atoi(ptr));
		//printf("-->%s\n", tmpi);
		dll_append(tmpinfo, new_jval_s(strdup(tmpi)));
		i++;
	}
		
	ptr = strstr(ptr, "$");
	ptr++;
	mid = atoi(ptr);

	*info = tmpinfo;
	*needyid = nid;	
	*msgid = mid;
}


/****************************************************************/

/*
 * find the needy robot in tohelp with nid
 */
Needy *find_needy(int nid)
{
	Dllist ptr;
	Needy *n;

	dll_traverse(ptr, tohelp) {
		n = (Needy *) jval_v(dll_val(ptr));
		if (n->nid == nid) 
			return n;
	}
	return NULL;
}	

/*
 * delete the needy in tohelp with nid
 */
void delete_needy(int nid)
{
	Dllist ptr;
	Needy *n;

	dll_traverse(ptr, tohelp) {
		n = (Needy *) jval_v(dll_val(ptr));
		if (n->nid == nid) {
			dll_delete_node(ptr);
			break;
		}
	}
}

