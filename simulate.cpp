#include "simulate.h"
#include <iostream>
#include <cmath>


// シミュレーション
void simulateWaterFlow(vector<vector<double>>& water, const vector<vector<int>>& flowDir, const vector<vector<double>>& surface, int width, int height, double DT, double n) { // nは粗度係数, dtは1ステップごとの時間変化？
    // 一時的な更新用マップ（新しい水量を入れておく）
    vector<vector<double>> nextWater = water; // 更新するための配列

    int ok = 0;
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            double h = water[y][x]; // 水の高さ
            if (h <= 1e-6) continue; // 1μm未満は水なしとする（≒ 0.0 とみなす）

            // 流向に従って移動
            int dir = flowDir[y][x];
            int targetX = x;
            int targetY = y;
            // 流向コードに対応する方向を探す
            for (int d = 0; d < 8; ++d) {
                if (dir == dirCode[d]) {
                    targetX = x + dxc[d];
                    targetY = y + dyc[d];
                    break;
                }
            }
            // 勾配S = tan(傾斜角)
            //double S = tan(wa_slope[y][x] * PI / 180.0);
            //if (S <= 0.0) S = 1e-6; // 平地でもわずかに流すようにする********************
            // 
            // 勾配 S を直接「水面差 / セル幅」で計算
            double dh = surface[y][x] - surface[targetY][targetX];
            if (dh <= 0.0) continue; // 水面が高い方向にだけ流す

            double d = (abs(targetX - x) + abs(targetY - y) == 2) ? w * sqrt(2.0) : w; // 斜めかな
            double S = dh / d;

            // マニング公式
            double A = h * w;// 流積（断面積）(m^2)           
            double m = 2 * h + w;// 潤辺(m)            
            double R = A / m;// 径深(m)
            double v = (1.0 / n) * pow(R, 2.0 / 3.0) * sqrt(S);// 流速[m/s]
            if (v < 0.001) v = 0.001;  // 小さすぎる流速は最低限に抑える


            double Q = v * A * DT;// 移動する水量 Q[m^3/s] = v × A
            double outFlow = Q / (w * w);// 水深の変化分 [m]

            // 水の高さを超えないように制限（安全対策）******************************************理論的に正しければいらなそう
            if (outFlow > h) {
                outFlow = h;
                ok++;
            }



            // 自分の水を減らし、流出先に加算
            nextWater[y][x] -= outFlow;
            nextWater[targetY][targetX] += outFlow;
        }
    }
    cout << "overwater_h:" << ok << "\n";
    // 結果を water に上書き
    water = nextWater;
}