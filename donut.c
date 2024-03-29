/* Donut that can run on an embedded system... 
   Originally written by Nicolás Kennedy <nicolas@nicbk.com>
   Licensed under the UNLICENSE (See LICENSE)
 */

#include <stdio.h> // Only library used, for standard output
#define PI 3.14159265358979323846 // First 21 digits of pi
#define TRUE 1 // makes the infinite loop @ line #138 look prettier

// Absolute value function for doubles
double abs_c(double x)
{
    if (x < 0)
    {
        return -x;
    } else
    {
        return x;
    }
}

// Recursive factorial capable of calculating a max of approximately 10^18 in size
unsigned long long fac(unsigned char x)
{
    if (x == 0)
    {
        return 1;
    } else
    {
        return x * fac(x - 1);
    }
}

// Exponential function
// Ignores edge cases such as indeterminate forms for simplicity
double pow_c(double x, unsigned char n)
{
    if (n == 0)
    {
        return 1;
    } else
    {
        double product = x;

        for (int i = 1; i < n; ++i)
        {
            product *= x;
        }

        return product;
    }
}

// MacLaurin series approximation of sin(x) from -PI/2 to PI/2
// with eight terms, and extended to the domain of all
// real numbers with modular arithmetic.
double sin(double x)
{
    double translation = 0;
    if (x > -PI/2)
    {
        translation = (int) ((x + PI/2) / PI);
    } else
    {
        translation = (int) ((x - PI/2) / PI);
    }

    x -= PI*translation;
    if ((int) abs_c(translation) % 2 == 1)
    {
        x = -x;
    }

    double sum = 0;

    for (int i = 0; i < 9; ++i)
    {
        sum += pow_c(-1, i) * pow_c(x, 2*i + 1) / fac(2*i + 1);
    }

    return sum;
}

// cos(x) function derived from sin(x)
double cos(double x)
{
    return sin(PI/2 - x);
}

// tan(x) function derived from sin(x) and cos(x)
double tan(double x)
{
    return sin(x) / cos(x);
} 

// Recursive Newton's method approximation of sqrt(x)
// which returns when the sequence converges with an error
// of less than 0.001
double sqrt_imp(double x, double n)
{
    double y = x - (pow_c(x, 2) - n) / (2 * x);

    if (abs_c(y - x) < 0.001)
    {
        return x;
    }
    else
    {
        return sqrt_imp(y, n);
    }
}

// Wrapper for sqrt_imp()
double sqrt(double x)
{
    return sqrt_imp(x, x);
}

// Renders the torus
void render()
{
    char sizex = 110; // Width of screen in characters
    char sizey = 52; // Height of screen in characters

    // Screens initialized in stack, not using standard library
    // functions for accessing heap.
    char screen[52][110] = {0}; // Final output screen
    double zbuffer[52][110] = {0}; // Buffer screen to keep track of object depth (Z Axis) to prevent clipping

    double viewport = 4; // side length of camera in the world's Euclidean coordinates

    double fov = 30; // Field of view of camera in degrees

    double zp = viewport / tan(fov * PI / 180); // Converts field of view into the focal distance of the camera 

    double torrad = 3; // Distance from center of torus to the center of a circular cross section of the torus
    double torcirrad = 1.5; // Radius of a circular cross section of the torus

    double torx = 0; // Translation of torus on the X axis
    double tory = 2; // Translation of torus on the Y axis
    double torz = 7; // Translation of torus on the Z axis

    double drot = 0.01; // Step at which torus is rotated
    double dtor = 0.02; // Step at which the renderer rotates circular cross sections about the axis of revolution
    double dcir = 0.02; // Step at which the renderer samples the circular cross sections of the torus

    // Directional light source
    double lightx = 0; // Position of light on the X axis 
    double lighty = 2; // Position of light on the Y axis
    double lightz = -1; // Position of light on the Z axis

    // Memoization of sin(x) and cos(x) on the interval [0, 2 * PI]
    // to speed up rendering
    double sin_memoization[1000] = {0};
    double cos_memoization[1000] = {0};

    for (double i = 0; i < 2*PI; i += 0.01) {
        sin_memoization[(int) (i * 1000 / (2 * PI))] = sin(i);
        cos_memoization[(int) (i * 1000 / (2 * PI))] = cos(i);
    }

    while (TRUE)
    {
        // Perform full rotation of the torus at constant step (deltas on lines #142 - #144)
        // Also this is the for loop for each output frame / image
        for (double rot = 0; rot < 2*PI; rot += drot)
        {
            double sinrot = sin_memoization[(int) (rot * 1000 / (2 * PI))];
            double cosrot = cos_memoization[(int) (rot * 1000 / (2 * PI))];

            // For each step of rotation of the torus, reset the Z buffer and screen.
            for (int y = 0; y < sizey; ++y)
            {
                for (int x = 0; x < sizex; ++x)
                {
                    zbuffer[y][x] = 0;
                    screen[y][x] = ' ';
                }
            }

            // Perform a full rotation of a circular cross section of the torus about the axis of revolution
            for (double tor = 0; tor < 2*PI; tor += dtor)
            {
                double sintor = sin_memoization[(int) (tor * 1000 / (2 * PI))];
                double costor = cos_memoization[(int) (tor * 1000 / (2 * PI))];

                // Perform a full rotation of a point on the circumference of the cross section of the torus
                for (double cir = 0; cir < 2*PI; cir += dcir)
                {
                    double sincir = sin_memoization[(int) (cir * 1000 / (2 * PI))];
                    double coscir = cos_memoization[(int) (cir * 1000 / (2 * PI))];

                    // Torus is parameterized

                    double cirpart = torrad + torcirrad * coscir;
                    double rotpart = cirpart * sintor * cosrot - torcirrad * sincir * sinrot;

                    double x = cirpart * costor * cosrot - sinrot * rotpart;
                    double y = cirpart * costor * sinrot + cosrot * rotpart;
                    double z = cirpart * sintor * sinrot + torcirrad * sincir * cosrot;

                    // Apply translation to torus

                    x += torx;
                    y += tory;
                    z += torz;

                    // distance inversely proportional to z coordinate

                    double distance = 1 / z;

                    // If any part of the torus is rendering outside of the camera's viewport,
                    // then skip the rendering of that part of the torus to prevent accessing
                    // invalid coordinates outside of the screen's dimensions
                    if (abs_c(x * zp / (zp + z)) > 4 || abs_c(y * zp / (zp + z)) > viewport)
                    {
                        continue;
                    }

                    // Project objects onto 2D Screen by scaling the proportional to their depth (Z axis)
                    // Camera is situated at the origin, so the viewport is also translated such that
                    // all negative coordinates become positive;
                    // in other words, the bottom left corner of the camera is now the origin.
                    double viewportx = viewport + x * zp / (zp + z);
                    double viewporty = viewport + y * zp / (zp + z);

                    // Convert the world's coordinates into coordinates starting from 0
                    // which can be used to index the screen array
                    char screenx = sizex * viewportx / (2 * viewport); 
                    char screeny = sizey * viewporty / (2 * viewport); 

                    // Only render the point on the torus to the screen if this part of the screen has
                    // not been rendered to yet in this frame,
                    // or if this point on the torus is closer to the camera than any other points previously
                    // rendered to this part of the screen.
                    if (zbuffer[screeny][screenx] == 0 || zbuffer[screeny][screenx] < distance)
                    {
                        zbuffer[screeny][screenx] = distance;

                        // Calculate the surface normal to the torus at the current point
                        // Instead of a more expensive process of finding vectors to calculate the cross product,
                        // this just applies all the rotation parameters to find a point on a sphere, which happens
                        // to be the normalized normal vector to the corresponding point of equivalent rotation
                        // parameters on the torus.
                        double normalrotpart = coscir * sintor * cosrot - sincir * sinrot;
                        
                        double normalx = coscir * costor * cosrot - sinrot * normalrotpart;
                        double normaly = coscir * costor * sinrot + cosrot * normalrotpart;
                        double normalz = coscir * sintor * sinrot + sincir * cosrot;

                        // Calculate segment between the directional light source and the point on the torus
                        double lightrayx = lightx - x;
                        double lightrayy = lighty - y;
                        double lightrayz = lightz - z;

                        // Calculate distance between the directional light source and the point on the torus
                        double lightraymag = sqrt(pow_c(lightrayx, 2) + pow_c(lightrayy, 2) + pow_c(lightrayz, 2));

                        // Scale the segment down to a length of one,
                        // creating a normalized vector representing a ray of light with constant luminosity
                        // hitting the torus at the current point from the direction of the light.
                        double lightvecx = lightrayx / lightraymag;
                        double lightvecy = lightrayy / lightraymag;
                        double lightvecz = lightrayz / lightraymag;

                        // Calculate how much light hits the surface of the torus at the current point
                        // Dot product of the torus surface normal and the light vector
                        // Not quite ray-tracing, as the light is never traced to/from the camera;
                        // however, this should be realistic enough.
                        double luminosity = normalx * lightvecx + normaly * lightvecy + normalz * lightvecz;

                        // Zero luminosity implicates that the light is orthogonal to the surface of the torus.
                        // Negative luminosity implicates the light is behind the surface of the torus.
                        if (luminosity <= 0)
                        {
                            continue;
                        }

                        // Scale the luminosity such that there is a direct relationship to the character chosen to represent
                        // the luminosity of the torus at the current point.
                        screen[screeny][screenx] = ".,_:!x%%&@$#"[(int) (luminosity * 10.4)];
                    }
                }
            }

            // Render the final frame
            for (int y = 0; y < sizey; ++y)
            {
                for (int x = 0; x < sizex; ++x)
                {
                    printf("%c", screen[y][x]);
                }

                printf("\n");
            }

            // ASCII Terminal escape code which moves the cursor to the position (0, 0)
            // in the terminal
            printf("\033[H");
        }
    }
}

int main()
{
    render();
}
