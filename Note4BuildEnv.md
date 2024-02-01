# Setup build env (for WSL or Linux)
## Overview of setting up Mewtwo compile environment
Mewtwo has to rely on several external open-source libraries to implement some basic functionalities.

The most important one is armadillo, which is a high-performance linear algebra library. 
The computation efficiency relies highly on the hardware optimizations (for example AVX512 instruction on APUs is a big plus for speeding up long vector evaluations).
Lower level math libraries bridge up the gap between the higher level software and lower level instruction set.
For example, Intel's Math Kernal Library (MKL) for Intel x86 CPUs, ACML for AMD's APUs, and Accelerate framework for M series Apple Silicon SoCs. 
Or more universal option OpenBLAS for

RTTR is a runtime reflection library allows us to locate the properties of objects in runtime, which makes the parametrical scanning easy to implement.

muParserX and exprtk are symbolic math parser allows us to parse symbolic math formulas from user input to logical calculations.

To use those 3rd party opensource libs, we have to compile them locally on the target machine and export the PATH environments.
Then the compiler and linker will be able to find them to build our software.
This document will guide you through the procedure of setting up the build environment (for Linux or WSL) from scratch.

### Homebrew install
Homebrew used to only work on macOS, now it is available for Linuxes. 
It is also a package managing software like apt on Ubuntu that provide you with pre-compile binaries. 

## Prepare Compile Toolchain 
### GCC
Firstly check if apt is up-to-date:
```shell
sudo apt-get upgrade -y
```
Add repo for apt
```shell
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
```
Install gcc-11
```shell
sudo apt-get install gcc-13 -y
```
Check if gcc-11 is successfully installed:
```shell
gcc-13 --version
```


### CMake
```shell
brew install cmake 
```

```shell
sudo apt  install cmake
```
### Intel-MKL (for x86 platform)
refer to https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl-download.html?operatingsystem=linux&distributions=aptpackagemanager

```shell
wget -O- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB | gpg --dearmor | sudo tee /usr/share/keyrings/oneapi-archive-keyring.gpg > /dev/null
echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" | sudo tee /etc/apt/sources.list.d/oneAPI.list
sudo apt update
sudo apt install intel-oneapi-mkl
sudo apt install intel-oneapi-mkl-devel
source /opt/intel/oneapi/setvars.sh
```

### HDF5
brew install hdf5

### OpenMP
brew install libomp
Dont forget add Linker flag to env and add to LD_LIBRARY_PATH

### Accelerate (for Apple-Silicon)

## Shell Path Environment setup
The shell path environment is quite important for directing the compiler and linker to find the libraries that we need.

## Compile third party libs

### Armadillo

### RTTR

```shell
wget https://www.rttr.org/releases/rttr-0.9.6-src.tar.gz
tar -xf
```

### muparserx

### exprtk
https://github.com/beltoforion/muparserx
````shell
wget https://github.com/beltoforion/muparserx/archive/refs/tags/v4.0.12.tar.gz
tar -xf v4.0.12.tar.gz
cd 
````
create a new folder under */include/exprtk
### 

