#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <values.h>
#include <time.h>
#include "fdmath.h"


#define MIN_MODULE             0.0174 /* sin(1ø) */
#define MAX_MODULE             0.9848 /* sin(80ø) */
#define MIN_X                  0
#define MAX_X                  M_PI

main(int argc, char *argv[])
{
    double PeakErr = 0;
    double Peak_k, Peak_x, Peak_y, steps;
    double F1, F2, Err, module, x, avg = 0;
    int i;
    long k = 0L;

    clrscr();
    steps = atoi(argv[1]);

    randomize();

    do
    {
        if (argc > 2) module = atof(argv[2]);
        else module = (MAX_MODULE-MIN_MODULE)/RAND_MAX * rand() + MIN_MODULE;
        if (argc > 2) x = atof(argv[3]);
        else x = (MAX_X-MIN_X)/RAND_MAX * rand() + MIN_X;
        F1 = EllIntegr_F(steps, module, x);
        F2 = EllIntegr_F(MAXINT-2, module, x);
        Err = fabs(F1 - F2);
        avg = (avg*k+Err);
        avg /= ++k;
        if (Err > PeakErr) {
            PeakErr = Err;
            Peak_x  = x;
            Peak_y  = F2;
            Peak_k  = module;
        }
        printf("\n%5ld: %.16lf (Soll %.15lf), E = %.16lG",
               k, F1, F2, Err);
    }
    while (!kbhit());

    printf("\n\nAvg Err = %.16lG, Peak Err = %.16lG\nat F(%lG,%lg) = %.16lG\n",
           avg, PeakErr, Peak_k, Peak_x, Peak_y);
    getchar();
    return 0;
}
