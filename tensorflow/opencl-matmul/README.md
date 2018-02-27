Test Tensorflow GEMM ( General Matrix-to-Matrix Multiplication on Adreno 540 ) [Performance mode]
===============================================================================================
1. Test Tensorflow MatMul Ops performance on Qualcomm Snapdragon 835 platform. The measured time
below is the average of 10 TF MatMul runs. Notice the execution time measured here includes
the TF processing overhead, the results for pure GEMM operation can be found in `../opencl-clblast`
directory.   

2. Libraries tested:
  1) Eigen:
    The default TF compute engine for Android devices. Currently, only CPU version is
    available. By default, the TF runtime assumes there're 8 symmetric cores on the devices.
  2) Simple MatMul OpenCL kernel v1:
    The simplest MatMul OpenCL kernel. This provides a baseline for performance improvement.
  3) MatMul OpenCL kernel v2:
    A slightly more complicated OpenCL kernel using local memory. Parameters for this
    configurations are work-group size, work-group dimension, local memory size.
  4) CLBlast BLAS library:
    An OpenCL version of BLAS library with default configuration.
  5) Tuned CLBlast BLAS library:
    An OpenCL version of BLAS library with tuned configuration for Adreno 540 GPU.  

3. Tensorflow MatMul FP32 results:
x----------------------------------------------------------------------------------------------x
| Square Matrix Multiplication, measurement unit: microsecond (us)                             |
x----------------------------------------------------------------------------------------------x
| Library\Size |  N=16   |  N=32   |  N=64   |  N=128  |  N=256  |  N=512  |  N=1024 |  N=2048 |
x----------------------------------------------------------------------------------------------x
| Eigen        | 13      | 29      | 98      | 558     | 8489    | 49335   | 283635  | 3.64e+06|
x----------------------------------------------------------------------------------------------x
| opencl-kernel|         |         |         |         |         |         |         |         |
| v1           | 2536    |   2578  |   2572  |   3972  |  13370  |  100843 | 791522  | 1.13e+07|
x----------------------------------------------------------------------------------------------x
| opencl-kernel|         |         |         |         |         |         |         |         |
| v2           | 2590.10 | 2566.13 | 2690.32 | 3032.24 | 11309.1 | 64300.9 | 418989  | 2.06e+06|
x----------------------------------------------------------------------------------------------x
| CLBlast      | 51555   | 53556   | 53142   | 53525   | 58421   | 85502   | 913113  | 7.01e+06|
| [Default]    |         |         |         |         |         |         |         |         |
x----------------------------------------------------------------------------------------------x
| CLBlast      | 53679   | 52819   | 52712   | 52049   | 56768   | 84532   | 933545  | 6.80e+06|
| [Tuned]      |         |         |         |         |         |         |         |         |
x----------------------------------------------------------------------------------------------x
