# Test Tensorflow MatMul performance on Adreno 540 [Performance mode]

## 1. Test Tensorflow MatMul Ops performance on Qualcomm Snapdragon 835 platform.
The measured time below is the average of 10 TF MatMul runs. Notice the execution time measured here includes
the TF processing overhead, the results for pure GEMM operation can be found in `../opencl-clblast`
directory.   

## 2. Libraries tested:
  ### 1) Eigen:
    The default TF compute engine for Android devices. Currently, only CPU version is
    available. By default, the TF runtime assumes there're 8 symmetric cores on the devices.
  ### 2) Simple MatMul OpenCL kernel v1:
    The simplest MatMul OpenCL kernel. This provides a baseline for performance improvement.
  ### 3) Local memory MatMul OpenCL kernel v2:
    A slightly more complicated OpenCL kernel using local memory. Parameters for this
    configurations are work-group size, work-group dimension, local memory size.
  ### 4) Default CLBlast BLAS library:
    An OpenCL version of BLAS library with default configuration.
  ### 5) Tuned CLBlast BLAS library:
    An OpenCL version of BLAS library tuned for Adreno 540 GPU.  

## 3. Tensorflow MatMul FP32 results:
#### **Square Matrix Multiplication, measurement unit: microsecond (us)
| Library\Size          |  N=16   |  N=32   |  N=64   |  N=128  |  N=256  |  N=512  |  N=1024 |  N=2048 |
| :---                  | :---    | :---    | :---    | :---    | :---    | :---    | :---    | :---    |
| cpu-Eigen-fp32        | 13      | 29      | 98      | 558     | 8489    | 49335   | 283635  | 3.64e+06|
| tf-kernel 1-fp32      | 9962    | 9744    | 10125   | 11141   | 20353   | 105416  | 825383  | 1.10E+07|
| tf-kernel 2-fp32      | 9460    | 10291   | 9557    | 11180   | 11309   | 70488   | 440655  | 3.69E+06|
| tf-Blast-fp32         | 51555   | 53556   | 53142   | 53525   | 58421   | 85502   | 913113  | 7.01e+06|
| tf-Blast-fp32-tuned   | 118644  | 116903  | 119384  | 117911  | 123486  | 143377  | 165560  | 433448  |
| cl-Blast-fp32         | 1550    | 1890    | 2768    | 2969    | 5395    | 29013   | 860412  | 6719718 |
| cl-Blast-fp32-tuned   | 2467    | 2544    | 2771    | 3209    | 4648    | 20057   | 52937   | 318077  |

#### Testing environment: ./opencl-matmul N 10
