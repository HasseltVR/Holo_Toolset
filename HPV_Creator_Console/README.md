# HPVCreatorConsole

## Description
This is a cross-platform command-line tool that let's you compress HPV file from a console window. 

Tested on Windows 10 - Mac OS Sierra - Ubuntu 16

## Compilation
This project uses a CMake workflow. Clone the parent Holo_Toolset repository to your computer. Enter the HPV_Creator_Console directory.

For Unix:
```
mkdir build
cd build
cmake ..
make
```

For Windows + Visual Studio :
```
mkdir build
cd build
cmake .. -G "Visual Studio 14" (for VS2015 - change 14 to your VS version)
```
Then open the Visual Studio solution file in the build directory to compile the project. 

For Windows + MinGW :
```
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

## Command-line parameters:
```
usage: ./HPVCreatorConsole --in=string --fps=int --type=int [options] ... 
options:
  -i, --in         in path (string) to image sequence directory
  -f, --fps        framerate (int)
  -t, --type       type (int)
  -s, --start      start frame (int [=0])
  -e, --end        end frame (int [=100])
  -o, --out        out path (string [=])
  -n, --threads    num threads (int [=8])
  -?, --help       print this message
```
The parameters above are mostly self-explanatory, but `type` is required and the argument should be an `int`, corresponding to the following compression types:

* `0` = DXT1 (no alpha)
* `1` = DTX5 (with alpha)
* `2` = Scaled DXT5 (CoCg_Y)