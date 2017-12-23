#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>   
#include <unistd.h>  
#include <iostream>
#include <fstream>
#include <errno.h>
#include <opencv2/opencv.hpp>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>


/******************prod_cons*****************/
#define QUEUESIZE 2
#define LOOP 5


/******************prod_cons*****************/
void *producer (void *args);
void *consumer (void *args);


/******************prod_cons*****************/
typedef struct {
	int buf[QUEUESIZE];
	long head, tail;
	int full, empty;
	pthread_mutex_t *mut;
	pthread_cond_t *notFull, *notEmpty;
} queue;


/******************prod_cons*****************/
queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, int in);
void queueDel (queue *q, int *out);
void millisleep(int milliseconds);

void *lowprior_thread(void *arg);
void *highprior_thread(void *arg);


/******************color to grey*****************/
int c2g(int);
int c2n(int);
int c2s(int);



/******************thread priorities*****************/

pthread_t hpt;
pthread_t lpt;
queue *fifo;


struct sched_param my_param;
pthread_attr_t lp_attr;
int policy;
pthread_attr_t hp_attr;



//pthread_barrier_t mybarrier;

using namespace std;
using namespace cv;



int send_image(int socket)
{
	FILE *picture;
	int size, read_size, stat, packet_index;
	char send_buffer[10240], read_buffer[256];
	packet_index = 1;

	// char buff[50];
	// sprintf(buff,"client_%d.jpeg",socket);
	char buffserv[50];
	//printf("c2g value of d: %d", d);
//	sprintf(buffserv,"server_echo_%d.jpeg",socket);
	sprintf(buffserv,"server_%d.jpeg",socket);

	//picture = fopen(buff, "r");
	picture = fopen(buffserv, "r");
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

	//Send Picture as Byte Array
	printf("Sending Picture as Byte Array\n");

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
        printf("Socket Value: %d\n",socket);
		


		packet_index++;  

		//Zero out our send buffer
		bzero(send_buffer, sizeof(send_buffer));
   	}
}



int receive_image(int socket)
{ // Start function 

	int buffersize = 0, recv_size = 0,size = 0, trans =0, read_size, write_size, packet_index =1,stat;

	char imagearray[10241],verify = '1';
	FILE *image;

	//Find the size of the image
	do{
		stat = read(socket, &size, sizeof(int));
	}while(stat<0);

	//Find the size of the image
	do{
		stat = read(socket, &trans, sizeof(int));
	}while(stat<0);


	printf("Packet received.\n");
	printf("Packet size: %i\n",stat);
	printf("Image size: %i\n",size);
	printf("Transformation: %i\n",trans);
	printf(" \n");

	char buffer[] = "Got it";

	//Send our verification signal
	do{
		stat = write(socket, &buffer, sizeof(int));
	}while(stat<0);

	printf("Reply sent\n");
	printf(" \n");

	char buff[50];
	sprintf(buff,"client_echo_%d.jpeg",socket);
	image = fopen(buff, "w");

	if( image == NULL) {
	      printf("Error has occurred. Image file could not be opened\n");
	      return -1;
	}

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
	switch(trans){
			case 1: 
					c2g(socket);
					break;
			case 2: 
					c2n(socket);
					break;
			case 3: 
					c2s(socket);
					break;
			default:
					c2g(socket);
		}


	fclose(image);
	printf("Image successfully Received!\n");
	return 1;
}



/******************color to grey*****************/
int c2g(int d)
{
// char* imageName = argv[1];
	char buff[50];
	//printf("c2g value of d: %d", d);
	sprintf(buff,"client_echo_%d.jpeg",d);
	
	Mat image;
	image = imread( buff, 1 );
//	image = imread( "clientecho.jpeg", 1 );

//	printf("In C2G");
	if( !image.data )
	{
		printf( "No image data \n " );
		return -1;
	}

	Mat gray_image;
	cvtColor( image, gray_image, COLOR_BGR2GRAY );

	char buffserv[50];
	//printf("c2g value of d: %d", d);
	sprintf(buffserv,"server_%d.jpeg",d);
	
	imwrite( buffserv, gray_image);

	namedWindow(  buff, WINDOW_AUTOSIZE );
	namedWindow( buffserv, WINDOW_AUTOSIZE );

	imshow( "Client image", image );
	imshow( "Gray image", gray_image );

//	imclose( image );
//	imclose( "Gray image", gray_image );

	waitKey(0);

	return 0;
}


/******************color to negative*****************/
int c2n(int d)
{
	char buff[50];
	//printf("c2g value of d: %d", d);
	sprintf(buff,"client_echo_%d.jpeg",d);
	Mat image;
	image = imread( buff, 1 );

    // initialize the output matrix with zeros
    Mat neg_image = Mat::zeros( image.size(), image.type() );
 
    // create a matrix with all elements equal to 255 for subtraction
    Mat sub_mat = Mat::ones(image.size(), image.type())*255;
 
    //subtract the original matrix by sub_mat to give the negative output neg_image
    subtract(sub_mat, image, neg_image);
 
     // Create Windows
//     namedWindow("Original Image", 1);
//     namedWindow("New Image", 1);
  // 	Mat neg_image;

 	char buffserv[50];
	//printf("c2g value of d: %d", d);
	sprintf(buffserv,"server_%d.jpeg",d);
	
	imwrite( buffserv, neg_image);

     // Show stuff
//     imshow("Original Image", image);
//     imshow("New Image", new_image);
 
     // Wait until user press some key
     waitKey();
     return 0;
}

/******************color to smooth*****************/
int c2s( int d)
{
	char buff[50];
	//printf("c2g value of d: %d", d);
	sprintf(buff,"client_echo_%d.jpeg",d);

	Mat image;
	image = imread( buff, 1 );

	 if( !image.data )
	 {
	   printf( " No image data \n " );
	   return -1;
	 }

	 Mat smooth_image;
	 //cvtColor( image, output_image, COLOR_BGR2GRAY );
	 for ( int i = 1; i < 10; i = i + 2 ){
	 	GaussianBlur( image, smooth_image, Size( i, i ), 0, 0 );
	 }

 	char buffserv[50];
	//printf("c2g value of d: %d", d);
	sprintf(buffserv,"server_%d.jpeg",d);
	imwrite(buffserv, smooth_image );

	 // namedWindow( imageName, WINDOW_AUTOSIZE );
	 // namedWindow( "Smooth image", WINDOW_AUTOSIZE );

	 // imshow( imageName, image );
	 // imshow( "Smooth image", output_image );

	 waitKey(0);

	 return 0;
}




/******************prod_cons*****************/
void *producer (void *q)
{

	int soc_client , soc_server , c, read_size,buffer = 0;
	struct sockaddr_in server , client;
	char *readin;
	queue *fifo;
	int min_priority;
/******************thread_priorities*****************/


/******************prod_cons*****************/
	fifo = (queue *)q;

	if (fifo ==  NULL) {
		fprintf (stderr, "main: Queue Init failed.\n");
		exit (1);
	}


	//Create socket
	soc_client = socket(AF_INET , SOCK_STREAM , 0);
	if (soc_client == -1){
		printf("Could not create socket");
	}

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8889 );

	//Bind
	if( bind(soc_client,(struct sockaddr *)&server , sizeof(server)) < 0){
		puts("bind failed");
		return 1;
	}

	puts("bind done");

	//Listen
	listen(soc_client , 20);

	my_param.sched_priority = sched_get_priority_min(SCHED_FIFO);
	min_priority = my_param.sched_priority;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &my_param);
	pthread_getschedparam (pthread_self(), &policy, &my_param);
	
	pthread_attr_init(&hp_attr);
	pthread_attr_setinheritsched(&hp_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&hp_attr, SCHED_FIFO);

	my_param.sched_priority = min_priority;
	min_priority++;
	pthread_attr_setschedparam(&hp_attr, &my_param);

	pthread_create(&hpt, &hp_attr, highprior_thread, NULL);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);



	while(1){
		
		if((soc_server = accept(soc_client, (struct sockaddr *)&client,(socklen_t*)&c))){
			puts("Connection accepted");
//			printf("Server Priority: %d \n", min_priority);
		}
	
		fflush(stdout);
	
		if (soc_server<0){
			perror("Accept Failed");
			return 1;
		}

		pthread_mutex_lock (fifo->mut);
		while (fifo->full) {
			printf ("producer: queue FULL.\n");
			pthread_cond_wait (fifo->notFull, fifo->mut);
		}

		queueAdd (fifo, soc_server);
		pthread_mutex_unlock (fifo->mut);
		pthread_cond_signal (fifo->notEmpty);
		millisleep (200);
	}

	return (NULL);
}





/******************prod_cons*****************/
void *consumer (void *q)
{
//	queue *fifo;
	int d;
	int min_priority;

	fifo = (queue *)q;

	my_param.sched_priority = sched_get_priority_min(SCHED_FIFO);
	min_priority = my_param.sched_priority;
//	min_priority++;

	pthread_setschedparam(pthread_self(), SCHED_FIFO, &my_param);
	pthread_getschedparam (pthread_self(), &policy, &my_param);
	
	pthread_attr_init(&lp_attr);
	pthread_attr_setinheritsched(&lp_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&lp_attr, SCHED_FIFO);

	my_param.sched_priority = min_priority + 1;
	pthread_attr_setschedparam(&lp_attr, &my_param);

	pthread_create(&lpt, &lp_attr, lowprior_thread, NULL);


	while(1){
//		printf("fifo full status: %d\n", fifo -> full);
		pthread_mutex_lock (fifo->mut);
		while (fifo->empty) {
			printf ("consumer: queue EMPTY.\n");
			pthread_cond_wait (fifo->notEmpty, fifo->mut);
		}
		queueDel (fifo, &d);
		pthread_mutex_unlock (fifo->mut);
		pthread_cond_signal (fifo->notFull);
		printf("Client priority: %d \n", min_priority);
		printf ("consumer: received %d.\n", d);
		millisleep(10000);
		receive_image(d);
		send_image(d);
		close(d);
	}

	return (NULL);
}






/******************prod_cons*****************/
queue *queueInit (void)
{
	queue *q;

	q = (queue *)malloc (sizeof (queue));
	if (q == NULL) return (NULL);

	q->empty = 1;
	q->full = 0;
	q->head = 0;
	q->tail = 0;
	q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init (q->mut, NULL);
	q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	pthread_cond_init (q->notFull, NULL);
	q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	pthread_cond_init (q->notEmpty, NULL);
	
	return (q);
}





/******************prod_cons*****************/
void queueDelete (queue *q)
{
	pthread_mutex_destroy (q->mut);
	free (q->mut);	
	pthread_cond_destroy (q->notFull);
	free (q->notFull);
	pthread_cond_destroy (q->notEmpty);
	free (q->notEmpty);
	free (q);
}






/******************prod_cons*****************/
void queueAdd (queue *q, int in)
{
	q->buf[q->tail] = in;
	q->tail++;
	if (q->tail == QUEUESIZE)
		q->tail = 0;
	if (q->tail == q->head)
		q->full = 1;
	q->empty = 0;

	return;
}





/******************prod_cons*****************/
void queueDel (queue *q, int *out)
{
	*out = q->buf[q->head];

	q->head++;
	if (q->head == QUEUESIZE)
		q->head = 0;
	if (q->head == q->tail)
		q->empty = 1;
	q->full = 0;

	return;
}





/******************prod_cons*****************/
void millisleep(int milliseconds)
{
      usleep(milliseconds * 1000);
}



void *highprior_thread(void *arg)
{
	pthread_t thread_id = pthread_self();
	struct sched_param param;
	int priority, policy, ret;

//	pthread_barrier_wait(&mybarrier);

	ret = pthread_getschedparam (thread_id, &policy, &param);
	priority = param.sched_priority;	
	// printf("Server priority = %d \n", priority);


	return NULL;
}


void *lowprior_thread(void *arg)
{

	pthread_t thread_id = pthread_self();
	struct sched_param param;
	int priority, policy, ret;

//	pthread_barrier_wait(&mybarrier);

	ret = pthread_getschedparam (thread_id, &policy, &param);
	priority = param.sched_priority;	
	// printf("Client priority = %d \n", priority);

	return NULL;
}



int main(int argc , char *argv[])
{

/******************prod_cons*****************/
	pthread_t pro, con;
//	queue *fifo;

	fifo = queueInit ();

	pthread_create (&pro, NULL, producer, fifo);
	pthread_create (&con, NULL, consumer, fifo);
	pthread_join (pro, NULL);
//	pthread_join (con, NULL);
	queueDelete (fifo);

	fflush(stdout);
	return 0;
}

