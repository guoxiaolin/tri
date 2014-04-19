tri
===
A simple finite element method library<br />
Read mesh information generated by Triangle [http://www.cs.cmu.edu/afs/cs.cmu.edu/project/quake/public/www/triangle.html](http://www.cs.cmu.edu/afs/cs.cmu.edu/project/quake/public/www/triangle.html)<br /> 

Usage
--------
Read parameters from default input file

	./tri

Read parameters from myinput.input

	./tri myinput.input

Changelog
--------
> Apr. 19
* modified all files with c++ template in order to deal with different problem definition for different schemes
* added a matlab visulization code plotTri.m
* added some sample mesh info files: dat/square.1.node, dat/square.1.ele, dat/square.1.edge
>
> Apr. 18
* Added a Discontinuous Galerkin solving system
* a sample parameter file for DG tri.input, and for FEM triFEM.input
>
> Mar. 28
* seperate BasicSolvingSystem and MySolvingSystem, rename the latter FEMSolvingSystem
>
> Mar. 27
* add some parameters to gain control of input/output
* a sample parameter setting file is tri.input
* able to choose between UMFPACK and SuperLU to solve the spare matrix
>
> Mar. 26
* the UMFPACK solving part is now working
* added SuperLU solving part
* rearranged some structure
* added some comments
>
> Mar. 25
* version 0.01
* known problem: UMFPACK solving part does not work
