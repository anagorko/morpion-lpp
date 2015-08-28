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
#include<algorithm>
#include<chrono>
#include<string.h>

#include<boost/program_options.hpp>
namespace po = boost::program_options;

#include "morpiongame.h"

std::chrono::steady_clock::time_point computation_begin;
std::chrono::steady_clock::time_point computation_end;

MorpionGame root;
long long int simuls = 0;

std::seed_seq random_seed( { 12 });
std::mt19937_64 generator(random_seed);

enum Variant { T5 = 0, D5 };

std::ostream& operator<<(std::ostream& os, Variant v)
{
    switch (v) {
        case D5: os << "5D"; break;
        case T5: os << "5T"; break;
    }
    
    return os;
}

void validate(boost::any& v, 
              std::vector<std::string> const& values,
              Variant*,
              int)
{
    po::validators::check_first_occurrence(v);
    std::string const& s = po::validators::get_single_string(values);
  
    if (s == "5T" || s == "5t" || s == "t5" || s == "T5" || s == "T" || s == "t") {
        v = boost::any(T5);
    } else if (s == "5D" || s == "5d" || s == "d5" || s == "D5" || s == "D" || s =="d") {
        v = boost::any(D5);
    } else {
        throw po::validation_error(po::validation_error::invalid_option_value);
    }
}

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
				long int elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - computation_begin).count();

				std::cout << "Iteration " << simuls << " at level " << level << " global best " << global_best.length << " time " << elapsed_time / 1000000 << "s" << std::endl;
			}

		}
	}
	if (l.length > global_best.length) {
		global_best = l;
	}
}


int main(int argc, char** argv)
{
    std::cout << "\033[1;34mMorpion Solitaire NRPA Solver.\033[0m" << std::endl << std::endl;

    po::options_description options("Allowed parameters");

    options.add_options()
        ("help", "this help message")
        ("variant,v", po::value<Variant>()->default_value(T5), "variant (5T or 5D)")
		("iterations,n", po::value<int>()->default_value(100), "number of iterations per level")
		("levels,l", po::value<int>()->default_value(4), "number of levels")
	;	

    po::variables_map vm;
    
    try {
        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);
    } catch( const std::exception& e) {
        std::cerr << "\033[1mError:\033[0m \033[1;31m" << e.what() << "\033[0m" << std::endl << std::endl;
        std::cout << options << std::endl;
        return 1;
    }
    
    if (vm.count("help")) {
        std::cout << options << std::endl;
        return 0;
    }

	root.print();

	generator.seed(33);

	line l; init(l); init(global_best);

	Weights w;

	computation_begin = std::chrono::steady_clock::now();
	nrpa(4, w, l);
	computation_end = std::chrono::steady_clock::now();

	std::cout << "Elapsed time: " << std::chrono::duration_cast<std::chrono::microseconds>(computation_end - computation_begin).count() / 1000000 << "s" << std::endl;

	std::cout << l.length << std::endl;

	return 0;
}

