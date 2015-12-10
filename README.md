The code and this README applies to two papers: [An upper bound of 84 for Morpion Solitaire 5D ](http://duch.mimuw.edu.pl/~henrykm/lib/exe/fetch.php?media=morpion5d.pdf) and ["485 - a new upper bound for Morpion Solitaire"](http://www.mimuw.edu.pl/~henrykm/lib/exe/fetch.php?media=upper-bound-morpion.pdf) by Henryk Michalewski, Andrzej Nagórko and Jakub Pawlewicz.


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

These two steps are done simultaneously.


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
