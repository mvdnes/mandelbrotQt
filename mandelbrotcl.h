#ifndef MANDELBROTCL_H
#define MANDELBROTCL_H

#include <CL/cl.hpp>
#undef min
#undef max

#include <string>
#include <vector>

//#define MBCL_DOUBLE

#ifdef MBCL_DOUBLE
typedef cl_double mbcl_real;
typedef cl_double4 mbcl_real4;
#else
typedef cl_float mbcl_real;
typedef cl_float4 mbcl_real4;
#endif

class MandelBrotCL
{
public:
    MandelBrotCL();
    bool setDefaultDevice();
    void setColorMap(cl_uint* map, cl_uint size);
    void requestRender(cl_uint width, cl_uint height, mbcl_real scale, mbcl_real centerx, mbcl_real centery, cl_uint maxiters, cl_uint* data);
private:
    cl_uint* colormap;
    cl_uint colormapsize;
    std::string getKernelSource();
    std::vector<cl::Device> devices;
    cl::Program program;
    cl::Context context;

    bool buildProgram();
    bool checkCLError(cl_int err, const char *name);
};

#endif // MANDELBROTCL_H
