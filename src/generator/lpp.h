#ifndef __LPP__
#define __LPP__

#include<vector>
#include<map>
#include<string>
#include<iostream>

/**
 * LinearCombination - a linear combination of std::strings with double coefficients.
 */
 
typedef std::vector<std::pair<std::string,double> > LinearCombination;

/**
 * Constraint represents a single linear (in)equality.
 */
 
class Constraint
{
public:

    enum Type { LT = 0, GT, EQ };

    LinearCombination linear;

private:

    Type type;
    double bound;
    std::string name;
            
public:
    void setName(std::string n) { name = n; }
    std::string getName() const { return name; }
    
    bool isEmpty() const
    {
        for (const std::pair<std::string, double> &p: linear) {
            if (p.second != 0.0) return false;
        }
        
        return true;
    }
    
    void addVariable(std::string v, double c) 
    {
        std::pair<std::string, double> a;
        
        for (std::pair<std::string, double> &p: linear) {
            if (p.first == v) {
                p.second += c;
                return;
            }
        }
        
        a.first = v;
        a.second = c;
        
        linear.push_back(a);
    }
    
    void setBound(double b) { bound = b; }
    void setType(Type t) { type = t; }
    
    const LinearCombination& getLinearCombination() const
    {
        return linear;
    }
    
    double getBound() const { return bound; }
    Type getType() const { return type; }
};

/**
 * Represents a solution of an LPP.
 */
 
class Solution
{
public:
    enum Status { OPT, FEAS, INFEAS, NOFEAS, UNBND, UNDEF } status;    
    std::map<std::string, double> valuation;
    double obj;
};

std::ostream& operator<<(std::ostream& os, const Solution& sol);

/**
 * Variable is a variable defined for an LPP together with constraints.
 */
 
class Variable
{
    std::string name;
    int goedel_number;
    double lo;
    double up;
    bool boolean;
    
    int ord;
    
public:
    Variable()
    {
        setName("unknown");
        setBoolean(false);
        setOrd(0);
    }
    
    Variable(std::string n, int g, double l = 0.0, double u = 1.0, bool b = false) {
        setName(n);
        setGoedelNumber(g);
        setUpperBound(u);
        setLowerBound(l);        
        setBoolean(b);
    }
    
    void setOrd(int o) { ord = o; }
    double getOrd() { return ord; }
    
    void setName(std::string n) { name = n; }
    std::string getName() const { return name; }
    
    void setGoedelNumber(int g) { goedel_number = g; }
    int getGoedelNumber() const { return goedel_number; }
    
    void setLowerBound(double l) { lo = l; }
    double getLowerBound() const { return lo; }
    
    void setUpperBound(double u) { up = u; }
    double getUpperBound() const { return up; }
    
    void setBoolean(bool b) { boolean = b;}
    bool getBoolean() const { return boolean; }
};

/**
 * Linear Programming Problem.
 */
 
class LPP
{
    std::vector<Constraint> constr;
    
    int last_var;
    std::map<std::string, Variable> vars;

    std::string comment;
    LinearCombination objective;
    
    void addVariables(const LinearCombination& lc)
    {
        for (const std::pair<std::string, double>& p: lc) {
            if (vars.find(p.first) == vars.end()) {
                vars[p.first] = Variable(p.first, last_var++);
            }
        }        
    }

protected:
    // TODO: move most methods from public part here
    
public:
    LPP()
    {
        last_var = 0;
    }

    friend std::ostream& operator<<(std::ostream& os, LPP& lpp);

    std::vector<Constraint> getConstraints() { return constr; }
    void addConstraint(const Constraint &c) { 
        if (c.isEmpty()) return;
        
        constr.push_back(c);
    }

    void addConstraints(std::vector<Constraint> v) {
        for (Constraint &c: v) {
            addConstraint(c);
        }
    }
    
    void setObjective(LinearCombination& obj) { objective = obj; }
    LinearCombination& getObjective() { return objective; }

    void setComment(std::string c) { comment = c; }
    std::string getComment() const { return comment; }
    
    void setVariableBounds(std::string v, double lo, double up)
    {
        if (vars.find(v) == vars.end()) {
            vars[v] = Variable(v, 0, lo, up);
        }
        
        vars[v].setUpperBound(up);
        vars[v].setLowerBound(lo);
    }
    
    void setVariableBoolean(std::string v, bool b)
    {
        if (vars.find(v) == vars.end()) {
            vars[v] = Variable(v, 0);
        }
        
        vars[v].setBoolean(b);
    }
    
    void setVariableOrd(std::string v, int o)
    {
        if (vars.find(v) == vars.end()) {
            vars[v] = Variable(v, 0);
        }
        
        vars[v].setOrd(o);
    }
    
    Solution solve_glpk();
    Solution solve_gurobi();
    
    Solution solve()
    {
        return solve_glpk();
    }

    void saveOrd(std::ostream&);
};


std::ostream& operator<<(std::ostream&, const LinearCombination&);
std::ostream& operator<<(std::ostream&, const Constraint&);
std::ostream& operator<<(std::ostream&, LPP&);

#endif
