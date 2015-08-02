//
// Morpion Solitaire Linear Programming Problem
//

// TODO:
//
// * fix symmetric games with cross not put in the center (at least produce an error)
// * implement conversion from .mst to pentasol format
// * implement conversion from pentasol format to .mst
// * implement board masks

#include<iostream>
#include<fstream>

#include<boost/assign/list_of.hpp>

using namespace std;

#include "lpp.h"
#include "morpion.h"
#include "morpion_lpp.h"

namespace po = boost::program_options;

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
    } else if (s == "4T" || s == "4t" || s == "t4" || s == "T4") {
        v = boost::any(T4);
    } else if (s == "4D" || s == "4d" || s == "d4" || s == "D4") {
        v = boost::any(D4);
    }  else {
        throw po::validation_error(po::validation_error::invalid_option_value);
    }
}

int main(int argc, char** argv)
{
    std::string output_filename;
    
    std::cout << "\033[1;34mMorpion Solitaire Linear Programming Problem Generator.\033[0m" << endl << endl;

    //
    // Parse options
    //
    
    po::options_description options("Allowed parameters");
        
    // this should be the defaults for lengths but it is not accepted by the compiler
    // 11,9,9,11,11,9,9,11
    // 15,15,5,5,15,15,5,5
    // std::vector<int> lengths = boost::assign::list_of(10)(10)(10)(10)(10)(10)(10)(10); 
    // std::vector<int> lengths = boost::assign::list_of(11)(9)(9)(11)(11)(9)(9)(11);
    std::vector<int> lengths = boost::assign::list_of(15)(15)(5)(5)(15)(15)(5)(5);
    // int lengthsArr[8] = {10,10,10,10,10,10,10,10};
    // std::vector<int> lengths (lengthsArr, lengthsArr + sizeof(lengthsArr) / sizeof(lengthsArr[0]) );
    
    std::vector<int> hplanes = boost::assign::list_of(1)(1)(1)(1)(1)(1)(1)(1);
    
    options.add_options()
        ("help", "this help message")
        ("variant,v", po::value<Variant>()->default_value(T5), "variant (5T, 5D, 4T or 4D)")
        ("width,w", po::value<int>()->default_value(20), "width of the board")
        ("height,h", po::value<int>()->default_value(20), "height of the board")
        ("reference,r", po::value<Dot>()->default_value(Dot(0,0)), "reference point")
        ("output,o", po::value<std::string>()->default_value("morpion_lpp.lp"), "output file")
        ("symmetric", po::bool_switch()->default_value(false), "make the problem symmetric")
        ("acyclic", po::bool_switch()->default_value(false), "make the problem acyclic")
        ("dot-acyclic", po::bool_switch()->default_value(false), "make the problem acyclic by ordering dots")
        ("bacyclic", po::bool_switch()->default_value(false), "make the problem bool acyclic")
        ("exact", po::bool_switch()->default_value(false), "make the problem exact")
        ("binary-moves", po::bool_switch()->default_value(false), "make move variables binary")
        ("print,p", po::bool_switch()->default_value(false), "print board")
        ("octagon,g", po::value<std::vector<int> >(&lengths)->multitoken(), "lenghts of edges of the octagon")
        ("halfplanes", po::value<std::vector<int> >(&hplanes)->multitoken(), "specify the board by giving distances from the center of the cross to the boundaries of the half-planes")
        ("hull", po::bool_switch()->default_value(false), "force the board to be the convex hull of the solution")
        ("shape", po::value<std::string>(), "load shape of the board from a pentasol game")
        ("rim", po::value<int>()->default_value(0), "create rim of allowed dots of given thickness")
        ("short-cycles", po::bool_switch()->default_value(false), "eliminate short cycles from the solution")
        ("allowed,a", po::value<std::string>(), "read list of allowed dots from file")
        ("solve", po::bool_switch()->default_value(false), "solve the problem")
        ("graph", po::value<std::string>()->default_value("morpion_lpp.gv"), "graph the solution")
        ("extra,e", po::bool_switch()->default_value(false), "add extra constraints (experimental)")
        ("plusplus", po::bool_switch()->default_value(false), "create LPP to compute ++ variant")
        ("nocross", po::bool_switch()->default_value(false), "do not put cross on the board")
        ("cn", po::bool_switch()->default_value(false),"generate LPP to compute C(n)")
        ("n,n", po::value<int>()->default_value(103), "optional parameter (e.g. to C(n))")
        ("potential", po::bool_switch()->default_value(false), "create LPP to compute bound for potential in ++ variant")
        ("ord", po::bool_switch()->default_value(false), "create .ord file corresponding to the problem")
        ("rhull", po::bool_switch()->default_value(false),"force the solution to touch edges of the rectangular hull of the board")
        ("rside", po::bool_switch()->default_value(false),"force the solution to touch right edge of rectangular hull of the board")
        ("tiered", po::value<int>()->default_value(10), "create LPP with tiered moves")
    ; // for some unclear reasons it does not accept ->default_value(lengths)   

    po::variables_map vm;
    
    try {
        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);
    } catch( const std::exception& e) {
        cerr << "\033[1mError:\033[0m \033[1;31m" << e.what() << "\033[0m" << endl << endl;
        std::cout << options << endl;
        return 1;
    }
    
    if (vm.count("help")) {
        std::cout << options << endl;
        return 0;
    }

    //
    // Set up the board
    //
    
    Board b;
    
    b.setVariant(vm["variant"].as<Variant>());
    b.setWidth(vm["width"].as<int>());
    b.setHeight(vm["height"].as<int>());
    b.calculateCRef();
    
    if (vm["reference"].as<Dot>() == Dot(0,0)) {
        b.setReference(b.getCRef()); // the reference point was not specified
    } else {
        if (vm["nocross"].as<bool>()) {
            b.setCRef(vm["reference"].as<Dot>());
            b.setReference(Dot(0,0));
        } else {
            b.setReference(vm["reference"].as<Dot>());
        }
    }
    
    if (vm.count("octagon")) {
        b.setHull(lengths);
   }

    if (vm.count("halfplanes")) {
        b.setHalfplaneHull(hplanes);
    }
    
    b.setRim(vm["rim"].as<int>());
    b.putRim();
    
    if (vm.count("shape")) {
        throw std::string("not implemented");
    }

    if (vm.count("allowed")) {
        for (int x = 0; x < b.getWidth(); x++) {
            for (int y = 0; y < b.getHeight(); y++) {
                b.disallowDot(Dot(x,y));
            }
        }
        ifstream f;
        std::string allowed_fn = vm["allowed"].as<std::string>();
        f.open(allowed_fn.c_str());
        while (!f.eof()) {
            int x, y; char c;
            f >> x;
            f >> c;
            f >> y;
            
            b.allowDot(Dot(x,y));
        }
        f.close();
    }
    
    if (!vm["nocross"].as<bool>()) {
        b.putCross();
    } else {
        b.clear();
    }
        
    //
    // some sanity checks
    //
    
    if (!b.isCrossInBounds() && !vm["nocross"].as<bool>()) {
        cerr << "\033[1mError:\033[0m \033[1;31mReference point is out of bounds.\033[0m" << endl << endl;
        return 1;
    }

    if (lengths.size() != 8) {
        cerr << "\033[1mError:\033[0m \033[1;31m The octagon option requires eight numbers.\033[0m" << endl << endl;
        return 1;
    }

    if (b.getWidth() < 10 && !vm["nocross"].as<bool>()) {
        cerr << "\033[1mError:\033[0m \033[1;31mBoard width must be at least 10.\033[0m" << endl << endl;
        return 1;
    }
    if (b.getWidth() > b.getMaxWidth()) {
        cerr << "\033[1mError:\033[0m \033[1;31mBoard width must be at most " << b.getMaxWidth() << ".\033[0m" << endl << endl;
        return 1;
    }    
    if (b.getHeight() < 10 && !vm["nocross"].as<bool>()) {    
        cerr << "\033[1mError:\033[0m \033[1;31mBoard height must be at least 10.\033[0m" << endl << endl;
        return 1;
    }
    if (b.getHeight() > b.getMaxHeight()) {
        cerr << "\033[1mError:\033[0m \033[1;31mBoard width must be at most " << b.getMaxHeight() << ".\033[0m" << endl << endl;
        return 1;
    }

    //
    // Set up the problem
    //
    
    BaseMorpionLPP *p;
    
    if (vm["plusplus"].as<bool>()) {
        p = new PlusPlusLPP();
    } else if (vm["cn"].as<bool>()) {
        p = new CnLPP();
        ((CnLPP*) p) -> setN(vm["n"].as<int>());
    } else if (vm["potential"].as<bool>()) {
        p = new PotentialLPP();
    } else {
        p = new MorpionLPP();
    }
    
    p -> setBoard(b);
    
    //
    // Set up parameters for LPP
    //
    
    std::string flags[] = { "symmetric", "acyclic", "bacyclic", "dot-acyclic", "exact",
                            "binary-moves", "hull", "short-cycles", "extra", "rhull",
                            "rside" };
    for (const std::string& flag: flags) {
        p -> setFlag(flag, vm[flag].as<bool>());
    }
        
    std::string values[] = { "tiers" };
    for (const std::string& value: values) {
        p -> setValue(value, vm[value].as<int>());
    }
    
    
    output_filename = vm["output"].as<std::string>();
    
    if (!p -> getFlag("exact") && p -> getFlag("acyclic")) {
        std::cout << "\033[1;34mWarning:\033[0m\033[1;31m be aware of semantics of fuzzy acyclic problems." << endl << endl;
    }
    
    if (p -> getFlag("acyclic") && p -> getFlag("bacyclic")) {
        cerr << "\033[1mError:\033[0m \033[1;31mYou can't use --acyclic and --bacyclic simultaneously.\033[0m" << endl << endl;
        return 1;
    }

    if (p -> getFlag("dot-acyclic") && (p -> getFlag("acyclic") || p -> getFlag("bacyclic"))) {
        cerr << "\033[1mError:\033[0m \033[1;31mYou can't use --dot-acyclic with --acyclic or --bacyclic simultaneously.\033[0m" << endl << endl;
        return 1;
    }

    if (p -> getFlag("exact") && p -> getFlag("binary-moves")) {
        std::cout << "\033[1;34mWarning:\033[0m\033[1;31m --exact overwrites --binary-moves." << endl << endl;
    }
    
    //
    // Display summary of the problem
    //
    
    std::cout << "     Variant: \033[1;33m" << b.getVariant() << "\033[0m" << endl;
    std::cout << "   Symmetric: \033[1;33m" << (p -> getFlag("symmetric") ? "Yes" : "No") << "\033[0m" << endl;
    std::cout << "     Acyclic: \033[1;33m" << (p -> getFlag("acyclic") ? "Yes" : "No") << "\033[0m" << endl;
    std::cout << "   B-Acyclic: \033[1;33m" << (p -> getFlag("bacyclic") ? "Yes" : "No") << "\033[0m" << endl;
    std::cout << " Dot-Acyclic: \033[1;33m" << (p -> getFlag("dot-acyclic") ? "Yes" : "No") << "\033[0m" << endl;
    std::cout << "  Exact Hull: \033[1;33m" << (p -> getFlag("hull") ? "Yes" : "No") << "\033[0m" << endl;
    std::cout << "    Solution: \033[1;33m" << (p -> getFlag("exact") ? "Exact" : (p -> getFlag("binary-move") ? "Moves exact" : "Fuzzy")) << "\033[0m" << endl;
    std::cout << "       Board: \033[1;33m" << b.getWidth() << "x" << b.getHeight() << "\033[0m" << endl;
    std::cout << "   Reference: \033[1;33m" << (b.getReference() + Dot(1,1)) << (b.getReference() == b.getCRef() ? " (C)" : "") << "\033[0m" << endl;
    std::cout << "      Output: \033[1;33m" << output_filename << "\033[0m" << endl;
    std::cout << "         Rim: \033[1;33m" << b.getRim() << "\033[0m" << endl;
    std::cout << "Short Cycles: \033[1;33m" << (p -> getFlag("short-cycles") ? "Disallowed" : "Allowed") << "\033[0m" << endl;
    std::cout << endl;
    std::cout << "Use --help for help." << endl;
    std::cout << endl;

    // Print some statistics
        
    std::cout << "       Boundary: \033[1;33m" << b.boundary() << "\033[0m" << endl;
    std::cout << "Number of moves: \033[1;33m" << b.getMoveList().size() << "\033[0m" << endl;
    std::cout << endl;
    
    if (vm["print"].as<bool>()) {
        std::cout << p -> getBoard() << std::endl;
    }    
    
    //
    // Generate LPP
    //
    
    LPP* lpp = p -> getLPP();

    std::cout << endl << endl;
    
    if ((vm["solve"].as<bool>() || 0) && vm.count("graph")) {
        Solution sol = lpp -> solve();

        std::cout << endl << sol;  
        
        if (vm.count("graph")) {
//            sol.getGraph(vm["graph"].as<std::string>());      
        }
    } else {
        ofstream of;
        of.open(output_filename.c_str(), ios::trunc);
        of << *lpp;
        of.close();
    }

    if (vm["ord"].as<bool>()) {
        ofstream of;
        of.open((output_filename + ".ord").c_str(), ios::trunc);
        lpp -> saveOrd(of);
        of.close();
    }
    
    return 0;
}
