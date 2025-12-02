#ifndef SIMULATE_H
#define SIMULATE_H

#include <vector>

// 外部定数の宣言
extern const double w; // 5mメッシュ（aaaaa)
extern const int dxc[8];
extern const int dyc[8];
extern const int dirCode[8]; // D8方向コード

using namespace std;

// シミュレーション関数の宣言
void simulateWaterFlow(
    vector<vector<double>>& water,
    const vector<vector<int>>& flowDir,
    const vector<vector<double>>& surface,
    int width, int height,
    double DT, double n = 0.03
);

#endif // SIMULATE_H