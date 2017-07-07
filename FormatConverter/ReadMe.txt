This is a C++ program to convert BedMaster EX's XML files into HDF5 files.
It is intended to be portable between linux and Windows. I have not gotten
it to work under cygwin.

Building on WINDOWS:
By default, the project assumes the HDF5 libraries (MSVC 2015 version) are in C:\HDF_Group 
Compile using Visual Studio 2015.

Building on LINUX:
Compile with h5c++ (HDF Group's c++ wrapper). There is a NetBeans project that works, but simply
running "make -f Makefile CONF=Release" should take care of everything.

Good luck,
