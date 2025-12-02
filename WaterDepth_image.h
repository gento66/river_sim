#ifndef WATERDEPTH_IMAGE_H
#define WATERDEPTH_IMAGE_H

#include <vector>
#include <string>

using namespace std;

// シミュレーション関数の宣言
void saveWaterDepthAsImage(
    const vector<vector<double>>& water,
    const string& filename
);


#endif // WATERDEPTH_IMAGE_H