#include "gausslaguerre.h"

GaussLaguerre::GaussLaguerre(int size)
{
    int j = 0;
    for (int n = 1; n <= 4; n++) // frequency
    {
        for (int k = 0; k <= 5; k++) // size
        {
            Matrix re = Matrix::zeros(size, size);
            Matrix im = Matrix::zeros(size, size);

            createWavelet(re, im, n, k, j);

            realKernels << re;
            imagKernels << im;
        }
    }
}

unsigned int factorial(unsigned int n)
{
    unsigned int ret = 1;
    for(unsigned int i = 1; i <= n; ++i)
        ret *= i;
    return ret;
}

double over(double n, double k)
{
    return ((double)factorial(n))/(factorial(k) * factorial(n-k));
}

void GaussLaguerre::createWavelet(Matrix &real, Matrix &imag, int n, int k, int j)
{
    int size = real.rows;
    assert(size > 0);
    assert(size == real.cols);
    assert(size == imag.rows);
    assert(size == imag.cols);

    for (int y = 0; y < size; y++)
    {
        double realY = size/2 - y;
        for (int x = 0; x < size; x++)
        {
            double realX = size/2 - x;
            double r = sqrt(realX*realX + realY*realY)/(size/4);
            double theta = atan2(realY, realX);

            real(y, x) = h(r, theta, n, k, j) * cos(n * theta);
            imag(y, x) = h(r, theta, n, k, j) * sin(n * theta);
        }
    }
}

double GaussLaguerre::L(double r, int n, int k)
{
    double sum = 0;
    for (int h = 0; h <= k; h++)
    {
        sum += pow(-1, k) * over(n + k, k - h) * pow(r, h) / factorial(h);
    }
    return sum;
}

double GaussLaguerre::h(double r, double theta, int n, int k, int j)
{
    return pow(-1, k) * pow(2, (fabs(n)+1.0)/2.0) * pow(M_PI, fabs(n)/2.0) *
            pow((((double)(factorial(k)))/factorial(fabs(n) + k)), 0.5) * pow(r, fabs(n)) *
            L(2 * M_PI * r * r, n, k) * exp(-M_PI * r * r) * exp(j * n * theta);
}
