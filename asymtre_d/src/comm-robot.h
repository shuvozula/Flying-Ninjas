/********************************************************************
 * Send message by opening socket
 *******************************************************************/
#include "clear.h"
int send_msg(int sockfd, char *msg, int id)
{
  int nbytes;
  char ip_addr[16];
  socklen_t addr_len;
  
  addr_len = sizeof(struct sockaddr);
  
  sprintf(ip_addr, "10.26.210.%d", id);
  remote_addr.sin_addr.s_addr = inet_addr(ip_addr);
  
  nbytes = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
  
  if (nbytes > 0) {
    //printf("Message is %s of size%d\n",msg,nbytes);
  } else
    perror("send_msg");

  return nbytes;
}

/********************************************************************
 * Open the UDP socket for sending messages
 *******************************************************************/

int open_socket()
{
  int fd;
  int yes = 1;
  
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  /* to avoid the "address already in use" error */
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(COMMPORT);
  local_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(local_addr.sin_zero), '\0', 8);
  
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = htons(COMMPORT);
  memset(&(remote_addr.sin_zero), '\0', 8);
  /* we'll set the address later */
  
  return fd;
}

void bind_socket(int sockfd, int port)
{
  struct sockaddr_in my_addr;

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(port);
  my_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(my_addr.sin_zero), '\0', 8);

  if (bind(sockfd, (struct sockaddr *)&my_addr, 
           sizeof(struct sockaddr)) == -1) {
    perror("bind");
    exit(1);
  }
}

/********************************************************************/
int recv_msg(int sockfd, char *msg)
{
  int nbytes;
  
  nbytes = recv(sockfd, msg, MAXSIZE - 1, 0);

  msg[nbytes] = '\0';
  
  if (nbytes > 0) { 
    //printf("Message is %s of size%d\n",msg,nbytes);
    return nbytes;
  } else return 0;
}
