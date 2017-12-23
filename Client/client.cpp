#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>   
#include <sys/ioctl.h>
#include <unistd.h>  
#include <iostream>
#include <fstream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>   
#include <unistd.h>  
#include <opencv2/opencv.hpp>
#include <pthread.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>



using namespace std;

//This function is to be used once we have confirmed that an image is to be sent
//It should read and output an image file



int receive_image(int socket)
{ // Start function 

      int buffersize = 0, recv_size = 0,size = 0, read_size, write_size, packet_index =1,stat;
      int d;
      char imagearray[10241],verify = '1';
      FILE *image;
      d = socket;

      //Find the size of the image
      do{
	      stat = read(socket, &size, sizeof(int));
      }while(stat<0);

//      printf("Inside receive_image!!!!!!!\n");
      printf("Packet received.\n");
      printf("Packet size: %i\n",stat);
      printf("Image size: %i\n",size);
//      printf("Inside receive_image Socket: %d\n",socket);
      printf(" \n");

      char buffer[] = "Got it";

      //Send our verification signal
      do{
	      stat = write(socket, &buffer, sizeof(int));
      }while(stat<0);

      printf("Reply sent\n");
      printf(" \n");
 
	  // char buff[50];
	  // sprintf(buff,"client_%d.jpeg",socket);

  // 	 char buffserv[50];
	 // //printf("c2g value of d: %d", d);
	 // sprintf(buffserv,"server_echo_%d.jpeg",d);
      image = fopen("server_echo.jpeg", "w");
	 
   //    cv::Mat show_image;
	  // show_image = cv::imread(image, 1);
   //    cv::namedWindow("Server_Echo", 0x00000001);
	  // cv::imshow("Server_Echo", show_image);

      if( image == NULL) {
	      printf("Error has occurred. Image file could not be opened\n");
      return -1; }

      //Loop while we have not received the entire file yet


      int need_exit = 0;
      struct timeval timeout = {10,0};

      fd_set fds;
      int buffer_fd, buffer_out;

      while(recv_size < size) {
      //while(packet_index < 2){

		FD_ZERO(&fds);
	      	FD_SET(socket,&fds);

		buffer_fd = select(FD_SETSIZE,&fds,NULL,NULL,&timeout);

		if (buffer_fd < 0)
			printf("error: bad file descriptor set.\n");

		if (buffer_fd == 0)
			printf("error: buffer read timeout expired.\n");

		if (buffer_fd > 0)
		{
			do{
				read_size = read(socket,imagearray, 10241);
			}while(read_size <0);

	        printf("Packet number received: %i\n",packet_index);
        	printf("Packet size: %i\n",read_size);

			printf("Socket Value: %d\n",socket);

        	//Write the currently read data into our image file
        	write_size = fwrite(imagearray,1,read_size, image);
       		printf("Written image size: %i\n",write_size); 

       		if(read_size !=write_size) {
       			printf("error in read write\n");    
			}


        		//Increment the total number of bytes read
        		recv_size += read_size;
        		packet_index++;
        		printf("Total received image size: %i\n",recv_size);
        		printf(" \n");
        		printf(" \n");
      		}

	}


	fclose(image);
	printf("Image successfully Received!\n");
	return 1;
}


int send_image(int socket, char* image, int trans)
{
	FILE *picture;
	int size, read_size, stat, packet_index;
	char send_buffer[10240], read_buffer[256];
	packet_index = 1;

	// char buff[50];
	// sprintf(buff,"client_%d.jpeg",socket);


	picture = fopen(image, "r");
	printf("Getting Picture Size\n");   

	if(picture == NULL) {
		printf("Error Opening Image File"); } 

	fseek(picture, 0, SEEK_END);
	size = ftell(picture);
	fseek(picture, 0, SEEK_SET);
	printf("Total Picture size: %i\n",size);

	//Send Picture Size
	printf("Sending Picture Size\n");
	write(socket, (void *)&size, sizeof(int));

	//Send Transform Number
	printf("Sending Transformation: %d\n", trans);
	write(socket, (void *)&trans, sizeof(int));///////////////////

	do { //Read while we get errors that are due to signals.
		stat=read(socket, &read_buffer , 255);
		printf("Bytes read: %i\n",stat);
	} while (stat < 0);

	printf("Received data in socket\n");
	printf("Socket data: %c\n", read_buffer);

	while(!feof(picture)) {
		//while(packet_index = 1){
		//Read from the file into our send buffer
		read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, picture);

		//Send data through our socket 

		do{
			stat = write(socket, send_buffer, read_size);  
		}while (stat < 0);

		printf("Packet Number: %i\n",packet_index);
		printf("Packet Size Sent: %i\n",read_size);     
		printf(" \n");
		printf(" \n");


		packet_index++;  

		//Zero out our send buffer
		bzero(send_buffer, sizeof(send_buffer));
   	}
}


int main(int argc , char **argv)
{

	int soc_client;
	struct sockaddr_in server;
	char *parray;
	char *image;
	int trans;


	image = argv[1];
	trans = atoi(argv[2]);



	//Create socket
	soc_client = socket(AF_INET , SOCK_STREAM , 0);

	if (soc_client == -1) {
		printf("Could not create socket");
	}

	memset(&server,0,sizeof(server));
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 8889 );

	//Connect to remote server
	if (connect(soc_client , (struct sockaddr *)&server , sizeof(server)) < 0) {
		cout<<strerror(errno);
		close(soc_client);
		puts("Connect Error");
		return 1;
	}

	puts("Connected\n");
	printf("Socket First value: %d",soc_client);
	send_image(soc_client, image, trans);

	receive_image(soc_client);

	close(soc_client);

	return 0;
}

