#include<iostream>
#include<fstream>
#include<vector>

using namespace std;

/************************************************************
*
* max_a - maximal length of a single edge of the octagon
*
* max_a = 100 is safe value, as any octagon that does not fit
*   in 100x100 square must have potential at least 300
*
* a posteriori, after performing the computation,
*    we can see that max_a = 45 is enough
*
*************************************************************/

const int max_a = 100;

int main()
{
    int cnt = 0;

    ofstream of;
    of.open("octagons.sql", ios::trunc);
    
    for (int a1 = 0; a1 < max_a; a1++) {
    cout << "a1=" << a1 << endl;
    for (int a2 = 0; a2 < max_a; a2++) {
    for (int a3 = 0; a3 < max_a; a3++) {
    for (int a4 = 0; a4 < max_a; a4++) {
    for (int a5 = 0; a5 < max_a; a5++) {
    for (int a6 = 0; a6 < max_a; a6++) {
        // condition O2
        int a7 = a1+a2+a3-a6-a5;
        // condition O3
        int a0 = a3+a4+a5-a7-a1;
                
        if (a7 < 0 || a0 < 0) continue;

        int feasible = false;
        
        // condition O1
        int potential = 8+3*(a0+a2+a4+a6)+4*(a1+a3+a5+a7);

        //int area = (1+a7+a0+a1)*(a1+a2+a3+1) - a1*(a1+1)/2 - a3*(a3+1)/2 -
        //    a5*(a5+1)/2 - a7*(a7+1)/2;
        
        // octagon has two opposite corners
        bool two_corners = (a1 == 0 && a5 == 0) || (a3 == 0 && a7 == 0);

        // octagon has only one corner
        bool one_corner = (a1 == 0 || a3 == 0 || a5 == 0 || a7 == 0) && !two_corners;

        if (!one_corner && !two_corners && potential == 288) { feasible = true; }
        if (one_corner && potential == 290) { feasible = true; }
        if (two_corners && potential == 292) { feasible = true; }
        
        // if potential > 288 + modifier, 
        //   then the octagon cannot be a hull of a Morpion 5T position
        // if potential < 288 + modifier, 
        //   then the octagon is included in a feasible octagon with 
        //   potential = 288 + modifier
        
        if (!feasible) continue;

        // conditions O4, O5, O6
        if (a0+a1+a2 < 9) continue;
        if (a1+a2+a3 < 9) continue;
        if (a0+a1+a7 < 9) continue;

        // condition O7
        if (a0 < a2 || a0 < a4 || a0 < a6) continue;

        // condition O8
        if (a7 < a1) continue;

        //int w = a7+a0+a1;
        //int h = a1+a2+a3;

        cnt++;
        
        if (cnt % 1000 == 0) { cout << cnt << std::endl; }

        of << "INSERT INTO octagons VALUES ( " << cnt << ", " <<
            a0 << ", " << a1 << ", " << a2 << ", " << a3 << ", " <<
            a4 << ", " << a5 << ", " << a6 << ", " << a7 <<
            ", NULL, NULL, NULL);\n";
        
    }
    }
    }
    }
    }
    }

    of.close();
        
    return 0;
}
