#include <clpeak.h>

#define MSTRINGIFY(...) #__VA_ARGS__

static const char *stringifiedKernels =
#include "global_bandwidth_kernels.cl"
#include "compute_sp_kernels.cl"
#include "compute_dp_kernels.cl"
#include "compute_integer_kernels.cl"
;

static const char *stringifiedKernelsNoInt =
#include "global_bandwidth_kernels.cl"
#include "compute_sp_kernels.cl"
#include "compute_dp_kernels.cl"
;

#ifdef USE_STUB_OPENCL
// Prototype
extern "C" {
void stubOpenclReset();
}
#endif


clPeak::clPeak():forcePlatform(false),forceDevice(false), specifiedPlatform(-1), specifiedDevice(-1), useEventTimer(false),
       isGlobalBW(true), isComputeSP(true), isComputeDP(true), isComputeInt(true), isTransferBW(true), isKernelLatency(true)
{
}

clPeak::~clPeak()
{
    if(log) delete log;
}

int clPeak::runAll()
{
    try
    {

#ifdef USE_STUB_OPENCL
        stubOpenclReset();
#endif
        vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        log->xmlOpenTag("clpeak");
        log->xmlAppendAttribs("os", OS_NAME);
        for(int p=0; p < (int)platforms.size(); p++)
        {
            if(forcePlatform && (p != specifiedPlatform))
                continue;

            log->print(NEWLINE "Platform: " + platforms[p].getInfo<CL_PLATFORM_NAME>() + NEWLINE);
            log->xmlOpenTag("platform");
            log->xmlAppendAttribs("name", platforms[p].getInfo<CL_PLATFORM_NAME>());

            cl_context_properties cps[3] = {
                    CL_CONTEXT_PLATFORM,
                    (cl_context_properties)(platforms[p])(),
                    0
                };
            cl::Context ctx(CL_DEVICE_TYPE_ALL, cps);
            vector<cl::Device> devices = ctx.getInfo<CL_CONTEXT_DEVICES>();

            string plaformName = platforms[p].getInfo<CL_PLATFORM_NAME>();
            bool isIntel = (plaformName.find("Intel") != std::string::npos)? true: false;
            bool isPocl = (plaformName.find("Portable Computing Language") != std::string::npos)? true: false;

            cl::Program prog;

            // FIXME Disabling integer compute tests on intel platform
            // Kernel build is taking much much longer time
            if(isIntel) {
                cl::Program::Sources source(1, make_pair(stringifiedKernelsNoInt, (strlen(stringifiedKernelsNoInt)+1)));
                isComputeInt = false;
                prog = cl::Program(ctx, source);
            } else {
                cl::Program::Sources source(1, make_pair(stringifiedKernels, (strlen(stringifiedKernels)+1)));
                prog = cl::Program(ctx, source);
            }

            // FIXME Disable compute-dp & comute-integer tests for pocl
            // DP test segfaults & integer test takes infinite time in llc step
            if(isPocl)
            {
                isComputeDP = false;
                isComputeInt = false;
            }

            try {
                prog.build(devices, BUILD_OPTIONS);
            }
            catch (cl::Error error)
            {
                log->print(TAB "Build Log: " + prog.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) + NEWLINE NEWLINE);
            }

            for(int d=0; d < (int)devices.size(); d++)
            {
                if(forceDevice && (d != specifiedDevice))
                    continue;

                device_info_t devInfo = getDeviceInfo(devices[d]);

                log->print(TAB "Device: " + devInfo.deviceName + NEWLINE);
                log->print(TAB TAB "Driver version  : ");
                log->print(devInfo.driverVersion);  log->print(" (" OS_NAME ")" NEWLINE);
                log->print(TAB TAB "Compute units   : ");
                log->print(devInfo.numCUs);         log->print(NEWLINE);
                log->print(TAB TAB "Clock frequency : ");
                log->print(devInfo.maxClockFreq);   log->print(" MHz" NEWLINE);
                log->xmlOpenTag("device");
                log->xmlAppendAttribs("name", devInfo.deviceName);
                log->xmlAppendAttribs("driver_version", devInfo.driverVersion);

                cl::CommandQueue queue = cl::CommandQueue(ctx, devices[d], CL_QUEUE_PROFILING_ENABLE);

                runGlobalBandwidthTest(queue, prog, devInfo);
                runComputeSP(queue, prog, devInfo);
                runComputeDP(queue, prog, devInfo);
                runComputeInteger(queue, prog, devInfo);
                runTransferBandwidthTest(queue, prog, devInfo);
                runKernelLatency(queue, prog, devInfo);

                log->print(NEWLINE);
                log->xmlCloseTag();       // device
            }
            log->xmlCloseTag();           // platform
        }
        log->xmlCloseTag();               // clpeak
    }
    catch(cl::Error error)
    {
        log->print(error.err() + NEWLINE);
        return -1;
    }

    return 0;
}


float clPeak::run_kernel(cl::CommandQueue &queue, cl::Kernel &kernel, cl::NDRange &globalSize, cl::NDRange &localSize, int iters)
{
    float timed = 0;

    // Dummy calls
    queue.enqueueNDRangeKernel(kernel, cl::NullRange, globalSize, localSize);
    queue.enqueueNDRangeKernel(kernel, cl::NullRange, globalSize, localSize);
    queue.finish();

    if(useEventTimer)
    {
        for(int i=0; i<iters; i++)
        {
            cl::Event timeEvent;

            queue.enqueueNDRangeKernel(kernel, cl::NullRange, globalSize, localSize, NULL, &timeEvent);
            queue.finish();
            timed += timeInUS(timeEvent);
        }
    } else      // std timer
    {
        Timer timer;

        timer.start();
        for(int i=0; i<iters; i++)
        {
            queue.enqueueNDRangeKernel(kernel, cl::NullRange, globalSize, localSize);
        }
        queue.finish();
        timed = timer.stopAndTime();
    }

    return (timed / iters);
}


