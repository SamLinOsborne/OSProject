#include <mpi.h>
#include <stdio.h>
#include <complex.h>
#include <omp.h>
#include <string.h>
#include <ctime>
#include "bitmap_image.hpp"
using namespace std;



const float MinRe = -2.0;
const float MaxRe = 1;
const float MinIm = -1;
const float MaxIm = 1;

unsigned char mandlebrot( int x, int y, int m, int d);


int main(int argc, char *argv[]) {
    
    //verifies correct number of arguments are passed
    if( argc != 4) 
    {
        cout<< argc <<endl;
        cout<< "Pass as arguments: max iterations (int), threads per node (int), image square dimension (int) " <<endl;
        return -1;
    }

    cout<< "Work started" <<endl;

    //converts arguments to integers for use
    int max_it = atoi( argv[1] );
    int threads_per_node = atoi( argv[2] );
    int width = atoi( argv[3] );
    int height = atoi( argv[3] );

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

    clock_t start_time = clock();

    //instructions for master
    if( rank == 0 )
    {
        cout<< "Rank 0 work started" <<endl;
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

        clock_t end_time = clock();
        double time_elapsed = double( end_time - start_time ) / CLOCKS_PER_SEC;
        img.save_image("frac.bmp");
        cout<< "Image saved" <<endl;

        ofstream records;

        records.open( "test_recordings.csv", ios::app);
        records << world_size << "," << threads_per_node << "," << max_it << "," << width << "," << time_elapsed << "\n";
    }

    //instructions for node01
    if( rank == 1)
    {
        cout<< "Rank 1 work started" <<endl;
        vector<unsigned char> buf;
        buf.resize(node01_end * height);
        omp_set_dynamic(0); //allows exact setting of number of threads rather than max threads
        omp_set_num_threads(threads_per_node); //sets exact num threads to thread per node argument

        #pragma omp parallel \
        shared (buf) \

        #pragma omp for
        for( int i = 0; i < node01_end; i++)
        {
            for( int j = 0; j < height; j++ )
            {
                buf[(i*width)+j]=( mandlebrot( i, j, max_it, width) );
            }
        }

        MPI_Send(&buf[0], buf.size(), MPI::UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    //instructions for node02
    if( rank == 2)
    {
        cout<< "Rank 2 work started" <<endl;
        vector<unsigned char> buf;
        buf.resize(( node02_end - node01_end) * height);
        omp_set_dynamic(0); //allows exact setting of number of threads rather than max threads
        omp_set_num_threads(threads_per_node); //sets exact num threads to thread per node argument
        #pragma omp parallel \
        shared (buf) \

        #pragma omp for
        for( int i = node01_end; i < node02_end ; i++)
        {
            for( int j = 0; j < height; j++ )
            {
                buf[((i - node01_end)*width)+j]=( mandlebrot( i, j, max_it, width) );
            }
        }
        MPI_Send(&buf[0], ( node02_end - node01_end) * height, MPI::UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD);
    }

    //instructions for node03
    if( rank == 3)
    {
        cout<< "Rank 3 work started" <<endl;
        vector<unsigned char> buf;
        buf.resize(( width - node02_end) * height);
        omp_set_dynamic(0); //allows exact setting of number of threads rather than max threads
        omp_set_num_threads(threads_per_node); //sets exact num threads to thread per node argument
        #pragma omp parallel \
        shared (buf) \

        #pragma omp for
        for( int i = node02_end; i < width ; i++)
        {
            for( int j = 0; j < height; j++ )
            {
                buf[((i - node02_end)*width)+j]=( mandlebrot( i, j, max_it, width) );
            }
        }
        MPI_Send(&buf[0], ( width - node02_end) * height, MPI::UNSIGNED_CHAR, 0, 2, MPI_COMM_WORLD);
    }
    // Finalize the MPI environment.
    
    cout<< "I made it! :" <<rank <<endl;
    MPI_Finalize();
}

unsigned char mandlebrot( int x, int y, int max_it, int square_dim )
{
    complex<float> point(
                        x*( (MaxRe-MinRe)/square_dim ) + MinRe ,
                        y*( (MaxIm-MinIm)/square_dim ) + MinIm );
                        
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
