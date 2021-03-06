#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> /* for strncpy */
#include "mpi.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>

const int MAX_STRING = 10000;
const int BUFFER_SIZE = 500;
void *addPort(void *id){
    int *id_ptr = (int *)id;
    *id_ptr += 10000;
    return NULL; 
}
int main (int argc, char *argv[])
{
    int comm_sz; //Number of processes
    int my_rank; //my process rank
    char ip_info[MAX_STRING];

    MPI_Init ( &argc, &argv );
    MPI_Comm_size ( MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank ( MPI_COMM_WORLD, &my_rank);
    if ( my_rank == 0 )
    {
        int fd;
        struct ifreq ifr;
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        /* I want to get an IPv4 IP address */
        ifr.ifr_addr.sa_family = AF_INET;
        /* I want IP address attached to "eth0" */
        strncpy(ifr.ifr_name, "enp130s0f0", IFNAMSIZ-1);
        ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
        char *IPaddress = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
        char masterConfig[MAX_STRING];
        int id = my_rank;
        sprintf(masterConfig, "%d,", id);
        strcat(masterConfig, IPaddress);
        int port = 10000;
        char portStr [10];
        sprintf(portStr, ":%d", port);
        strcat(masterConfig, portStr);
        printf ("this scripts for print ip information of processors\n");
        //Receives the Worker IP and fills it in config file
        FILE *f = fopen("config.txt", "w");
        if (f == NULL)
        {
            printf("Error opening config file!\n");
            exit(1);
        }
        fprintf(f, "%s\n", masterConfig);
        for (int q = 1; q < comm_sz; q++ )
        {
            MPI_Recv(ip_info, MAX_STRING, MPI_CHAR, q, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            fprintf(f, "%s\n", ip_info);   
        }
        fclose(f);
        //Use popen to start master process and catch output in log file
        char logName [50];
        sprintf(logName, "%d.log", id);
        FILE *fl = fopen(logName, "w");
        if (fl == NULL)
        {
            printf("Error opening log file!\n");
            exit(1);
        }
        FILE *fp;
        int status;
        char buffer[BUFFER_SIZE];
        fp = popen("./master", "r");
        if(fp == NULL)
        {
            printf("popen error\n");
            exit(1);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        while (fgets(buffer, BUFFER_SIZE, fp) != NULL)
            fprintf(fl, "%s", buffer);

    }
    if ( my_rank > 0 )
    {
        int fd;
        struct ifreq ifr;
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        /* I want to get an IPv4 IP address */
        ifr.ifr_addr.sa_family = AF_INET;
        /* I want IP address attached to "eth0" */
        strncpy(ifr.ifr_name, "enp130s0f0", IFNAMSIZ-1);
        ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
        //pthread_t inc_x_thread;
        int id = my_rank;
        /*if(pthread_create(&inc_x_thread, NULL, addID, &id)) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
        if(pthread_join(inc_x_thread, NULL)) {
            fprintf(stderr, "Error joining thread\n");
            return 2;
        }*/
        char workerConfig[MAX_STRING];
        sprintf(workerConfig, "%d,", id);
        /* send result */
        char *IPaddress = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
        strcat(workerConfig, IPaddress);
        int port = my_rank + 10000;
        char portStr [10];
        sprintf(portStr, ":%d", port);
        strcat(workerConfig, portStr);
        MPI_Send (workerConfig, MAX_STRING, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        //use popen() to start worker process and catch output in the log
        char logName [50];
        sprintf(logName, "%d.log", id);
        FILE *fl = fopen(logName, "w");
        if (fl == NULL)
        {
            printf("Error opening log file!\n");
            exit(1);
        }
        FILE *fp;
        int status;
        char buffer[BUFFER_SIZE];
        char command [20];
        sprintf(command, "./worker2 %d %d", id,  comm_sz - 1);
        fp = popen(command, "r");
        if(fp == NULL)
        {
            printf("popen error\n");
            exit(1);
        }
        while (fgets(buffer, BUFFER_SIZE, fp) != NULL)
            fprintf(fl, "%s", buffer);
    }

/*
 *  *   Terminate MPI.
 *   *   */
     MPI_Finalize ( );
/*
 *  *   Terminate
 *   *   */
    if ( my_rank == 0 )
    {
        printf ( "\n" );
        printf ( "Normal end of execution: job finish!\n" );
        printf ( "\n" );
    }
    return 0;
}

