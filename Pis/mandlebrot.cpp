#include <mpi.h>
#include <stdio.h>
#include <complex.h>
#include <omp.h>
#include <string.h>
#include "bitmap_image.hpp"
using namespace std;


const int width = 500;
const int height = 500;

const float MinRe = -2.0;
const float MaxRe = 1;
const float MinIm = -1;
const float MaxIm = 1;

unsigned char mandlebrot( int x, int y, int m);


int main(int argc, char *argv[]) {
    
    // if( argc != 2)
    // {
    //     cout<< argc <<endl;
    //     cout<< "Pass max iterations (int) as argument" <<endl;
    //     return -1;
    // }

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    //var to hold mpi receive status
    MPI_Status status;

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    if( world_size > 4)
    {
        cout<< "No more than 3 worker nodes allowed" <<endl;
        return -1;
    }

    // Get the rank of the process
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //int max_it =  atoi(argv[1]);
    int max_it = 30;

    int node01_end;
    int node02_end;

    switch( world_size ) //Partitions work for each worker node
    {
        case 2:
        node01_end = width;
        break;

        case 3:
        node01_end = width/2;
        node02_end = width;
        break;

        case 4:
        node01_end = width/3;
        node02_end = 2*node01_end;
        break;
    }

    //instructions for master
    if( rank == 0 )
    {
        bitmap_image img( width, height );
        vector<unsigned char> receive_buffer;

        if( world_size >= 2)
        {
            receive_buffer.resize(node01_end * height);

            MPI_Recv(&receive_buffer[0], node01_end * height, MPI::UNSIGNED_CHAR, 1, 0, MPI_COMM_WORLD, &status);

            for( int i = 0; i < node01_end; i++)
            {
                for( int j = 0; j < height; j++)
                {
                    img.set_pixel(i, j, receive_buffer.front(), 0, 0);
                    receive_buffer.erase(receive_buffer.begin());
                }
                if( i % 1000 == 0)
                    cout<< i <<"/"<< (node01_end * height) <<endl;
            }
        }
        

        if( world_size >= 3)
        {
            receive_buffer.resize(( node02_end - node01_end) * height);
            MPI_Recv(&receive_buffer[0], ( node02_end - node01_end) * height, MPI::UNSIGNED_CHAR, 2, 1, MPI_COMM_WORLD, &status);

            for( int i = node01_end; i < node02_end ; i++)
            {
                for( int j = 0; j < height; j++)
                {
                    img.set_pixel(i, j, receive_buffer.front(), 0, 0);
                    receive_buffer.erase(receive_buffer.begin());
                }
            }
        }

        if( world_size == 4)
        {
            receive_buffer.resize(( width - node02_end) * height);
            MPI_Recv(&receive_buffer[0], ( width - node02_end) * height, MPI::UNSIGNED_CHAR, 3, 2, MPI_COMM_WORLD, &status);

            for( int i = node02_end; i < width ; i++)
            {
                for( int j = 0; j < height; j++)
                {
                    img.set_pixel(i, j, receive_buffer.front(), 0, 0);
                    receive_buffer.erase(receive_buffer.begin());
                }
            }
        }

        img.save_image("frac.bmp");
        cout<< "Image saved" <<endl;
    }

    //instructions for node01
    if( rank == 1)
    {
        vector<unsigned char> buf;
        buf.resize(node01_end * height);
        omp_set_dynamic(0); //allows exact setting of number of threads rather than max threads
        omp_set_num_threads(4); //sets exact num threads to thread per node argument

        #pragma omp parallel \
        shared (buf) \

        #pragma omp for
        for( int i = 0; i < node01_end; i++)
        {
            for( int j = 0; j < height; j++ )
            {
                buf[(i*width)+j]=( mandlebrot( i, j, max_it) );
            }
        }

        MPI_Send(&buf[0], buf.size(), MPI::UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    //instructions for node02
    if( rank == 2)
    {
        vector<unsigned char> buf;
        buf.resize(( node02_end - node01_end) * height);
        omp_set_dynamic(0); //allows exact setting of number of threads rather than max threads
        omp_set_num_threads(4); //sets exact num threads to thread per node argument
        #pragma omp parallel \
        shared (buf, max_it) \

        #pragma omp for
        for( int i = node01_end; i < node02_end ; i++)
        {
            for( int j = 0; j < height; j++ )
            {
                buf[((i*width)+j) - node01_end]=( mandlebrot( i, j, max_it) );
            }
        }
        MPI_Send(&buf[0], ( node02_end - node01_end) * height, MPI::UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD);
    }

    //instructions for node03
    if( rank == 3)
    {
        vector<unsigned char> buf;
        buf.resize(( width - node02_end) * height);
        omp_set_dynamic(0); //allows exact setting of number of threads rather than max threads
        omp_set_num_threads(4); //sets exact num threads to thread per node argument
        #pragma omp parallel \
        shared (buf, max_it) \

        #pragma omp for
        for( int i = node02_end; i < width ; i++)
        {
            for( int j = 0; j < height; j++ )
            {
                buf[((i*width)+j) - node02_end]=( mandlebrot( i, j, max_it) );
            }
        }
        MPI_Send(&buf[0], ( width - node02_end) * height, MPI::UNSIGNED_CHAR, 0, 2, MPI_COMM_WORLD);
    }
    // Finalize the MPI environment.
    
    cout<< "I made it! :" <<rank <<endl;
    MPI_Finalize();
}

unsigned char mandlebrot( int x, int y, int max_it )
{
    complex<float> point(
                        x*( (MaxRe-MinRe)/width ) + MinRe ,
                        y*( (MaxIm-MinIm)/height ) + MinIm );
                        
    complex<float> z(0,0);
    int iterations = 0;
    while( abs(z) < 2 && iterations < max_it ) {
        z = z*z + point;
        iterations++;
    }
    if( iterations < max_it ) 
        return 255 * iterations/max_it;
    else
        return 0;
}
