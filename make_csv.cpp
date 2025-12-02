#include "make_csv.h"
#include <iostream>
#include <fstream>
using namespace std;

 //csv作ってみる
void makeCsv(const vector<vector<double>>& water) {
    ofstream file("water.csv");
    if (!file) {
        cerr << "ファイルを開けません\n";
    }
    for (size_t i = 0; i < water.size(); i++) {
        for (size_t j = 0; j < water[i].size(); j++) {
            file << water[i][j] << "\n";
            
        }
    }
    file.close();
}