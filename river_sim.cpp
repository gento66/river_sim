// D8の完成形だと思う
//

#define _CRT_SECURE_NO_WARNINGS
#include "stb_image_write.h"
#include "make_csv.h"
#include "WaterDepth_image.h"
#include "mix_image.h"
#include "simulate.h"



#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "tinyxml2.h"  // TinyXML2
#define PI 3.141592653589793
std::vector<std::pair<int, int>> riverCells;// 川のセルの座標を記録

const double w = 5.0; // 5mメッシュ（xmlから入手可能だ）

const int STEP = 5000; //ステップ数

const double DEPTH = 0.05; // 初期水深 (m)

const double DT = 0.1; // 1ステップ何秒であるか


using namespace std;
using namespace tinyxml2;

// 標高データ読み込み関数
vector<vector<double>> loadElevations(const string& filename, int width) {
    vector<vector<double>> elevations;

    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {//ファイルの読み込み
        cerr << "XML読み込み失敗: " << doc.ErrorStr() << endl;
        return elevations;
    }

    //標高データを取りに行く
    XMLElement* root = doc.FirstChildElement("Dataset");
    if (!root) {
        cerr << "Datasetが見つかりません\n";
        return elevations;
    }

    XMLElement* tupleList = root->FirstChildElement("DEM")
        ->FirstChildElement("coverage")
        ->FirstChildElement("gml:rangeSet")
        ->FirstChildElement("gml:DataBlock")
        ->FirstChildElement("gml:tupleList");

    if (!tupleList) {
        cerr << "tupleListが見つかりません\n";
        return elevations;
    }

    //標高データのテキストを全てゲトる
    const char* rawText = tupleList->GetText();
    if (!rawText) {
        cerr << "tupleListにテキストがありません\n";
        return elevations;
    }

    istringstream iss(rawText);//文字列をストリームにする？
    string token;
    vector<double> row;//1行分のデータ
    int count = 0;//現在の行数をカウント

    while (iss >> token) {//空白区切りで1単語ずつ取得する
        size_t comma = token.find(',');
        if (comma != string::npos) {//カンマが見つかったら
            string valueStr = token.substr(comma + 1);//文字列（数値）を取り出す

            try {
                double value = stod(valueStr);//文字列をdoubleに変換
                //if (value == -9999.0) value = 0.0; // iranaikamo
                row.push_back(value);//データを追加
                count++;

                if (count == width) {//1行分のデータを取得したら
                    elevations.push_back(row);//2次元ベクトルに追加
                    row.clear();
                    count = 0;//リセット
                }
            }
            catch (...) {
                cerr << "数値変換失敗: " << valueStr << endl;
            }
        }
    }

    return elevations;
}

// 水域データの処理(周りの平均)
void fillMissingElevations(vector<vector<double>>& data, int width, int height) {
    const double nodata = -9999.0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {




            if (data[y][x] == nodata) {
                riverCells.emplace_back(y, x); // 川のセルを記録

                double sum = 0.0;
                int count = 0;

                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int ny = y + dy;
                        int nx = x + dx;

                        if (ny < 0 || ny >= height || nx < 0 || nx >= width) continue;
                        if (ny == y && nx == x) continue;

                        double neighbor = data[ny][nx];
                        if (neighbor != nodata) {
                            sum += neighbor;
                            count++;
                        }
                    }
                }

                if (count > 0) {
                    data[y][x] = sum / count;
                }
                else {
                    data[y][x] = 0.0;  // 周囲にも有効データがない場合の代替値
                }
            }
        }
    }
}


//傾斜角を計算する関数
vector<vector<double>> makeSlope(const vector<vector<double>>& data, int width, int height) {
    vector<vector<double>> slope(height, vector<double>(width, 0.0));// 傾斜角の配列（初期値は0.0）

    double dx = 5.0;  // 東西方向のピクセル間隔（m）
    double dy = 5.0;  // 南北方向のピクセル間隔 (m)
    int ct = 0;

    for (int i = 1; i < height - 1; ++i) {
        for (int j = 1; j < width - 1; ++j) {

            /*
            //標高0が含まれる地点の処理
            if (data[i - 1][j - 1] == 0.0 || data[i - 1][j] == 0.0 || data[i - 1][j + 1] == 0.0 ||
                data[i][j - 1] == 0.0 || data[i][j] == 0.0 || data[i][j + 1] == 0.0 ||
                data[i + 1][j - 1] == 0.0 || data[i + 1][j] == 0.0 || data[i + 1][j + 1] == 0.0) {// 9ピクセル内に0があるときは

                slope[i][j] = 0.0; // 傾斜を0として扱う

                continue;
            }
            */

            double dzdx = ((data[i - 1][j - 1] + 2 * data[i][j - 1] + data[i + 1][j - 1]) - (data[i - 1][j + 1] + 2 * data[i][j + 1] + data[i + 1][j + 1])) / (8.0 * dx);
            double dzdy = ((data[i - 1][j - 1] + 2 * data[i - 1][j] + data[i - 1][j + 1]) - (data[i + 1][j - 1] + 2 * data[i + 1][j] + data[i + 1][j + 1])) / (8.0 * dy);

            double slope_rad = atan(sqrt(dzdx * dzdx + dzdy * dzdy));
            double slope_deg = slope_rad * 57.29578;  // ラジアン→度

            if (slope_deg == 0.0) { // 傾斜0のとき
                //slope_deg = 1.0; // 試しに
                ct += 1;
            }
            slope[i][j] = slope_deg; // データを格納
        }
    }
    cout << "slope_0:" << ct << "\n";
    return slope;
}

// 方位角を計算する関数
vector<vector<double>> makeAspect(const vector<vector<double>>& data, int width, int height) {
    vector<vector<double>> aspect(height, vector<double>(width, 0.0));// 方位角の配列（初期値は0.0）

    double dx = 5.0;  // 東西方向のピクセル間隔（m）
    double dy = 5.0;  // 南北方向のピクセル間隔 (m)


    for (int i = 1; i < height - 1; ++i) {
        for (int j = 1; j < width - 1; ++j) {

            /*
            //標高0が含まれる地点の処理
            if (data[i - 1][j - 1] == 0.0 || data[i - 1][j] == 0.0 || data[i - 1][j + 1] == 0.0 ||
                data[i][j - 1] == 0.0 || data[i][j] == 0.0 || data[i][j + 1] == 0.0 ||
                data[i + 1][j - 1] == 0.0 || data[i + 1][j] == 0.0 || data[i + 1][j + 1] == 0.0) {// 9ピクセル内に0があるときは

                aspect[i][j] = 360; // どうにかする（川のある地点の方位角をどう扱うか迷う）
                continue;
            }
            */

            double zx = ((data[i - 1][j + 1] + 2 * data[i][j + 1] + data[i + 1][j + 1]) -
                (data[i - 1][j - 1] + 2 * data[i][j - 1] + data[i + 1][j - 1])) / (8.0 * dx);

            double zy = ((data[i - 1][j - 1] + 2 * data[i - 1][j] + data[i - 1][j + 1]) -
                (data[i + 1][j - 1] + 2 * data[i + 1][j] + data[i + 1][j + 1])) / (8.0 * dy);

            double A = atan2(-zy, -zx) * 180.0 / PI;// 傾きと逆向きを正
            if (A < 0) A += 360.0;

            aspect[i][j] = A;// データを格納
        }
    }

    return aspect;
}

// D8の方向オフセット（右回り：E, SE, S, SW, W, NW, N, NE）
const int dxc[8] = { 1, 1, 0, -1, -1, -1,  0, 1 };
const int dyc[8] = { 0, 1, 1,  1,  0, -1, -1, -1 };
const int dirCode[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };// D8方向コード

// 流出方向（D8法）
vector<vector<int>> computeFlowDirection(const vector<vector<double>>& dem, int width, int height) {
    vector<vector<int>> flowDir(height, vector<int>(width, 0));

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            double centerElev = dem[y][x];
            double minElev = centerElev;
            int minDir = 0;

            // 8方向の隣接セルをチェック
            for (int d = 0; d < 8; ++d) {
                int nx = x + dxc[d];
                int ny = y + dyc[d];
                double neighborElev = dem[ny][nx];

                if (neighborElev < minElev) {
                    minElev = neighborElev;
                    minDir = dirCode[d];  // 最も低い方向のコードを記録
                }
            }

            flowDir[y][x] = minDir;  // 流向を記録（1,2,...,128 or 0）
        }
    }

    return flowDir;

}
// 水を配置***************************************************************************************************************************
vector<vector<double>> WaterDepth(int width, int height, double depth = DEPTH) {
    // 単位：m（0.01m = 1cm）
    vector<vector<double>> water(height, vector<double>(width, depth));

    return water;
}

// 標高 + 水深をする
vector<vector<double>> TotalHeight(const vector<vector<double>>& data, const vector<vector<double>>& water, int width, int height) {
    vector<vector<double>> surface(height, vector<double>(width, 0.0));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            surface[y][x] = data[y][x] + water[y][x];  // 標高 + 水深 = 水面の高さ
        }
    }

    return surface;
}



// 水 + 標高（グレースケール）これは標高に依存しすぎるため、あまり使えないかもね
void saveWaterDemImage(const vector<vector<double>>& surface, const string& filename) {
    int height = surface.size();
    int width = surface[0].size();

    // 最小・最大標高を調べる
    double minHeight = surface[0][0], maxHeight = surface[0][0];
    for (const auto& row : surface) {
        for (double h : row) {
            if (h < minHeight) minHeight = h;
            if (h > maxHeight) maxHeight = h;
        }
    }cout << "max:" << maxHeight << "min:" << minHeight << endl;

    // 標高をグレースケール値に変換
    vector<unsigned char> image(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double normalized = (surface[y][x] - minHeight) / (maxHeight - minHeight);//正規化
            image[y * width + x] = static_cast<unsigned char>(255.0 * normalized);;
        }
    }
    // 水標画像
    if (stbi_write_png(filename.c_str(), width, height, 1, image.data(), width)) {
        cout << "水標画像保存成功: " << filename << "\n\n";
    }
    else {

        cerr << "水標画像保存失敗: " << filename << endl;
    }

}


int main() {

    int c = 0;
    string xmlFile = "FG-GML-5438-01-14-DEM5A-20180226.xml";
    int width = 225;  // データの列数（今はまだ直接書き込んでいます）

    // 標高データ
    vector<vector<double>> data = loadElevations(xmlFile, width);

    int height = data.size();//データの行数を取得

    // 水域処理
    fillMissingElevations(data, width, height);

    // 川の部分を3m下げる（記録されたセルだけ）
    for (const auto& cell : riverCells) {
        int y = cell.first;
        int x = cell.second;
        data[y][x] -= 3.0;
    }

    // 傾斜データ
    vector<vector<double>> slope = makeSlope(data, width, height);

    // 方位データ
    vector<vector<double>> aspect = makeAspect(data, width, height);

    // 流出方向
    vector<vector<int>> flowDir = computeFlowDirection(data, width, height);

    // 全域に5cmの水を置く
    vector<vector<double>> water = WaterDepth(width, height);

    /*
    // 表示テスト
    for (const auto& row : data) {
        for (double d : row) {
            cout << fixed << setprecision(1) << d << " ";
            c++;
        }
        cout << endl;
    }
    cout << c << endl;
    */



    // 標高画像生成
    if (!data.empty()) {

        // 最小・最大標高を調べる
        double minHeight = data[0][0], maxHeight = data[0][0];
        for (const auto& row : data) {
            for (double h : row) {
                if (h != 0) {// 0はスキップ（グレースケールの時に都合が悪いため）
                    if (h < minHeight) minHeight = h;
                    if (h > maxHeight) maxHeight = h;
                }
            }
        }cout << "max:" << maxHeight << "min:" << minHeight << endl;
        // 標高をグレースケール値に変換
        vector<unsigned char> image(width * height);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                double h = data[y][x];
                unsigned char gray = 0;

                double normalized = (h - minHeight) / (maxHeight - minHeight);//正規化
                if (normalized < 0.0) normalized = 0.0;//マイナスのときは0として扱う

                gray = static_cast<unsigned char>(255.0 * normalized);
                image[y * width + x] = gray;
            }
        }
        // 画像ファイルとして保存
        if (stbi_write_png("image/dem_output.png", width, height, 1, image.data(), width)) {
            cout << "標高画像保存成功: dem_output.png\n";
        }
        else {
            cerr << "標高画像保存失敗\n";
        }
    }

    //傾斜画像生成
    if (!slope.empty()) {
        vector<unsigned char> slopeImage(width * height);

        // 最小・最大傾斜を調べる
        double min = slope[1][1], max = slope[1][1];
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                double h = slope[y][x];
                if (h < min) min = h;
                if (h > max) max = h;
            }
        }
        cout << "maxslope:" << max << "minslope:" << min << endl;



        //グレースケール変換
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                double angle = slope[y][x];

                if (angle < 0) angle = 0;// (不要)
                if (angle > 90) angle = 90;// (いらない)
                unsigned char gray = static_cast<unsigned char>((angle / max) * 255);// 正規化してグレースケール変換
                slopeImage[y * width + x] = gray;// 格納
            }
        }
        // 傾斜画像保存
        if (stbi_write_png("image/slope_output.png", width, height, 1, slopeImage.data(), width)) {
            cout << "傾斜画像保存成功: slope_output.png\n";
        }
        else {
            cerr << "傾斜画像保存失敗\n";
        }
    }

    //方位画像生成
    if (!aspect.empty()) {
        vector<unsigned char> aspectImage(width * height);

        // 最小・最大方位を調べる
        double min = aspect[1][1], max = aspect[1][1];
        for (const auto& row : aspect) {
            for (double h : row) {
                if (h < min) min = h;
                if (h > max) max = h;
            }
        }cout << "max:" << max << "min:" << min << "\n";



        // グレースケール変換
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                double angle = aspect[y][x];// 1ピクセルずつ取り出す

                unsigned char gray = static_cast<unsigned char>((1.0 - angle / 360.0) * 255.0);// 正規化してグレースケール変換
                aspectImage[y * width + x] = gray;// 格納
            }
        }
        // 方位画像保存
        if (stbi_write_png("image/aspect_output.png", width, height, 1, aspectImage.data(), width)) {
            cout << "方位画像保存成功: aspect_output.png\n\n";
        }
        else {
            cerr << "方位画像保存失敗\n\n";
        }
    }





    double rainfall_mm_per_hour = 50.0;     // 毎時
    double rainfall_rate = rainfall_mm_per_hour / 1000.0; // m/h に変換
    double dt = 0.2; // 秒（ステップの時間と一致させる）
    // ↓ 秒単位の雨量に変換
    double rainfall_per_step = rainfall_rate * (dt / 3600.0); // m/step

    // シミュレーション**********************************************************************************
    int steps = STEP;// ステップの数
    for (int t = 0; t < steps; ++t) {

        ////雨を追加
        //for (int y = 0; y < height; ++y) {
        //    for (int x = 0; x < width; ++x) {
        //        water[y][x] += rainfall_per_step;
        //    }
        //}

        if (t == 0) {
            // 水深高画像
            //string filename1 = "image/water_step_" + to_string(t + 1) + ".png";
            ostringstream oss;
            oss << "image/water_step_" << setw(4) << setfill('0') << (t) << ".png";
            string filename1 = oss.str();
            saveWaterDepthAsImage(water, filename1); //水深画像の生成します

            // 地形 + 水深 の画像
            string filename2 = "image2/mix_step_" + to_string(t) + ".png";
            MixImage("image/dem_output.png", filename1, filename2);
        }



        //更新処理
        vector<vector<double>> surface = TotalHeight(data, water, width, height); // totalHeight
        vector<vector<double>> wa_slope = makeSlope(surface, width, height); // 更新傾斜

        // 流出方向
        vector<vector<int>> waterDir = computeFlowDirection(surface, width, height);

        // 最小・最大傾斜を調べる
        double min = wa_slope[1][1], max = wa_slope[1][1];
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                double h = wa_slope[y][x];
                if (h < min) min = h;
                if (h > max) max = h;
            }
        }
        cout << t + 1 << ":maxslope:" << max << " minslope:" << min << endl;

        double totalWater = 0.0;
        for (const auto& row : water)
            for (double d : row) totalWater += d;
        cout << "Total water: " << totalWater << " m\n";

        simulateWaterFlow(water, waterDir, surface, width, height, DT);// simulation**************************************


        //if ((t + 1) >= 1000) {
        //    if ((t + 1) % 50 == 0) {
        //        // 水深高画像
        //        //string filename1 = "image/water_step_" + to_string(t + 1) + ".png";
        //        std::ostringstream oss;
        //        oss << "image/water_step_" << std::setw(4) << std::setfill('0') << (t + 1) << ".png";
        //        std::string filename1 = oss.str();
        //        saveWaterDepthAsImage(water, filename1); //水深画像の生成します

        //        // 地形 + 水深 の画像
        //        string filename2 = "image2/mix_step_" + to_string(t + 1) + ".png";
        //        MixImage("image/dem_output.png", filename1, filename2);
        //    }
        //}else {
        //    if ((t + 1) % 25 == 0) {
        //        // 水深高画像
        //        //string filename1 = "image/water_step_" + to_string(t + 1) + ".png";
        //        std::ostringstream oss;
        //        oss << "image/water_step_" << std::setw(4) << std::setfill('0') << (t + 1) << ".png";
        //        std::string filename1 = oss.str();
        //        saveWaterDepthAsImage(water, filename1); //水深画像の生成します

        //        // 地形 + 水深 の画像
        //        string filename2 = "image2/mix_step_" + to_string(t + 1) + ".png";
        //        MixImage("image/dem_output.png", filename1, filename2);
        //    }
        //}

        if ((t + 1) % 25 == 0) {
            if ((t + 1) > 1000 && (t + 1) % 2 != 0) continue;
            // 水深高画像
            //string filename1 = "image/water_step_" + to_string(t + 1) + ".png";
            ostringstream oss;
            oss << "image/water_step_" << setw(4) << setfill('0') << (t + 1) << ".png";
            string filename1 = oss.str();
            saveWaterDepthAsImage(water, filename1); //水深画像の生成します

            // 地形 + 水深 の画像
            string filename2 = "image2/mix_step_" + to_string(t + 1) + ".png";
            MixImage("image/dem_output.png", filename1, filename2);
        }


    }

    makeCsv(water); //水深だね


    //csv作ってみる
    ofstream file("data.csv");
    if (!file) {
        cerr << "ファイルを開けません\n";
    }
    for (size_t i = 0; i < data.size(); i++) {
        for (size_t j = 0; j < data[i].size(); j++) {
            file << data[i][j] << "\n";

        }
    }
    file.close(); 

    



    return 0;
}