#include <iostream>
#include <fstream>
#include <complex>
#include <omp.h>
#include "bitmap_image.hpp"

using namespace std;

const int width = 900;
const int height = 900;
float MinRe = -2.0;
float MaxRe = 1;
float MinIm = -1;
float MaxIm = 1;

bitmap_image img( width, height );

int mandlebrot( int x, int y, int m);

int main(int argc, char *argv[])
{
    int test_threads;
    int max_it =  atoi(argv[1]);
    if( argc != 2)
    {
        cout<< argc <<endl;
        cout<< "use arguments iterations" <<endl;
        return -1;
    }
    omp_set_num_threads(10);
    #pragma omp parallel \
    shared (img, max_it, MinRe, MaxRe, MinIm, MaxIm) \

    #pragma omp for
    for( int i = 0; i < width; i++)
    {
    test_threads = omp_get_num_threads();
        for( int j = 0; j < height; j++ )
        {
            img.set_pixel( i, j, mandlebrot( i, j, max_it), 0, 0 );
        }
        if(!(i%30))
            cout<< 100 * float(i)/width << "%" <<endl;
    }

    img.save_image("Test.bmp");
    cout<< "Used " <<test_threads << " threads" <<endl;
    return 0;
}

int mandlebrot( int x, int y, int max_it )
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