#include <auctioneer.h>

/*********************** write Log file  ***********************/
/*
 * write "t: string" to the specified file
 */
void write_file(char *filen, int t, char *s)
{
  FILE *result;
  result = fopen(filen, "a");
  if (result == 0) {
    printf("Cannot open %s to write\n", filen);
    exit(1);
  }
  fprintf(result, "%d: %s\n", t, s);
  fclose(result);
}

// generate a new message
Message *new_msg_auc()
{
  Message *m;

  m = (Message *) malloc(sizeof(Message));
  m->type = -1;
	m->numrobot = -1;
  m->leaderID = -1;
  m->cost = -1;
  m->coalition = new_dllist();
  m->timeofarr = -1;
  m->t = (Task *) malloc(sizeof(Task));

  return m;
}

// generate the message according to the struct Message
char *gen_msg_auc(Message *m)
{
  Dllist ptr;

  char *msg = NULL;
  char *char_numrobot = NULL;
  char *char_leaderID = NULL;
  char *char_cost = NULL;
  char *char_x = NULL;
  char *char_y = NULL;
  char *char_ori = NULL;
  char *char_gx = NULL;
  char *char_gy = NULL;
  char *char_timeofarr = NULL;
  char *tmp = NULL;

  msg = (char *) malloc(MSG_SIZE*sizeof(char));
  char_numrobot = (char *) malloc(INT_SIZE*sizeof(char));
  char_leaderID = (char *) malloc(INT_SIZE*sizeof(char));
	char_cost = (char *) malloc(INT_SIZE*sizeof(char));
  char_x = (char *) malloc(INT_SIZE*sizeof(char));
  char_y = (char *) malloc(INT_SIZE*sizeof(char));
  char_ori = (char *) malloc(INT_SIZE*sizeof(char));
  char_gx = (char *) malloc(INT_SIZE*sizeof(char));
  char_gy = (char *) malloc(INT_SIZE*sizeof(char));
  char_timeofarr = (char *) malloc(INT_SIZE*sizeof(char));
  tmp = (char *) malloc(INT_SIZE*sizeof(char));

  itoa(m->numrobot, char_numrobot);
  itoa(m->leaderID, char_leaderID);
	itoa(m->cost, char_cost);
  itoa(m->t->x, char_x);
  itoa(m->t->y, char_y);
  itoa(m->t->ori, char_ori);
  itoa(m->t->gx, char_gx);
  itoa(m->t->gy, char_gy);
  itoa(m->t->timeofarr, char_timeofarr);

  if (m->type == 'T') { // task announcement
    strcpy(msg, "T");
    strcat(msg, "$");
    strcat(msg, m->t->name);
    strcat(msg, "$");
		strcat(msg, char_x);
		strcat(msg, "$");
    strcat(msg, char_y);
    strcat(msg, "$");
    strcat(msg, char_ori); 
    strcat(msg, "$");
    strcat(msg, char_numrobot);
    strcat(msg, "$");
    strcat(msg, char_timeofarr);
    strcat(msg, "$");
    strcat(msg, char_gx);
    strcat(msg, "$");
    strcat(msg, char_gy);
  } else if (m->type == 'B') { // bid submission
    strcpy(msg, "B");
    strcat(msg, char_cost);
    strcat(msg, "$");
    strcat(msg, char_numrobot);
    dll_traverse(ptr, m->coalition) {
      itoa((int) jval_i(dll_val(ptr)), tmp);
      strcat(msg, "$");
      strcat(msg, tmp);
    }
  } else if (m->type == 'W') { // winner
    strcpy(msg, "W");
    strcat(msg, char_numrobot);
    dll_traverse(ptr, m->coalition) {
      itoa((int) jval_i(dll_val(ptr)), tmp);
      strcat(msg, "$");
      strcat(msg, tmp);
    }
  } else if (m->type == 'A') { // award
    strcpy(msg, "A");
    strcat(msg, char_leaderID);
  } else if (m->type == 'P') { // completion
    strcpy(msg, "P");
    strcat(msg, char_timeofarr);
  } else if (m->type == '#') {// my position
    strcpy(msg, "#");
    strcat(msg, char_leaderID);
    strcat(msg, "$");
    strcat(msg, char_x);
    strcat(msg, "$");
    strcat(msg, char_y);
  }
  strcat(msg, "!");
  strcat(msg, "\0");
  //printf("create msg: \"%s\"\n", msg);

  free(char_leaderID);
  free(char_cost);
  free(char_x);
  free(char_y);
  free(char_ori);
  free(char_numrobot);

  return msg;
}
