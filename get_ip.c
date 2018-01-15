#include <stdio.h>
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

void *addID(void *id){
    int *id_ptr = (int *)id;
    *id_ptr += 100;
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
        printf ("this scripts for print ip information of processors\n");
        for (int q = 1; q < comm_sz; q++ )
        {
            MPI_Recv(ip_info, MAX_STRING, MPI_CHAR, q, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%s\n",ip_info);
        }

    }
    if ( my_rank > 0 )
    {
        int fd;
        struct ifreq ifr;
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        /* I want to get an IPv4 IP address */
        ifr.ifr_addr.sa_family = AF_INET;
        /* I want IP address attached to "eth0" */
        strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
        ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
        pthread_t inc_x_thread;
        int id = my_rank;
        if(pthread_create(&inc_x_thread, NULL, addID, &id)) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
        if(pthread_join(inc_x_thread, NULL)) {
            fprintf(stderr, "Error joining thread\n");
            return 2;
        }
        char addNum[5];
        sprintf(addNum, "%d", id);
        /* send result */
        char *result = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
        strcat(result, addNum);
        MPI_Send (result, MAX_STRING, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

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

