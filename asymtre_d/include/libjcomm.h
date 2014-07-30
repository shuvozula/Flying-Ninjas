/** \file libjcomm.h
 * Contains function prototypes and constants for libjcomm.
 */

#ifndef _JCOMM_
#define _JCOMM_

/* Header files */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

/* Return codes */
#define JC_SUCCESS          0
#define JC_NOTSUPPORTED     1
#define JC_BADHOST          2
#define JC_BADSOCKET        3
#define JC_NOBIND           4
#define JC_NOCONNECT        5
#define JC_NOWRITE          6
#define JC_NOREAD           7

/* Datatypes */
typedef enum {IPV4,IPV6} SocketType;

/**
 * Creates a socket to accept incoming connections.
 * Calls socket(), bind(), and listen().
 */
extern int create_server_socket(const char *hn,int port,SocketType type,int *sock);

/**
 * Accepts connections on a socket.
 * Calls accept().
 */
extern int accept_connection(int listen,int *sock);

/**
 * Creates a socket to connect to a server.
 * Calls socket() and connect().
 */
extern int create_client_socket(const char *hn,int port,SocketType type,int *sock);

/**
 * Writes data to a socket. Handles incomplete writes intelligently.
 * Calls write().
 */
extern int write_socket(int sock,void *data,int len);

/**
 * Reads data from a socket. Handles incomplete reads intelligently.
 * Calls read().
 */
extern int read_socket(int sock,void *data,int len);

#endif
