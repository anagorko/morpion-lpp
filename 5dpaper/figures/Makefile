all: small1.pdf small2.pdf empty.pdf 85.pdf 94.pdf uncon.pdf uncon_small.pdf uncon_big.pdf
	# Done
%.pdf: %.tex
	pdflatex $<
small1.tex: small.psol ../psol2tikz.py
	../psol2tikz.py small.psol > small1.tex
small2.tex: small.psol ../psol2tikz.py
	../psol2tikz.py --nonumbers --graph small.psol > small2.tex
empty.tex: empty.psol ../psol2tikz.py
	../psol2tikz.py empty.psol > empty.tex
clean:
	rm *.pdf *.aux *.log
85.tex: 85.psol ../psol2tikz.py
	../psol2tikz.py --nonumbers --graph 85.psol > 85.tex
85.psol: 85.sol ../sol2psol.py
	../sol2psol.py 85.sol > 85.psol
94.tex: 94.psol ../psol2tikz.py
	../psol2tikz.py --nonumbers --unordered 94.psol > 94.tex
94.psol: 94.sol ../sol2psol_unmarked.py
	../sol2psol_unmarked.py 94.sol > 94.psol
uncon.psol: uncon.sol ../sol2psol_unmarked.py
	../sol2psol_unmarked.py uncon.sol > uncon.psol
uncon.tex: uncon.psol ../psol2tikz.py
	../psol2tikz.py --nonumbers --unordered uncon.psol > uncon.tex
uncon_small.psol: uncon_small.sol ../sol2psol_unmarked.py
	../sol2psol_unmarked.py uncon_small.sol > uncon_small.psol
uncon_small.tex: uncon_small.psol ../psol2tikz.py
	../psol2tikz.py --nonumbers --unordered uncon_small.psol > uncon_small.tex
uncon_big.psol: uncon_big.sol ../sol2psol_unmarked.py
	../sol2psol_unmarked.py uncon_big.sol > uncon_big.psol
uncon_big.tex: uncon_big.psol ../psol2tikz.py
	../psol2tikz.py --nonumbers --unordered uncon_big.psol > uncon_big.tex
