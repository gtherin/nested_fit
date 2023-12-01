# NESTED FIT

Nested_fit is a data analysis program based on the Bayesian statistics for the computation of, for a given set of data and chosen model, the complete probability distribution for each parameter and the Bayesian evidence.
The evidence calculation is based on the nested algorithm from Skilling 2004 [1,2] with the implementation of specific traversall for the search of new live points.
It is written in Fortran/C++/Python and includes complementary routines for visualizing the output results and for doing automatic analysis of series of data.
More information on the program can be found in Refs. [A,B,C] here above.

- :hammer: Jump to [installation](#Installation-instructions).
- :page_with_curl: Jump to [usage](#Usage).

### License

Users are required to accept the license agreement given in LICENSE file. Nested Fit is free for academic usage.

Users are also required to cite the Nested Fit papers here below in their publications (at least (A) and (B)) and their authors.

**Reference articles of nested_fit**:
- [A] M. Trassinelli, *Bayesian data analysis tools for atomic physics*, Nucl. Instrum. Methods B **408**, 301-312 (2017),
[doi:10.1016/j.nimb.2017.05.030](http://dx.doi.org/10.1016/j.nimb.2017.05.030),  [	arXiv:1611.10189](https://arxiv.org/abs/1611.10189)\
- [B] M. Trassinelli, *The Nested_fit Data Analysis Program*, Proceedings **33**, 14 (2019), [doi:10.3390/proceedings2019033014](https://doi.org/10.3390/proceedings2019033014)\
- [C] M. Trassinelli, P. Ciccodicola *Mean Shift Cluster Recognition Method Implementation in the Nested Sampling Algorithm*, Entropy **22**, 185 (2020), [doi:10.3390/e22020185](https://doi.org/10.3390/e22020185)
\
- [D] L. Maillard, F. Finocchi, M. Trassinelli * *Assessing Search and Unsupervised Clustering Algorithms in Nested Sampling*, Entropy **25**, 347 (2023), [doi:10.3390/e25020347](https://doi.org/10.3390/e25020347)

### Authors
Dr. Martino Trassinelli\
CNRS, Institute of NanoSciences of Paris\
email: trassinelli AT insp.jussieu.fr\
email: m.trassinelli AT gmail.com

Lune Maillard\
Institute of NanoSciences of Paris, Sorbonne Université, CNRS\
email: lune.maillard AT insp.upmc.fr

César Godinho\
LIBPhys / NOVA University of Lisbon\
email: c.godinho AT campus.fct.unl.pt

## Installation instructions ##

### From PyPI

The simplest way to install nested_fit.

**Prerequisite**:
- CMake
- Fortran build toolchain (`gcc`, `g++` and `gfortran` or `ifort` or `ifx`)
- Python 3 with numpy, scipy, matplotlib, pandas, getdist

**Instructions**:

1. Just run pip:
```sh
pip install nested_fit
```

### From source

Has more control on system specific optimizations.

**Prerequisite**:
- CMake
- Fortran build toolchain (`gcc`, `g++` and `gfortran` or `ifort` or `ifx`)
- Python 3 with numpy, scipy, matplotlib, padas, getdist (optional)

**Instruction**:

1. Get nested_fit:

```sh
git clone git@github.com:martinit18/nested_fit.git
```
2. Configure cmake:
```sh
cd nested_fit
mkdir build && cd build
cmake ..
```

3. Install nested_fit:
```sh
cmake --build . --config Release --target install
```

These command will build two different executables in the bin directory: 
- `nested_fitXXX` for likelihood function maximisation for data analysis,
- `nested_fit_funcXXX` for functions maximisation not using data.

If Python is found an utility for running and analysing data using nested_fit is also installed: the nested_py package.

**CMake options**

| Option             | Description                                                          | Default |
|:-------------------|:---------------------------------------------------------------------|:-------:|
|DEBUG               | Enable debug mode.                                                   | OFF     |
|NORNG               | Set the nested_fit to use a set seed. Internal test use mainly.      | OFF     |
|OPENMP              | Enable/Disable OpenMP support.                                       | OFF     |
|OPENMPI             | Enable/Disable OpenMPI support.                                      | OFF     |
|AUTOTESTS           | Automatically run tests after compiling.                             | OFF     |
|LAPACK              | Use LAPACK library functions instead of the internal ones.           | OFF     |
|LTRACE              | Enable trace logging output. Will hinder performance.                | OFF     |
|INSTALL_SYSTEM_WIDE | Install for all users (differs for each OS). Requires elevated user. | OFF     |



> You can pass in options on the cmake generation step via: `cmake -D<option_name>=<ON/OFF> ..`\
> These will prevail any time you run your build tools commands.

> :warning: For getdist function in the python library: change in the file `xxx/pythonxx/site-packages/getdist/plots.py`,
`matplotlib.use('Agg')` to `matplotlib.use('TkAgg')`.


### From source (Makefile backwards compatibility)

**Prerequisite**:
- GNU Make (for Mod_metadata.f90 to be updated, GNU Make 4.0 is the minimum version required)
- Fortran compiler (gfortran by default)
- Python 3 with numpy, scipy, matplotlib, pandas, getdist (optional)

**Instruction**:
1. Download the latest version or clone the repository (Same as step 1. from above)
2. Run the commands:
```
cd nested_fit/src/
make
```
These command will build two different executables in the bin directory: 
- `nested_fitXXX` for likelihood function maximisation for data analysis,
- `nested_fit_funcXXX` for functions maximisation not using data. 

Running `make like` will only build the first executable while running `make func` will only build the second executable.

**Makefile options**

The options for the Makefile are the same as the one for the CMake. For an option to be set to OFF, it needs to be commented in the Makefile.

### General comments

- For the moment, the options OPENMPI and OPENMP cannot be selected at the same time. If both are set to ON, OPENMP will be set to OFF.

- The options NORNG and OPENMP cannot be selected at the same time. If both are set to ON, OPENMP will be set to OFF.

## File descriptions

**Input files**:
- `nf_input.yaml`: Main input file of the program with all user parameter definitions and run configurations.
More info is available [here](#Usage).
- `datafile` to analyze indicated in `nf_input.yaml`.

**Main output files**:
- `nf_output_res.dat`: main output with the results.
It contains the details of the computation (n. of live points trials, n. of total iteration), the final evidence value and its uncertainty, the parameter values corresponding to the maximum of the likelihood function, the mean, the median, the standard deviation and the confidence intervals (credible intervals) (68%, 95% and 99%) of the posterior probability distribution of each parameter.
The corresponding information gain and the Bayesian complexity are also provided.
- `nf_output_data_*.dat`: original input data together with the model function values corresponding to the parameters with the highest likelihood function value ('max') to the mean value ('mean') or median ('median'), the residuals and the uncertainty associated to the data.
- `nf_output_fit_*.dat`: Model function values with higher density sampling that the data (for plot purpose). In addition, different components of the model are given
- `nf_output_tries.dat`: For each live points trial, it contains the final evidence, the number of iterations and the maximum value of the likelihood function.
- `nf_output_points.txt`: contains all discarded and final live points values, their associated likelihood values and posterior probabilities. From them, the different parameter probability distributions can be built.
For this purpose, the python module `nested_py` can be used also for compressed `nf_output_points.txt` files (using gzip with the name `nf_output_points.txt.gz`).
Together with this file, the files `nf_output_points.paramnames` and `nf_output_points.ranges` are also created for the use of the GetDist python library.

## Usage

**Details of the input file line by line**

A complete selection of input files example is given in the folder `examples` where use cases of the python library are given.

It follows a complete description of `nf_input.yaml` file.

```yaml
version: 5.0                             # Program version
datafiles: file1.csv [, file2.csv, ...]  # Name of the data file(s)
specstr: x,c,ce                          # Datafile layout
likelihood: GAUSSIAN                     # The likelihood function
```

The `specstr` field tells nested_fit what data the datafile columns have.

`specsct` specification:
- `x`: This is the x data column (required).
- `c`: This is the counts data column (required).
- `ce`: This is the error data column (optional).
- `i`: This column will be ignored by nested_fit (optional).


Likelihood functions available (for data with error bars, Poisson statistics is assumed for counts):
- `GAUSSIAN`: Default normal distribution assuming data likelihood.
- `MOD_JEFFREYS`: Robust likelihood that does not assume a majorated error distribution (see Ref. [2]).

```yaml
function:
    expression: f(x, a, b) = ax + b # Use LaTeX
    expression: my_func.cpp         # Or use C++
    expression: my_func.f90         # Or use Fortran
    expression: GAUSS_BG            # Or use a nested_fit legacy function (deprecated)

    # Or for multiple files use the following nomenclature
    expression_1: ...
    expression_2: ...
    expression_<n>: ...

```
- LaTeX specification [here](#LaTeX-Specification).
- C/Fortran API specification [here](#C/Fortran-Specification).


```yaml
200                 # Number of live points
LIKE_ACC            # Method used for convergence 
1.E-05    0.01      # Evidence final accuracy and additional convergence parameter
```

For the moment, there are three convergence methods:
- `LIKE_ACC`: the algorithm stops when the difference between the calculated evidence and the estimated total evidence is below a certain value (first parameter on the above line). Typical value for the parameter : 1.E-05. In this case, the function that is maximised is the log-likelihood and it is associated to data.
- `ENERGY_ACC`: the algorithm stops when the difference between the calculated partition function and the estimated total partition function is below a certain value (first parameter on the above line). The second parameter corresponds to the temperature at which the partition function is calculated. Typical value for the first parameter : 1.E-05. In this case, the function that is maximised is the opposite of the energy function.
- `ENERGY_MAX`: the algorithm stops when the difference between the current contribution to the partion function and its maximal previous contribution is below a certain value (first parameter on the above line). The second parameter corresponds to the temperature at which the partition function is calculated. Typical value for the first parameter : -10.  In this case, the function that is maximised is the opposite of the energy function.

After convergence is reached, all remaining live points are assigned: 
- the logarithm of the likelihoods averaged over the live points (`LIKE_ACC` case),
- the opposite of the energies averaged over the live points (`ENERGY_ACC` and `ENERGY_MAX` cases).
```
RANDOM_WALK         # Type of search of live points
0.1  20   100  10   # Param. search algo.(2), max n. tries, max of max tries
```
For the moment, a random walk (`RANDOM_WALK`), a uniform search around each live point (`UNIFORM`), slice sampling (`SLICE_SAMPLING`) and slice sampling with an adaptable step (`SLICE_SAMPLING_ADAPT`) are implemented. The first two parameters of the above line are specific to the search algorithm:
- `RANDOM_WALK` par. 1: fraction of standard deviation for each jump, par. 2: number of jumps. Suggested values: 0.1-0.2, 10-40.
- `SLICE_SAMPLING` and `SLICE_SAMPLING_ADAPT` par. 1: fraction of standard deviation for segment exploration, par. 2: number of jumps. Suggested values: ~1, 3-5.
- `UNIFORM` par. 1: fraction of standard deviation for the box size, par. 2: number of jumps. Suggested values: 0.1-1, 1.


```
y	f	0.5	0.2     # cluster analysis or not (y/n), method (f/g/d/s/k), param. cluster algo. (2)
```

For the moment four clustering algorithms are implemented. The two parameters are specific to the method
For the second option:
- `f`: mean-shift with flat kernel (par. 1: distance)
- `g`: mean-shift with gaussian kernel (par. 1: distance, par. 2: bandwidth)
- `d`: dbscan (par. 1: distance, par. 2 : minimum number of neighbours)
- `s`: agglomerative clustering with single linkage (par. 1: distance limit (in percentage of the maximum distance))
- `k`: k nearest neighbours (no parameters)

```
1    100000         # Number of runs and maximum of steps
GAUSS_BG            # Name of the function
```



```
L                   # Additional data: left/right (l/r)
500     20          # Additional data:  npoint, nwidth for convolution
1   1024  1 1024    # xmin, xmax, ymin, ymax
4                   # number of parameters
# npar  name    value   step    min     max     fixed
1	'bg'	0.11	-1	0.	0.5	0
2	'x0'	454.6	-1	400	600	0
3	'amp'	296	-1	20	1000	0
4	'sigma'	20.0	-1	0	100	0
```

### LaTeX Specification

### C/Fortran Specification

Additional information can be found in the reference articles.

## Present version and history of the past versions

The present version is 5.0.0\
New features:
- New modified Jeffreys likelihood for data
- Update README.md
- Add CI support via github actions. Only available for linux and macOS.
- Add support to fully install via pip.
- Add python package install support (not published).
- Support custom data file column ordering/separation. Support .csv, .tsv
- New native function parser that reads users Fortran or C/C++ files with functions.
- New LaTeX function parser that reads user inline functions.
- Complete overhaul of input file and function user function definition.


Previous versions are:
 - 4.5 New modified Jeffreys likelihood for data
 - 4.4 New "write_input" function in python library \
 New fit functions \
 External LAPACK library link option \ 
 OpenMP for parallel search of new points \
 OpenMPI support (only available for number of tries) \
 OpenMP parallelisation for independent live point search \ 
 New user function calling method \
 Add Windows support \
 New build system generator (CMake) \
 Improved performance
 - 4.3 New (test) function : harmonic potential in 3D and loggamma \
 Choice between different convergence methods : evidence or partition function  
 - 4.2 Additional search methods : Uniform search around each live point and Slice Sampling
 - 4.1 New cluster recognition methods added
 - 4.0  2D data analysis for count-type XY \
 Computation acceleration for the 1D case introducing a mask instead of IF condition in the likelihood \
 New ERFPEAK function \
 New example files for python analysis with pandas
 - 3.5 Modularization of the search and cluster recognition methods in preparation of implementation of new algorithms \
 New interpolation options for 1D and 2D histograms using GetDist Python package \
 Correction of some bugs in the python library \
 Additional folder with exercises is now available \
 Installation instructions now available \
 Compatible now with intel fortran (options to change in Makefile)
 - 3.4 Introduction of benchmark tests with synthetic likelihood function via the module Mod_likelihood_tests,f90 (instead of Mod_likelihood.f90).
 Available tests: TEST_GAUSS (multidimensional Gaussian), TEST_GAUSSIAN_SHELLS (multidimensional Gaussian shells, worse case with available search and clustering methods),
 TEST_EGGBOX (eggbox style profile to test clustering), TEST_ROSENBROCK (Rosenbock function test for 2-n dimension).\
 Change of the outputs: nf_output_points.dat -> nf_output_points.txt, plus files nf_output_points.paramnames,  and nf_output_points.ranges to be compatible with GetDist Python package. New 'triangle plot' available now.
 - 3.3 Modular version of likelihood function in preparation for handling more complex data (2D data, ...).
 - 3.2  This is the first version with free sharing code only.
    Pion mass function and laser interpolation taken out to avoid Numerical Recipes.\
    Indexing for sorting data from SLATEC routine now.\
    Log(factorial) and gamma function from intrinsic function DLGAMMA now (and via a new routine for the factorial).\
    Test for integer input for Poisson likelihood.\
    Fitpack for splines in Shirley profile too.
 - 3.1  Optimization of parallel computing.\
      Corrections to Shirley functions.\
      Add of latest Shirley functions.\
      Fix a bug in the fixed parameters.\
      Add of the time stamp to the cluster analysis files
 - 3.0  Cluster analysis for live points to improve (very much!!!) the search efficiency.
 - 2.3  Parallelism optimization. Usercondition routine removed\
      Correction for gaussian priors (before was done at each jump, now only at the end).
 - 2.2  Add of "Rocking curve" profile that uses external simulated data for set of files
      (parallel and antiparallel, still to test).
      Solved problem with probabilities > 1 for gaussian data. Each data point is considered distributed
      with a Gaussian. But in the normalization factor, sigmas have not to appear. Otherwise probabilities
      can have a dimension.The variables are naturally transformed in dimensionless unit in the
      exponential part.\
      Extraction of live points values in case of non-convergence
 - 2.1  Add of "Rocking curve" profile that uses external simulated data and FITPACK routines for the use of smoothed B-splines.
 - 2.0  Treatment of data with error bars becomes possible.\
      No error bars: Likelihood with Poisson distribution.\
      Error bars   : Likelihood with Gaussian distribution.
 - 1.0  Add of Shirley background for photoemission spectra\
      Add of data output for mean and median parameter values (in addition to max likelihood values).
      Parallelization is back but only for the likelihood calculation otherwise it sucks (most of the time).
 - 0.9  Implementation of complex error function from TOMS algorithm n. 680 (included in Fortran 2008) WOFZ.f.
 - 0.8: Save in a file the different fit components (to implement in part of the functions).
 - 0.7: Add of pion mass function for sets.
 - 0.6: Add capabilities to analyze sets of spectra.
 - 0.5: Add of the pion mass function.
 - 0.4: Improved search algorithm for presence of local maxima
      Add of the information, minimum required iteration and complexity calculation
      D.S. Sivia, "Data Analysis, a Bayesian tutorial" (Ref. [2]),
      R. Trotta, Contemporary Physics 49, 71 (2008),
      J. Veitch and A. Vecchio, Phys. Rev. D 81, 062003 (2010).
 - 0.3: Optimization with memory allocation and more variables accessible from the input file.
 - 0.2: Version with parallel seek of groups of live points inspired by (not working anymore since 2016)
      J. Veitch and A. Vecchio, Phys. Rev. D 81, 062003 (2010) and
      N. Chopin and C.P. Robert, Biometrika 97, 741-755 (2010).
 - 0.1: Program developed from D.S. Sivia, "Data Analysis, a Bayesian tutorial" (2006) and L. Simons' original program.

## External codes and other contributors

### Additional included sources

In addition to the original files from the main authors, nesed_fit includes
- *FITPACK (DIERCKX)* package, to fit and interpolating data with splines (no license)
  - Ref: Paul Dierckx, Curve and Surface Fitting with Splines, Oxford University Press, 1993
 Developer,
  - Address: Paul Dierckx, Department of Computer Science, K.U. Leuven, Celestijnenlaan 200 A, B-3001, Heverlee, Belgium, Paul.Dierckx@cs.kuleuven.ac.be
  - [http://nalag.cs.kuleuven.be/research/topics/fitpack.shtml](http://nalag.cs.kuleuven.be/research/topics/fitpack.shtml)
- `dpsort.f` + dependencies from SLATEC library (no license) for sorting arrays with respect another array
- `WOFZ.f`' for the complex error function from TOMS algorithm n. 680 (included in Fortran 2008)
- `rinteg.f` to calculate the integral of a function using a nq points quadrature ( nq is any integer between 1 and 14 ). Written by C. C. J. Roothaan (no license)

### Other contributors to the code
(chronological order)

- Anna Lévy (photoemission spectra models)
- Nancy Paul (rocking curves and Bragg spectrometer lines)

### Thanks for bugs corrections
(alphabetical order)

- Takuma Okumura

## Other resources

**Articles about nested sampling method**:\   
[1] J. Skilling, *Nested sampling for general Bayesian computation*, Bayesian Anal. **1**, 833-859 (2006)\
[2] D.S. Sivia and J. Skilling, *Data analysis: a Bayesian tutorial*. Second ed. 2006: Oxford University Press
