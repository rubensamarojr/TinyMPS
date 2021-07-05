# TinyMPS
This is a simple library for Moving Particle Semi-implicit (MPS) method.

## Requirements

- [GCC (GNU Compiler Collection)](https://gcc.gnu.org)
- [Eigen](http://eigen.tuxfamily.org)
- [Boost C++ Libraries](http://www.boost.org)

In order to install C++ compiler (GCC) on **windows**, we recommend to install [Cygwin](https://cygwin.com). You can find [here](https://www3.ntu.edu.sg/home/ehchua/programming/howto/Cygwin_HowTo.html) how to install Cygwin. 
The following Packages should be selected during the Cygwin installation:
- automake
- gcc-core
- gcc-fortran
- gcc-g++
- gdb
- libstdc++
- make

## Usage of TinyMPS

You can build the project in GNU/Linux using the makefile. Follow these steps (CPU version):

Edit the `Makefile` file (if necessary) with a text editor.

Copy the folder called `Eigen` that is inside the previous download (Eigen) and paste it in the folder `include` of the TinyMPS directory. 

Copy the folder called `boost` that is inside the previous download (Boost) and paste it in the folder `include` of the TinyMPS directory.

Using **Git BASH** or **command prompt (cmd)**, go to the folder `TinyMPS` with the command

```bash
cd TinyMPS
```

Make sure the environment is clean and ready to compile. Do the command

```bash
make clean
```

Execute the make by the following command

```bash
make all
```

This should create several binary executables (.exe) in folder `bin`.

To run an example, first create a folder called `output`. Do the command

```bash
mkdir output
```

Run an executable, e.g. 

Using **Windows**

```bash
.\bin\standard_mps.exe
```

Using **Linux**

```bash
./bin/standard_mps
```

It should start the program and print something like:

Computation Time: 00h 00min 37.876s.
Time step: 001500, Current time: 9.983891e-01, Delta time: 5.654592e-04
Particles - inners: 629, surfaces: 199, others: 564 (ghosts: 0)
Max velocity: 2.829559
Solver - iterations: 60, estimated error: 1.33184e-16

This example code writes vtk files as output. You can visualize them with Paraview (https://www.paraview.org).

## License
Copyright (c) 2017 Shota SUGIHARA

Distributed under the [MIT License](LICENSE).


## Voro++
Go out of the folder `TinyMPS` with the comand

```bash
cd ..
```

Clone the Voro++ repository into your system. Using **Git BASH** or **command prompt (cmd)**, enter the following command

```bash
git clone https://github.com/chr1shr/voro.git
```

To beign, you should edit the file `config.mk` using a text editor. Make the following change

```bash
PREFIX=build
```

Go to the folder `voro` with the comand

```bash
cd voro
```

Create a new folder called `build`. Do the command

```bash
mkdir build
```

Go to the folder `build` with the command

```bash
cd build
```

Execute cmake to generate the makefile. Do the command

```bash
cmake -G "Unix Makefiles" -DBUILD_SHARED_LIBS=ON ..
```

Execute make by the following command

```bash
make all
```

This should create several binary executables (.exe) in folders inside the folder examples. Also, the libraries **libvoro++.dll** and **libvoro++.dll.a** are created in the current folder build.

Go to the folder `TinyMPS`, and create a folder called `lib`.

Copy the file **libvoro++.dll** from the folder `voro/build` to the folder `lib` in the TinyMPS directory.

## GMSH

Download the Gmsh from
https://gmsh.info/

Gmsh will be used to verify the voronoi geometries (.geo).

## voroGmsh
Out of the folders `TinyMPS` and `voro`, clone the voroGmsh repository into your system. Enter the following command

```bash
git clone https://github.com/DorianDepriester/voroGmsh.git
```

go to the folder `voroGmsh` with the command

```bash
cd voroGmsh
```

Create new folders called `src` and `include`. Do the commands

```bash
mkdir src
```

```bash
mkdir include
```

Move the file `vorogmsh.h` to the folder `include` and move the file `vorogmsh.cpp` to the folder `src`.

Using texte editor, make the following change in both files `vorogmsh.cpp` and `vorogmsh.h`

```bash
#include "adjacencyMatrix.git/trunk/adjacencyMatrix.h"
```

to

```bash
#include "adjacencyMatrix.h"
```

Since voroGmsh needs AdjacencyMatrix, you should add the files from AdjacencyMatrix into the voroGmsh directory.

First, clone the AdjacencyMatrix repository by the command

```bash
git clone https://github.com/DorianDepriester/adjacencyMatrix.git
```

After that, copy the file `adjacencyMatrix.h` to the folder `include` in the voroGmsh directory and copy the file `adjacencyMatrix.cpp` to the folder `src` in the voroGmsh directory.

It is necessary to add all the header files (.hh) from voro++ to voroGmsh.
Copy all files with extension ".hh" from the folder `voro/src` to the folder `include` in the voroGmsh directory.

The makefile we provide here (look in the folder `TinyMPS/VORO_MAKEFILE`) can generate static library (.a), shared library (.dll) or an executable (.exe) .

Edit the Makefile file (if necessary) with a text editor.

To create the static library **libvoroGmsh.a**, enter the command

```bash
make static
```

To create the shared library **libvoroGmsh.dll**, enter the command

```bash
make shared
```

To create the executable **voroGmsh.exe**, first, create new folder called `lib` with the commands

```bash
mkdir lib
```

Copy the file **libvoroGmsh.dll** from the folder `voro/build` to the folder `lib` in the voroGmsh directory.

Enter the comand

```bash
make
```

Copy the file **libvoro++.dll** from the folder `voroGmsh` to the folder `lib` in the TinyMPS directory.

## TinyMPS + Voro++

After the inclusion of the libraries Voro++ and voroGmsh following the previous instructons, you can compile tinyMPS with voro++. 

To do that, go to the folder `TinyMPS` and make sure that the environment is clean

```bash
make clean
```

Execute the following command

```bash
make voro=yes
```
