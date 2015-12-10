# Papers

The code and this README applies to two papers  by Henryk Michalewski, Andrzej Nagórko and Jakub Pawlewicz:

1. [An upper bound of 84 for Morpion Solitaire 5D ](http://duch.mimuw.edu.pl/~henrykm/lib/exe/fetch.php?media=morpion5d.pdf) and 
2. ["485 - a new upper bound for Morpion Solitaire"](http://www.mimuw.edu.pl/~henrykm/lib/exe/fetch.php?media=upper-bound-morpion.pdf)


# First steps with the repository

For both articles please make sure that the following code executes at your machine:

```
> mkdir build
> cd build
> cmake ../src
> make
> ./generator --help
```

# An upper bound of 84 for Morpion Solitaire 5D 

This part of README applies to the article [An upper bound of 84 for Morpion Solitaire 5D ](http://duch.mimuw.edu.pl/~henrykm/lib/exe/fetch.php?media=morpion5d.pdf) (submitted). 

The main computation in this paper is performed in two steps:

1. we generate a family of square boxes which contain the initial cross of the Morpion Solitaire; this process gives 291 boxes which are inhabitated by Morpion graphs (this concept is explained in the paper, but the essential point is that every Morpion Solitaire position is a Morpion graph). No larger Morpion graphs exists
2. on each box we solve a respective linear program described in the paper. 

These two steps are done simultaneously, but for the sake of presentation consider this as a two step procedure (this makes
life much easier when it comes to verification of the main result). Here is summary of 12 most time consuming computations

| box 	| parameters passed to the generator 	| time in ms 	|
|:------------:	|:---------------------------------------------------------------------------------:	|------------------------	|
| [4, 4, 3, 3] 	| -p --exact -w 40 -h 40 --halfplanes 34 0 34 0 30 0 30 0 -v 5D --rhull --potential 	| 458869678 	|
| [5, 3, 4, 2] 	| -p --exact -w 40 -h 40 --halfplanes 30 0 38 0 26 0 34 0 -v 5D --rhull --potential 	| 401310655 	|
| [5, 4, 4, 2] 	| -p --exact -w 40 -h 40 --halfplanes 34 0 38 0 26 0 34 0 -v 5D --rhull --potential 	| 375864127 	|
| [4, 4, 4, 2] 	| -p --exact -w 40 -h 40 --halfplanes 34 0 34 0 26 0 34 0 -v 5D --rhull --potential 	| 364860000 	|
| [4, 4, 4, 3] 	| -p --exact -w 40 -h 40 --halfplanes 34 0 34 0 30 0 34 0 -v 5D --rhull --potential 	| 348995665 	|
| [4, 4, 4, 1] 	| -p --exact -w 40 -h 40 --halfplanes 34 0 34 0 22 0 34 0 -v 5D --rhull --potential 	| 309573343 	|
| [4, 3, 4, 3] 	| -p --exact -w 40 -h 40 --halfplanes 30 0 34 0 30 0 34 0 -v 5D --rhull --potential 	| 292855965 	|
| [5, 4, 3, 3] 	| -p --exact -w 40 -h 40 --halfplanes 34 0 38 0 30 0 30 0 -v 5D --rhull --potential 	| 292077452 	|
| [5, 4, 1, 4] 	| -p --exact -w 40 -h 40 --halfplanes 38 0 34 0 22 0 34 0 -v 5D --rhull --potential 	| 250352851 	|
| [5, 4, 4, 1] 	| -p --exact -w 40 -h 40 --halfplanes 34 0 38 0 22 0 34 0 -v 5D --rhull --potential 	| 247872631 	|
| [5, 4, 3, 1] 	| -p --exact -w 40 -h 40 --halfplanes 34 0 38 0 22 0 30 0 -v 5D --rhull --potential 	| 225412609 	|
| [5, 4, 2, 3] 	| -p --exact -w 40 -h 40 --halfplanes 34 0 38 0 30 0 26 0 -v 5D --rhull --potential 	| 192579437 	|
| [5, 3, 3, 3] 	| -p --exact -w 40 -h 40 --halfplanes 30 0 38 0 30 0 30 0 -v 5D --rhull --potential 	| 175422024 	|
| [5, 4, 1, 2] 	| -p --exact -w 40 -h 40 --halfplanes 34 0 38 0 26 0 22 0 -v 5D --rhull --potential 	| 153208941 	|
| [5, 4, 1, 3] 	| -p --exact -w 40 -h 40 --halfplanes 38 0 34 0 22 0 30 0 -v 5D --rhull --potential 	| 153180138 	|
| [5, 3, 4, 1] 	| -p --exact -w 40 -h 40 --halfplanes 30 0 38 0 22 0 34 0 -v 5D --rhull --potential 	| 151909523 	|
| [5, 2, 4, 1] 	| -p --exact -w 40 -h 40 --halfplanes 26 0 38 0 22 0 34 0 -v 5D --rhull --potential 	| 146176140 	|
| [5, 4, 2, 4] 	| -p --exact -w 40 -h 40 --halfplanes 38 0 34 0 26 0 34 0 -v 5D --rhull --potential 	| 143636064 	|
| [5, 3, 4, 3] 	| -p --exact -w 40 -h 40 --halfplanes 30 0 38 0 30 0 34 0 -v 5D --rhull --potential 	| 133894421 	|
| [5, 2, 4, 2] 	| -p --exact -w 40 -h 40 --halfplanes 26 0 38 0 26 0 34 0 -v 5D --rhull --potential 	| 126863849 	|


The total computation time for 291 problems amounted to less than 3000 hours on a single core of a Linux computer equipped with
Intel Xeon 5620 @ 2.4 Ghz and 24 GB of RAM.  Approximately half of the computation time was spent on 20 most difficult instances listed in the table.  Computations were performed with Gurobi 6.05 and Gurobi 6.5 optimizers. We 
currently recalculate all results with Gurobi 6.5. 


The data in the table can be used for verification of results stated in [our paper on Morpion Solitaire 5D ](http://duch.mimuw.edu.pl/~henrykm/lib/exe/fetch.php?media=morpion5d.pdf). Let us consider a specific instances, namely the box [5,4,2,4] listed in row 18 of the table and an easy instance [4,2,0,2] not listed in this table which computes in under
60 seconds. 

```python
# verifier.py 
# a test program which runs the optimizer on a single instance

import os
import sys
from gurobipy import *
import time
import math

callback_interrupt = False
best_known_bound = 82

def gurobi_callback(model, where):
  global callback_interrupt
  
  if where == GRB.callback.MIP:
    sols = model.cbGet(GRB.callback.MIP_SOLCNT)
    bound = model.cbGet(GRB.callback.MIP_OBJBND)
        
    if sols > 0 and bound <= best_known_bound:
        callback_interrupt = True
        print "Stopping computation (feasible model and objective smaller than known bound)"
        model.terminate()

# a typical larger instance 
# os.system("./generator -o 5_4_2_4.lp  -p --exact -w 40 -h 40 --halfplanes 38 0 34 0 26 0 34 0 -v 5D --rhull --potential")
# a fast instance
os.system("./generator -o 4_2_0_2.lp  -p --exact -w 40 -h 40 --halfplanes 26 0 34 0 26 0 18 0 -v 5D --rhull --potential")


callback_interrupt = False
# a typical larger instance 
# model = read("5_4_2_4.lp")
# a fast instance
model = read("4_2_0_2.lp")

model.params.threads = 1
model.params.MIPfocus = 3
model.optimize(gurobi_callback)
```

In order to run the script execute it from the console:
```
> python verifier.py
```
This makes an ASCII-art drawing of the board and passes the linear problem to the optimizer.

# 485 - a new upper bound for Morpion Solitaire

## Introduction

This part of README applies to the article ["485 - a new upper bound for Morpion Solitaire"](http://www.mimuw.edu.pl/~henrykm/lib/exe/fetch.php?media=upper-bound-morpion.pdf) presented at CGW@IJCAI.  The respository contains two programs mentioned in the article: `octagons.cpp` and `generator.cpp`. 

They should compile on a pretty general system equipped with a C++ compiler. The compilation was tested on OS X and Linux machines. 

## Example of a relaxed program 

This example is mentioned on page 9 of the article. We assume that the user installed the [Gurobi solver](http://www.gurobi.com/). Other solvers can be used as well, but we systematically investigated the performance only for this solver.  

```
> ./generator -w 60 -h 60 -g 10 8 10 12 10 8 10 12 -p --nocross -r 100,100 --plusplus -v 5t
> gurobi_cl ResultFile=morpion.sol morpion_lpp.lp
```
Among other data we find on the standard output
```
Solved with barrier
Solved in 2497 iterations and 1.78 seconds
Optimal objective  5.868235294e+02
```

In the first five lines of the file `morpion.sol` we find

```
==> morpion.sol <==
# Objective value = 5.8682352941176305e+02
ln_15_27_D0 2.9411764705882798e-01
ln_15_27_D1 2.9411764705882798e-01
ln_15_27_D2 8.8235294117717356e-02
ln_15_28_D0 2.9411764705882237e-01
...
```

Figure 5 on page 9 was automatically generated from `morpion.sol`.
