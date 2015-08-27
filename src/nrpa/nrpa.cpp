/*
 * TODO:
 *  -command line arguments: 
 *     number of iterations per level
 *     number of levels
 *     variant
 *	   octagon
 *     seed
 *  - copying constructor + its use in nrpa simulation and adapt (DONE)
 *  - benchmark
 *  - SIGINT handling
 *  - best line saving
 *  - handling of dominating moves
 *  - log file
 */

#include<iostream>
#include<random>
#include<vector>
#include<set>
#include <algorithm>
#include<string.h>

#include "morpiongame.h"

MorpionGame root;

long long int simuls = 0;

std::seed_seq random_seed( { 12 });
std::mt19937_64 generator(random_seed);

const float alpha = 1.0f;

const int bound = 485;

typedef long long int hash;

class Weights
{
	float w[MorpionGame::max_goedel_number];

public:
	Weights()
	{
		for (int i = 0; i < MorpionGame::max_goedel_number; i++) {
			w[i] = 1.0f;
		}
	}

	Weights(const Weights& _w)
	{
		memcpy(w, _w.w, sizeof(w));
	}

	Weights& operator=(const Weights& _w) {
		memcpy(w, _w.w, sizeof(w));
		return *this;
	}

	float& operator[](int i) {
		return w[i];
	}
};

struct line {
	unsigned int length;
	MorpionGame::Move mv[bound];
};

void init(line &l) {
	l.length = 0;
}

void simulate(line &l, Weights &w)
{
	simuls++; if (simuls % 100000 == 0) { std::cout << simuls << " simulations." << std::endl; }

	l.length = 0;

	MorpionGame simulation(root);

	while(!simulation.Moves().empty()) {
		float s = 0.0f;

	        for (const MorpionGame::Move& m: simulation.Moves()) {
	            s += exp(w[MorpionGame::goedel_number(m)]);
        	}

	 	std::uniform_real_distribution<> dis(0.0, s);
        	float r = dis(generator);

		s = 0.0f;

		MorpionGame::Move chosen;
	        for (const MorpionGame::Move& m: simulation.Moves()) {
	            	s += exp(w[MorpionGame::goedel_number(m)]);
			if (s >= r) {
				chosen = m; break;
			}
        	}
		l.mv[l.length++] = chosen;
		simulation.MakeMove(chosen);

	}
}

void adapt(line &l, Weights &w)
{
	MorpionGame simulation(root);

	Weights ww(w);

	for (unsigned int i = 0; i < l.length; i++) {
		MorpionGame::Move &m = l.mv[i];

		ww[MorpionGame::goedel_number(m)] *= exp(alpha);

		float W = 0.0f;

	        for (const auto &k: simulation.Moves()) {
	            W += w[MorpionGame::goedel_number(k)];
        	}


        	for (const auto &k: simulation.Moves()) {
	            ww[MorpionGame::goedel_number(k)] /= exp(alpha * 
						w[MorpionGame::goedel_number(k)] / W);
	        }

		simulation.MakeMove(m);		
	}

	w = ww;
}

line global_best;

void nrpa(int level, Weights &w, line &l)
{
	if (level == 0) {
		simulate(l, w);
		return;
	} else {
		line nl; 

		Weights wc(w);

		for (int i = 0; i < 100; i++) {
			init(nl);

			nrpa(level - 1, wc, nl);

			if (nl.length >= l.length) {
				if (nl.length > l.length && level > 1) {
					std::cout << "New best line length " << nl.length << " at level " << level << std::endl;
				}

				l = nl;
			}

			adapt(l, wc);

			if (level >= 3) {
				std::cout << "Iteration " << simuls << " at level " << level << " global best " << global_best.length << std::endl;
			}

		}
	}
	if (l.length > global_best.length) {
		global_best = l;
	}
}


int main()
{
	MorpionGame g;
	g.print();

	generator.seed(13);

	line l; init(l); init(global_best);

	Weights w;

	nrpa(4, w, l);

	std::cout << l.length << std::endl;

	return 0;
}

