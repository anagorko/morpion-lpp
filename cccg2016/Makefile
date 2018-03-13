LATEXMK=latexmk

.PHONY: FORCE_MAKE

FINALPDF=cccg2016.pdf

ROSIN_PREFIXES=68 69 79 80 81
ROSIN82_NAMES=$(ROSIN_PREFIXES:%=82_%)
SOL_NAMES_IN=85 exmpl_bbox
PSOL_NAMES_IN=82 $(ROSIN82_NAMES) small $(SOL_NAMES_IN)
PSOLS=$(PSOL_NAMES_IN:%=data/%.psol)
DOTS=$(PSOLS:.psol=.dots)

all: $(FINALPDF)

# custom rules

data/%.psol: data/%.sol
	data/sol2psol.py $< >$@

$(ROSIN_PREFIXES:%=data/82_%.dots): data/82_%.dots: data/82.psol
	data/psol2dat.py --limit $* --suffix _$* $<

data/%.dots: data/%.psol
	data/psol2dat.py $<

$(FINALPDF): table.tex $(DOTS)

# MAIN LATEXMK RULE

# -pdf tells latexmk to generate PDF directly (instead of DVI).
# -pdflatex="" tells latexmk to call a specific backend with specific options.
# -use-make tells latexmk to call make for generating missing files.

# -interactive=nonstopmode keeps the pdflatex backend from stopping at a
# missing file reference and interactively asking you for an alternative.

%.pdf: %.tex FORCE_MAKE
	$(LATEXMK) -pdf -recorder -bibtex -use-make $<

clean:
	$(LATEXMK) -c

mrproper:
	$(LATEXMK) -C
	rm -f data/*.dots data/*.moves $(SOL_NAMES_IN:%=data/%.psol) *.nav *.snm
