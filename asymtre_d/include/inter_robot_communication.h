



void talk_to_robot(char message_type, char to_robot_id, char mesg[]);
char *listen_to_robot(void);
void init_communication(int argc, char *argv[]); 







void init_communication(int argc, char *argv[])
{
	/*
	* Process Command Line ==> inter-robot-comm
	*/
for ( i = 1; i < argc; i++ )
  {
  if ( strcmp( argv[i], "-portc" ) == 0 )
    port = GetPort( argv[ ++i ] );

  else if ( strcmp( argv[i], "-serverc" ) == 0 )
    server = argv[ ++i ];

  else if ( strcmp( argv[i], "-countc" ) == 0 )
    count = atoi( argv[ ++i ] );

  }
/* ROBOT_NAME = argv[argc - 2][0];*/

/**/
 CHAR_ROBOT_NAME = argv[argc - 1][0];
  /**/

connect_robot_id = atoi(argv[argc - 1]);
 printf("\nCHAR_ROBOT_NAME = %c; connect_robot_id = %d",
	CHAR_ROBOT_NAME, connect_robot_id);

	/*
	* Create Socket
	*/
if ( (sfd_c = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
  perror( "(client)  error creating socket" );

	/*
	* Retrieve Server Host Address
	*/
else if ( (hp = gethostbyname( server )) == NULL )
  {
  fprintf(
      stderr,
      "(client)  error getting server host info for '%s'\n", server
      );
  }
else
  {
  bzero( (char *) &local_addr, sizeof(local_addr) );
  bzero( (char *) &server_addr, sizeof(server_addr) );
  server_addr.sin_addr = *((struct in_addr *) hp->h_addr_list[0]);
  server_addr.sin_family = hp->h_addrtype;
  server_addr.sin_port = htons( port );

		/*
		* Bind Local Address To Socket
		*/
  if (
      bind( sfd_c, (struct sockaddr *) &local_addr, sizeof(local_addr) )
      < 0
      )
    perror( "(client)  error binding address to socket" );

		/*
		* Connect to Server
		*/
  else if (
      connect( sfd_c, (struct sockaddr *)&server_addr, sizeof(server_addr) ) < 0
      )
	{
    	 perror( "(client)  connect error" );
	 exit(1);
	}
}
write(sfd_c,&CHAR_ROBOT_NAME,1); /* tell server which robot this is */


}

char *listen_to_robot(void)
    {
      char		finished;
      char *		buffer = NULL;
      fd_set		the_mask;
      fd_set		read_mask;

			/*
			* We break on input from the socket or standard input
			*/
    FD_ZERO( &the_mask );
    FD_SET( 0, &the_mask );
    FD_SET( sfd_c, &the_mask );

      read_mask = the_mask;

      switch( select( getdtablesize(), &read_mask, NULL, NULL, &poll_time ) )

        {
        case -1:
          perror( "(client)  select() error" );
          break;

        case 0:
	  /* printf( "Timeout waiting for input...\n" );*/
          break;

        default:
			/* Check Socket First */
	  if ( FD_ISSET( sfd_c, &read_mask ) )
	    {
	      int		byte_count;

	      /*            ioctl( sfd_c, FIONREAD, &byte_count );*/
	        ioctl( sfd_c, I_NREAD, &byte_count );

/*  puts("listening .... "); */

	    if ( byte_count == 0 )
		finished = 1;

	    else if ( (buffer = (char *)malloc( byte_count + 1 )) == NULL )
	      printf("allocation failed\n" );

	    else
	      {
	      read( sfd_c, buffer, byte_count );
	      buffer[ byte_count ] = 0;
 	     printf( "[Received==>]\n%s\n", buffer );

	      /* free( buffer );*/
	      /*puts("   ");*/

	      }
	    } /* if socket active */

} /* switch */
return(buffer);
}

void talk_to_robot(char msg_type, char to_robot_id, char mesg[])
    {
      char		finished;
      fd_set		the_mask;
      fd_set		read_mask;
      char		sendthis[strlen(mesg)+3];


			/*
			* We break on input from the socket or standard input
			*/
    FD_ZERO( &the_mask );
    FD_SET( 0, &the_mask );
    FD_SET( sfd_c, &the_mask );

      	   if ( FD_ISSET( 0, &the_mask ) )
	    {
	      int		length;
	      int		v;
  
	      sendthis[0] = msg_type;
	      sendthis[1] = to_robot_id;
	      for(v=0;v<strlen(mesg);v++)
		{
		  sendthis[v+2]=mesg[v];
		}
 		 
	      sendthis[v+2] = '\0';
	      length = strlen( sendthis );
	      /* printf("sending .... -> %s\n",sendthis);*/
	      write( sfd_c, sendthis, length);
	      
	    }
 }

