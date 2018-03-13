#!/bin/sh

~/morpion-lpp/src/tools/pentasol2tikz.py 178.psol --scale 0.75 > 178.tex
~/morpion-lpp/src/tools/pentasol2tikz.py --scale 0.95 empty.psol > empty.tex
~/morpion-lpp/src/tools/pentasol2tikz.py --scale 0.95 1.psol > 1.tex
~/morpion-lpp/src/tools/pentasol2tikz.py --scale 0.95 2.psol > 2.tex
~/morpion-lpp/src/tools/pentasol2tikz.py --scale 0.95 3.psol > 3.tex
~/morpion-lpp/src/tools/pentasol2tikz.py bruneau.psol --scale 0.75 > bruneau.tex
~/morpion-lpp/src/tools/pentasol2tikz.py average.psol --dotsize 0.5 --nonumbers --scale 0.5 > average.tex
