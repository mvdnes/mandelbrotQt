#include "mandelbrotcl.h"


#include <iostream>

MandelBrotCL::MandelBrotCL()
{
}

bool MandelBrotCL::setDefaultDevice() {
    std::vector<cl::Platform> platformList;

    cl::Platform::get(&platformList);
    if (platformList.size() == 0) return false;

    this->devices.clear();
    platformList[0].getDevices(CL_DEVICE_TYPE_GPU, &(this->devices));
	if (this->devices.size() == 0) return false;

    return buildProgram();
}

void MandelBrotCL::setColorMap(cl_uint *map, cl_uint size) {
    this->colormap = map;
    this->colormapsize = size;
}

void MandelBrotCL::requestRender(cl_uint width, cl_uint height, mbcl_real scale, mbcl_real centerx, mbcl_real centery, cl_uint maxiters, cl_uint *out) {
    cl_int err;
    cl::Buffer outCL(
                context,
                CL_MEM_WRITE_ONLY,
                width * height * sizeof(cl_uint),
                NULL,
                &err);
    if (!checkCLError(err, "Buffer::Buffer()")) return;

    cl::Buffer colormapCL(
                context,
                CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                colormapsize * sizeof(cl_uint),
                colormap,
                &err);
    if (!checkCLError(err, "Buffer::Buffer() 2")) return;


    cl::Kernel kernel(program, "run", &err);
    if (!checkCLError(err, "Kernel::Kernel()")) return;

    // top left bottom right
    mbcl_real4 rect = {
        centery + (height / 2 * scale),
        centerx - (width / 2 * scale),
        centery - (height / 2 * scale),
        centerx + (width / 2 * scale),
    };

    unsigned int argnum = 0;
    err = kernel.setArg(argnum++, rect);
    if (!checkCLError(err, "Kernel::setArg()")) return;
    err = kernel.setArg(argnum++, maxiters);
    if (!checkCLError(err, "Kernel::setArg()")) return;
    err = kernel.setArg(argnum++, colormapCL);
    if (!checkCLError(err, "Kernel::setArg()")) return;
    err = kernel.setArg(argnum++, colormapsize);
    if (!checkCLError(err, "Kernel::setArg()")) return;
    err = kernel.setArg(argnum++, outCL);
    if (!checkCLError(err, "Kernel::setArg()")) return;

    cl::CommandQueue queue(context, devices[0], 0, &err);
    if (!checkCLError(err, "CommandQueue::CommandQueue()")) return;

    cl::Event event;
    err = queue.enqueueNDRangeKernel(
                kernel,
                cl::NullRange,
                cl::NDRange(height, width),
                cl::NullRange,
                NULL,
                &event);
    if (!checkCLError(err, "ComamndQueue::enqueueNDRangeKernel()")) return;

    event.wait();
    err = queue.enqueueReadBuffer(
                outCL,
                CL_TRUE,
                0,
                width * height * sizeof(cl_uint),
                out);
    if (!checkCLError(err, "ComamndQueue::enqueueReadBuffer()")) return;
}

std::string MandelBrotCL::getKernelSource() {
    std::string type =
#ifdef MBCL_DOUBLE
            "double"
#else
            "float"
#endif
            ;
    return
#ifdef MBCL_DOUBLE
            "#ifdef cl_khr_fp64\n"
            "    #pragma OPENCL EXTENSION cl_khr_fp64 : enable\n"
            "#elif defined(cl_amd_fp64)\n"
            "    #pragma OPENCL EXTENSION cl_amd_fp64 : enable\n"
            "#else\n"
            "    #error \"Double precision floating point not supported by OpenCL implementation.\"\n"
            "#endif\n"
#endif
            "__kernel void run(\n"
            "	const " + type + "4 rect4,\n"
            "	const uint max_iteration,\n"
            "	__global uint* colormap,\n"
            "	const uint colormapsize,\n"
            "	__global uint* output)\n"
            "{\n"
            "	// rect: top left bottom right\n"
            "\n"
            "	uint yid = get_global_id(0);\n"
            "	uint xid = get_global_id(1);\n"
            "	uint ymax = get_global_size(0);\n"
            "	uint xmax = get_global_size(1);\n"
            "\n"
            "	const " + type + " *rect = (const " + type + "*)&rect4;\n"
            "	" + type + " y0 = (" + type + ")yid / ymax * (rect[0] - rect[2]) + rect[2];\n"
            "	" + type + " x0 = (" + type + ")xid / xmax * (rect[3] - rect[1]) + rect[1];\n"
            "	" + type + " x = 0.0f, y = 0.0f, xtemp;\n"
            "	uint iteration = 0;\n"
            "\n"
            "	while (x*x + y*y < 2*2 && iteration < max_iteration) \n"
            "	{\n"
            "		xtemp = x * x - y * y + x0;\n"
            "		y = 2 * x * y + y0;\n"
            "		x = xtemp;\n"
            "		++iteration;\n"
            "	}\n"
            "   uint value;\n"
            "   if (iteration == max_iteration) value = 0xFF000000;\n"
            "	else value = colormap[iteration % colormapsize];\n"
            "   output[yid * xmax + xid] = value;\n"
            "}\n";
}

bool MandelBrotCL::checkCLError(cl_int err, const char *name) {
    if (err != CL_SUCCESS) {
        std::string errstr = "unknown";
        switch (err) {
        case CL_SUCCESS: errstr = "CL_SUCCESS"; break;
        case CL_INVALID_PROGRAM: errstr = "CL_INVALID_PROGRAM"; break;
        case CL_INVALID_VALUE: errstr = "CL_INVALID_VALUE"; break;
        case CL_INVALID_DEVICE: errstr = "CL_INVALID_DEVICE"; break;
        case CL_INVALID_BINARY: errstr = "CL_INVALID_BINARY"; break;
        case CL_INVALID_BUILD_OPTIONS: errstr = "CL_INVALID_BUILD_OPTIONS"; break;
        case CL_INVALID_OPERATION: errstr = "CL_INVALID_OPERATION"; break;
        case CL_DEVICE_NOT_AVAILABLE: errstr = "CL_DEVICE_NOT_AVAILABLE"; break;
        case CL_BUILD_PROGRAM_FAILURE: errstr = "CL_BUILD_PROGRAM_FAILURE"; break;
        }

        std::cerr << "ERROR: " << name << " (" << errstr << ", " << err << ")" << std::endl;
        return false;
    }
    return true;
}

bool MandelBrotCL::buildProgram() {
    cl_int err;
    cl_context_properties cprops[] = {
        CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(devices[0].getInfo<CL_DEVICE_PLATFORM>()),
        0
    };
    cl::Context context(devices, cprops, NULL, NULL, &err);
    if (!checkCLError(err, "cl::Context()")) return false;
    this->context = context;

    std::string prog = getKernelSource();
    cl::Program::Sources source(1, std::make_pair(prog.c_str(), prog.length()));
    cl::Program program(this->context, source);
    err = program.build(devices, "");
    if (!checkCLError(err, "program.build()")) {
        std::cerr << "== Compilation error: ==\n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << "\n==" << std::endl;
        return false;
    }

    this->program = program;

    return true;
}
