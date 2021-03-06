clpeak
======

A synthetic benchmarking tool to measure peak capabilities of a opencl device. It only measures the peak metrics that can be achieved using vector operations and does not represent a real-world use case


eg:

    Platform: AMD Accelerated Parallel Processing
      Device: Tahiti
        Driver version  : 1445.5 (VM) (Linux x64)
        Compute units   : 32
        Clock frequency : 1000 MHz

        Global memory bandwidth (GBPS)
          float   : 211.27
          float2  : 220.40
          float4  : 217.36
          float8  : 116.76
          float16 : 59.32

        Single-precision compute (GFLOPS)
          float   : 3700.45
          float2  : 3548.87
          float4  : 3531.60
          float8  : 3528.15
          float16 : 3518.72

        Double-precision compute (GFLOPS)
          double   : 864.44
          double2  : 860.07
          double4  : 859.88
          double8  : 850.53
          double16 : 830.28

        Integer compute (GIOPS)
          int   : 783.14
          int2  : 783.54
          int4  : 776.09
          int8  : 753.42
          int16 : 753.52

        Transfer bandwidth (GBPS)
          enqueueWriteBuffer         : 8.86
          enqueueReadBuffer          : 5.29
          enqueueMapBuffer(for read) : 22093.45
            memcpy from mapped ptr   : 3.99
          enqueueUnmap(after write)  : 43471.33
            memcpy to mapped ptr     : 3.92

        Kernel launch latency : 38.14 us



Send in results of your device to krrishnarraj@gmail.com
