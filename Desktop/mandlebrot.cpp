#include <iostream>
#include <fstream>
#include <complex>

using namespace std;

const int width = 800;
const int height = 600;
float MinRe = -2.0;
float MaxRe = 1.0;
float MinIm = -1.2;
float MaxIm = MinIm+(MaxRe-MinRe)*height/width;

int mandlebrot( int x, int y, int m);

int main(int argc, char *argv[])
{
    if( argc != 3)
    {
        cout<< argc <<endl;
        cout<< "use arguments iterations, filename" <<endl;
        return -1;
    }
    ofstream image(string(argv[2]) + ".ppm");
    if( image.is_open() )
    {
        image << "P3\n" << width << " " << height << "\n 255" <<endl;

        for( int i = 0; i < width; i++)
        {
            for( int j = 0; j < height; j++ )
            {
                image<< mandlebrot( i, j, atoi(argv[1]) ) << ' ' << 0 << ' ' << 0 << endl;
            }
            if(!(i%30))
                cout<< 100 * float(i)/width << "%" <<endl;
        }
        image.close();
    }
    else
    {
        cout<< "Couldn't open image" <<endl;
    }
    return 0;
}

int mandlebrot( int x, int y, int max_it )
{
    complex<float> point( (float)x/width-1.5, (float)y/height-0.5 );
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