/*
 * TODO:
 *  - unique save file
 *  - save best line
 *  - log file
 *  - quiet mode
 */

#include<iostream>
#include<random>
#include<vector>
#include<set>
#include<algorithm>
#include<chrono>
#include<string.h>
#include<iomanip>
#include<signal.h>

#include "fastexp.h"

#include<boost/program_options.hpp>
#include<boost/assign/list_of.hpp>
namespace po = boost::program_options;

#include "morpiongame.h"

/*
 * The root object holds the root search position.
 * We use it for performance: copying constructor is much faster for MorpionGame.
 */
MorpionGame root;

/*
 * Search options.
 */

struct SearchOptions {
	MorpionGame::Variant v;		// 5T or 5D
	int n;						// number of iterations at every level
	unsigned int l;				// number of levels
	unsigned int s;				// random seed
	bool standard;				// use standard adapt
	float alpha;				// alpha value
	bool extend;				// extend search when new sequences are found
	bool fastexp;				// use fast but lest precise exp() function			
	int octagon[8];				// distances of sides of octagon from center of the cross; 0 represents infinity
	bool symmetric;				// generate center-symmetric solutions
} opts;

std::ostream& operator<<(std::ostream& os, SearchOptions& s)
{
	os << "\033[1;34mSearch Options\033[0m" << std::endl << std::endl;

    os << "Variant: \033[1;33m" << s.v << "\033[0m" << std::endl;
    os << "Iterations: \033[1;33m" << s.n << "\033[0m" << std::endl;
    os << "Seed: \033[1;33m" << s.s << "\033[0m" << std::endl;
    os << "Number of levels: \033[1;33m" << s.l << "\033[0m" << std::endl;
    os << "Alpha: \033[1;33m" << std::fixed << std::setprecision(3) << s.alpha << "\033[0m" << std::endl;
    os << "Adapt: \033[1;33m" << (s.standard ? "moves" : "layers") << "\033[0m" << std::endl;
    os << "Extend: \033[1;33m" << (s.extend ? "yes" : "no") << "\033[0m" << std::endl;
    os << "Fast exp: \033[1;33m" << (s.fastexp ? "yes" : "no") << "\033[0m" << std::endl;
	os << "Octagon: \033[1;33m";
	for (int i = 0; i < 8; i++) { os << s.octagon[i] << " "; }
    os << "\033[0m" << std::endl;
    os << "Symmetric: \033[1;33m" << (s.symmetric ? "yes" : "no") << "\033[0m" << std::endl;

    os << std::endl;

	return os;
}

float e(const float x)
{
	if (opts.fastexp) {
		return fastexp(x);
	} else {
		return exp(x);
	}
}

/*
 * Search information.
 */

struct SearchState {
	std::chrono::steady_clock::time_point computation_begin;
	std::chrono::steady_clock::time_point computation_end;

	unsigned long long int simuls;

	std::mt19937_64 generator;

	MorpionGame::Sequence best;

	void init(SearchOptions& o)
	{
		best.init();
		computation_begin = std::chrono::steady_clock::now();
		generator.seed(o.s);
		simuls = 0;
	}
} state;

std::ostream& operator<<(std::ostream& os, SearchState& s)
{
	os << "\033[1;34mSearch State\033[0m" << std::endl << std::endl;

	long int elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>
								(std::chrono::steady_clock::now() - s.computation_begin).count();

	// TODO: Finished: yes/no

	os << "  Elapsed time: \033[1;33m" << std::fixed << std::setprecision(2)
			  << elapsed_time / 1000000.0 << "\033[0m s" << std::endl;

	double sps = (double) s.simuls / ((double)elapsed_time / 1000000.0);

	os << "  Simulations: " << s.simuls << " (" << sps << " per second)" << std::endl;

	os << "  Best line: " << s.best.length << std::endl;

	// TODO: how many iterations from each level are processed (make it work with extend)

	return os;
}

void saveLine(MorpionGame::Sequence &s, std::string ) // filename)
{
	MorpionGame simulation(root);

	for (unsigned int i = 0; i < s.length; i++) {
		simulation.MakeMove(s.mv[i]);		
	}

//	simulation.SaveMovesFile(simulation.GetResults(), filename);
}

/*
 * Handling of user interrupts (Ctrl+C)
 */
static volatile bool userInterrupt = false;
std::chrono::steady_clock::time_point sigint_time;

void SIGINTHandler(int) {
    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - sigint_time).count() >= 2) {
		std::cout << std::endl << opts << std::endl;
		std::cout << state << std::endl;
		std::cout << "\033[1;31mPress Ctrl+C again within 2 seconds to interrupt computation.\033[0m" << std::endl;	
		sigint_time = std::chrono::steady_clock::now();
	} else {
		std::cout << "\033[1;31mInterrupting computation by user request.\033[0m" << std::endl;	
		userInterrupt = true;
	}
}

/*
 * Probability weights table. It stores exp(adaptation weight of m) for each move m.
 */

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

	const float& operator[](int i) const {
		return w[i];
	}
};

/*
 * Single playout given probability weight table. Result is stored in passed sequence.
 */

void simulate(const Weights &w, MorpionGame::Sequence &l)
{
	state.simuls++; 
	if (state.simuls % 100000 == 0) { 
		std::cout << state.simuls << " simulations." << std::endl; 
	}

	l.init();

	MorpionGame simulation(root);

	while(simulation.Moves().length > 0) {
		float s = 0.0f;

        for (unsigned int i = 0; i < simulation.Moves().length; i++) {
            s += w[MorpionGame::goedel_number(simulation.Moves().mv[i])];
       	}

	 	std::uniform_real_distribution<> dis(0.0, s);
        float r = dis(state.generator);

		s = 0.0f;

		MorpionGame::Move chosen = simulation.Moves().mv[simulation.Moves().length-1]; // sometimes r would be greater than s!

        for (unsigned int i = 0; i < simulation.Moves().length; i++) {
           	s += w[MorpionGame::goedel_number(simulation.Moves().mv[i])];
			if (s >= r) {
				chosen = simulation.Moves().mv[i]; break;
			}
       	}

		l.mv[l.length++] = chosen;
		simulation.MakeMove(chosen);

		if (opts.symmetric) {
			l.mv[l.length++] = simulation.symmetric(chosen);
			simulation.MakeMove(simulation.symmetric(chosen));
		}
	}
}

/*
 * Probability weights adaptation. Standard way (gradient ascent move by move).
 */

void adapt(Weights &w, const MorpionGame::Sequence &l)
{
	MorpionGame simulation(root);

	for (unsigned int i = 0; i < l.length; i++) {
		const MorpionGame::Move &m = l.mv[i];

		float W = 0.0f;
        for (unsigned int j = 0; j < simulation.Moves().length; j++) {
            W += w[MorpionGame::goedel_number(simulation.Moves().mv[j])];
       	}

        for (unsigned int j = 0; j < simulation.Moves().length; j++) {
	    	w[MorpionGame::goedel_number(simulation.Moves().mv[j])] /= e(opts.alpha * 
						w[MorpionGame::goedel_number(simulation.Moves().mv[j])] / W);
	    }

		w[MorpionGame::goedel_number(m)] *= e(opts.alpha);

		simulation.MakeMove(m);
	}
}

/*
 * Probability weights adaptation. Non-standard way (gradient ascent layer by layer).
 */

long long int layer[MorpionGame::max_goedel_number];
long long int ln = 1;

void adapt_layers(Weights &w, const MorpionGame::Sequence& l)
{
	MorpionGame simulation(root);

	ln += 2;
	for (size_t i = 0; i < l.length; i++) {
		layer[MorpionGame::goedel_number(l.mv[i])] = ln;
	}

	for (unsigned int i = 0; i < l.length; i++) {
		//MorpionGame::Move &m = l.mv[i];

		if (layer[MorpionGame::goedel_number(l.mv[i])] < ln) continue;

		MorpionGame::Sequence mvs;

		float W = 0.0f; 
        for (unsigned int j = 0; j < simulation.Moves().length; j++) {
			if (layer[MorpionGame::goedel_number(simulation.Moves().mv[j])] == ln) {
				mvs.mv[mvs.length++] = simulation.Moves().mv[j];
				layer[MorpionGame::goedel_number(simulation.Moves().mv[j])]--;
			}

            W += w[MorpionGame::goedel_number(simulation.Moves().mv[j])];
       	}


		float invW = 1.0f / W;

        for (unsigned int j = 0; j < simulation.Moves().length; j++) {
	    	w[MorpionGame::goedel_number(simulation.Moves().mv[j])] *= e(-(opts.alpha * mvs.length *
					w[MorpionGame::goedel_number(simulation.Moves().mv[j])] * invW - 
					(layer[MorpionGame::goedel_number(simulation.Moves().mv[j])] == ln - 1 ? opts.alpha : 0)));
	    }
/*
        for (unsigned int j = 0; j < simulation.Moves().length; j++) {
	    	w[MorpionGame::goedel_number(simulation.Moves().mv[j])] /= e(opts.alpha * mvs.length *
					w[MorpionGame::goedel_number(simulation.Moves().mv[j])] * W - 
					(layer[MorpionGame::goedel_number(simulation.Moves().mv[j])] == ln - 1 ? opts.alpha : 0));
	    }
*/
		for (unsigned int j = 0; j < mvs.length; j++) {
			simulation.MakeMove(mvs.mv[j]);
		}	
	}
}

void report_layer_probabilities(Weights &w, MorpionGame::Sequence &l)
{
	MorpionGame simulation(root);

	ln += 2;
	for (size_t i = 0; i < l.length; i++) {
		layer[MorpionGame::goedel_number(l.mv[i])] = ln;
	}

	int lr = 1;

	for (unsigned int i = 0; i < l.length; i++) {
		//MorpionGame::Move &m = l.mv[i];

		if (layer[MorpionGame::goedel_number(l.mv[i])] < ln) continue;

		MorpionGame::Sequence mvs;

		float W = 0.0f, lW = 0.0f; 
        for (unsigned int j = 0; j < simulation.Moves().length; j++) {
			if (layer[MorpionGame::goedel_number(simulation.Moves().mv[j])] == ln) {
				mvs.mv[mvs.length++] = simulation.Moves().mv[j];
				layer[MorpionGame::goedel_number(simulation.Moves().mv[j])]--;
	            lW += w[MorpionGame::goedel_number(simulation.Moves().mv[j])];
			}

            W += w[MorpionGame::goedel_number(simulation.Moves().mv[j])];
       	}

		float prob = 1.0f;

        for (unsigned int j = 0; j < simulation.Moves().length; j++) {
			if (layer[MorpionGame::goedel_number(simulation.Moves().mv[j])] == ln - 1) {

				prob *= lW / W;
				lW -= w[MorpionGame::goedel_number(simulation.Moves().mv[j])];
				W -= w[MorpionGame::goedel_number(simulation.Moves().mv[j])];
			} else {
			}
	    }

		for (unsigned int j = 0; j < mvs.length; j++) {
			simulation.MakeMove(mvs.mv[j]);
		}	

		std::cout << " " << lr << ": " << std::fixed << std::setprecision(2) << (lr < 4 ? "\033[1;33m" : "") 
			<< prob * 100.0f << (lr < 4 ? "\033[0m" : "") << "% ";
		lr++;

		if (lr > 15) break;
	}
	std::cout << std::endl;
}

/*
 * NRPA
 */

void nrpa(int level, Weights &w, MorpionGame::Sequence &l)
{
	assert(level > 0);

	Weights wc(w);
	MorpionGame::Sequence nl; 

	for (int i = 0; i < opts.n; i++) {
		nl.init();

		if (level == 1) {
			simulate(wc, nl);	// replaces level 0 call
		} else {
			nrpa(level - 1, wc, nl);
		}

		if (nl.length >= l.length) {
			if (nl.length > l.length) {
				// log search information
				if (level > 2 && nl.length >= state.best.length * 0.9) {
					std::cout << "New best line length " << nl.length << " at level " << level << std::endl;
				}

				// extend search when new line is found
				if (opts.extend && nl.length >= state.best.length * 0.9) {
					i = 0;
				}
			}

			l = nl; 
		}

		// use chosen adaptation method
		if (opts.standard) {
			adapt(wc, l);
		} else {
			adapt_layers(wc, l);
		}

		// log search information
		if (level >= 4) {
			long int elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>
										(std::chrono::steady_clock::now() - state.computation_begin).count();

			double sps = (double) state.simuls / ((double)elapsed_time / 1000000.0);

			std::cout << "Iteration " << state.simuls << " at level " << level 
					  << " global best \033[1;33m" << state.best.length << "\033[0m time " 
				 	  << std::fixed << std::setprecision(2) << elapsed_time / 1000000.0 << "s, " 
					  << sps << " sps" << std::endl;

			report_layer_probabilities(wc,state.best);
		}

		// break on user interrupt
		if (userInterrupt) break;
	}

	// record best sequence
	if (l.length >= state.best.length) {
		state.best = l;
		if (l.length > state.best.length && state.best.length > (opts.v == MorpionGame::D5 ? 70 : 150)) {
			saveLine(l, std::to_string(state.best.length) + ".psol");
		}
	}
}

/*
 * Parsing of variant argument from the command line.
 */
void validate(boost::any& v, 
              std::vector<std::string> const& values,
              MorpionGame::Variant*,
              int)
{
    po::validators::check_first_occurrence(v);
    std::string const& s = po::validators::get_single_string(values);
  
    if (s == "5T" || s == "5t" || s == "t5" || s == "T5" || s == "T" || s == "t") {
        v = boost::any(MorpionGame::T5);
    } else if (s == "5D" || s == "5d" || s == "d5" || s == "D5" || s == "D" || s =="d") {
        v = boost::any(MorpionGame::D5);
    } else {
        throw po::validation_error(po::validation_error::invalid_option_value);
    }
}

/*
 * Main
 */

int main(int argc, char** argv)
{
    std::cout << "\033[1;34mMorpion Solitaire NRPA Solver.\033[0m" << std::endl << std::endl;

	/*
  	 * Interrupt search after Ctrl+C is pressed twice.
	 */
	signal(SIGINT, SIGINTHandler);
	sigint_time = std::chrono::steady_clock::now();

	/*
	 * Search options.
	 */
    po::options_description options("Allowed parameters");

	std::vector<int> hplanes = boost::assign::list_of(0)(0)(0)(0)(0)(0)(0)(0);

    options.add_options()
        ("help", "this help message")
		("print,p", "print the board")
        ("variant,v", po::value<MorpionGame::Variant>()->default_value(MorpionGame::T5), "variant (5T or 5D)")
		("iterations,n", po::value<int>()->default_value(100), "number of iterations per level")
		("levels,l", po::value<int>()->default_value(5), "number of levels")
		("seed,s", po::value<int>()->default_value(1), "random seed")
		("standard", po::value<bool>()->default_value(false), "use standard adapt")
		("alpha", po::value<float>()->default_value(1.0f), "value of alpha")
		("octagon,g", po::value<std::vector<int> >(&hplanes)->multitoken(), "use octagonal board")
		("fastexp", po::value<bool>()->default_value(true), "use fast but less accurate exp function")
		("extend", po::value<bool>() -> default_value(false), "extend search when new sequence is found")
		("clip,c", "clip board to default octagon for given variant")
		("symmetric", po::value<bool>() -> default_value(false), "generate center-symmetric solutions")
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

	// Record search options

	opts.v = vm["variant"].as<MorpionGame::Variant>();
	opts.n = vm["iterations"].as<int>();
	opts.l = vm["levels"].as<int>();
	opts.s = vm["seed"].as<int>();
	opts.alpha = vm["alpha"].as<float>();
	opts.fastexp = vm["fastexp"].as<bool>();
	opts.extend = vm["extend"].as<bool>();
	opts.standard = vm["standard"].as<bool>();
	opts.symmetric = vm["symmetric"].as<bool>();

	if (vm.count("clip")) {
		if (opts.v == MorpionGame::T5) {
//			hplanes = { 30, 48, 34, 56, 58, 76, 38, 32 };
			hplanes = { 26, 44, 30, 52, 54, 72, 34, 28 };
		} else {
			hplanes = { 26, 24, 34, 52, 38, 32, 30, 44 };
		}
	}

	for (int i = 0; i < 8; i++)
		opts.octagon[i] = hplanes[i];


	// Initialize root position

	root.variant = opts.v;
	root.clipBoard(opts.octagon);

	if (opts.symmetric) {
		root.clipAsymmetric();
	}

	if (vm.count("print")) {
		root.print(opts.octagon);
	}

	// Initialize search state

	state.init(opts);

	// Start search
	MorpionGame::Sequence l; l.init();
	Weights w;

	nrpa(opts.l, w, l);

	// Save and report search results
	state.computation_end = std::chrono::steady_clock::now();

	std::cout << state << std::endl;
	std::cout << opts << std::endl;

	return 0;
}

