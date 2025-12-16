#include "simulate.h"
#define _USE_MATH_DEFINES
#include <math.h>   // math.h のほうが確実
#include <iostream>
#include <algorithm>
#include <cmath>


// シミュレーション
void simulateWaterFlow(vector<vector<double>>& water, const vector<vector<int>>& flowDir, const vector<vector<double>>& surface, int width, int height, double DT, double n) { // nは粗度係数, dtは1ステップごとの時間変化？
    // 一時的な更新用マップ（新しい水量を入れておく）
    vector<vector<double>> nextWater = water; // 更新するための配列

    const double dist[8] = { 1.0, M_SQRT2, 1.0, M_SQRT2, 1.0, M_SQRT2, 1.0, M_SQRT2 };
    const double dx = 5.0;

    int overCapCount = 0;

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            double h = water[y][x]; // 水の高さ
            if (h <= 1e-6) continue; // 1μm未満は水なしとする（≒ 0.0 とみなす）

            // 1)各方向の傾斜を計算
            double slopes[8]; //傾斜
            double dhs[8]; //水高差
            double S_total = 0.0; //合計の傾斜
            int validDirs = 0; //有効な数をカウント
            for (int i = 0; i < 8; ++i) {
                int nx = x + dxc[i];
                int ny = y + dyc[i];

                // 境界チェック

                double dh = surface[y][x] - surface[ny][nx];
                dhs[i] = dh;
                if (dh > 0.0) {
                    double s = dh / (dx * dist[i]);
                    slopes[i] = s;
                    S_total += s;
                    ++validDirs;
                }
                else {
                    slopes[i] = 0.0;
                }
            }
            // pit or flatは滞留
            if (S_total <= 1e-12 || validDirs == 0) {
                continue;
            }


            // 2)各方向の比率を求め、セルの水を分配
            // depth_i = h * (slopes[i] / S_total)
            //double depthPot[8]; //流量の計算
            //double depthSum = 0.0; //総流量
            double caps[8]; //移動量の中継

            double Srep = S_total / validDirs; // 有効傾斜の平均値
            if (Srep < 1e-8) continue;

            double v = (1.0 / n) * pow(h, 2.0 / 3.0) * sqrt(Srep); // マニング

            double fraction = v * DT / dx;
            fraction = clamp(fraction, 0.0, 0.5);

            double Q_total = h * fraction;

            for (int i = 0; i < 8; ++i) {
                if (slopes[i] > 0.0)
                    caps[i] = Q_total * (slopes[i] / S_total);
                else
                    caps[i] = 0.0;
            }

            // 3)条件を加える必要があれば変更する


            // 4)nextwaterに適用
            for (int i = 0; i < 8; ++i) {
                if (caps[i] <= 0.0) continue;
                int nx = x + dxc[i];
                int ny = y + dyc[i];

                nextWater[ny][nx] += caps[i];
                nextWater[y][x] -= caps[i];

                // 補正：負になったら0にする（数値誤差対策）
                if (nextWater[y][x] < 0.0) {
                    // この場合は非常に稀。負になった分を0にしてカウント
                    nextWater[y][x] = 0.0;
                    overCapCount++;
                }
            }
        }
    }
    
    cout << "MD8_simple _overcap _count: " << overCapCount << "\n";
    
    water = nextWater;
    


        //************************************************************


//        // 流向に従って移動
//        int dir = flowDir[y][x];
//        int targetX = x;
//        int targetY = y;
//        // 流向コードに対応する方向を探す
//        for (int d = 0; d < 8; ++d) {
//            if (dir == dirCode[d]) {
//                targetX = x + dxc[d];
//                targetY = y + dyc[d];
//                break;
//            }
//        }
//        // 勾配S = tan(傾斜角)
//        //double S = tan(wa_slope[y][x] * PI / 180.0);
//        //if (S <= 0.0) S = 1e-6; // 平地でもわずかに流すようにする********************
//        // 
//        // 勾配 S を直接「水面差 / セル幅」で計算
//        double dh = surface[y][x] - surface[targetY][targetX];
//        if (dh <= 0.0) continue; // 水面が高い方向にだけ流す

//        double d = (abs(targetX - x) + abs(targetY - y) == 2) ? w * sqrt(2.0) : w; // 斜めかな
//        double S = dh / d;

//        // マニング公式
//        double A = h * w;// 流積（断面積）(m^2)           
//        double m = 2 * h + w;// 潤辺(m)            
//        double R = A / m;// 径深(m)
//        double v = (1.0 / n) * pow(R, 2.0 / 3.0) * sqrt(S);// 流速[m/s]
//        if (v < 0.001) v = 0.001;  // 小さすぎる流速は最低限に抑える


//        double Q = v * A * DT;// 移動する水量 Q[m^3/s] = v × A
//        double outFlow = Q / (w * w);// 水深の変化分 [m]

//        // 水の高さを超えないように制限（安全対策）******************************************理論的に正しければいらなそう
//        if (outFlow > h) {
//            outFlow = h;
//            overCapCount++;
//        }



//        if (outFlow > dh / 2) outFlow = dh / 2;



//        // 自分の水を減らし、流出先に加算
//        nextWater[y][x] -= outFlow;
//        nextWater[targetY][targetX] += outFlow;
//    }
//}
//cout << "overCapCount:" << overCapCount << "\n";
//// 結果を water に上書き
//water = nextWater;

    
}