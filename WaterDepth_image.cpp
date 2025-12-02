#include "WaterDepth_image.h"
#include "stb_image_write.h"
#include <algorithm>
#include <iostream>
using namespace std;


//// …[ƒ}ƒbƒv¶¬iƒOƒŒ[ƒXƒP[ƒ‹j
//void saveWaterDepthAsImage(const vector<vector<double>>& water, const string& filename) {
//    int height = water.size();
//    int width = water[0].size();
//
//    vector<unsigned char> image(width * height);
//
//    // Å‘å…[‚ğ’T‚µ‚Ä³‹K‰»
//    double maxDepth = 0.0;
//    for (const auto& row : water)
//        for (double d : row)
//            if (d > maxDepth) maxDepth = d;
//
//    cout << "maxDepth:" << maxDepth << "m\n";
//
//    if (maxDepth == 0.0) maxDepth = 1.0; // 0œZ–h~
//
//    for (int y = 0; y < height; ++y) {
//        for (int x = 0; x < width; ++x) {
//            double normalized = water[y][x] / 0.25; // ³‹K‰»******************************************************************5cm
//            image[y * width + x] = static_cast<unsigned char>(normalized * 255.0);
//        }
//    }
//
//    // …[‰æ‘œ
//    if (stbi_write_png(filename.c_str(), width, height, 1, image.data(), width)) {
//        cout << "…[‰æ‘œ•Û‘¶¬Œ÷: " << filename << "\n\n";
//    }
//    else {
//
//        cerr << "…[‰æ‘œ•Û‘¶¸”s: " << filename << endl;
//    }
//}


//// 3’iŠKƒJƒ‰[‰æ‘œ
//void saveWaterDepthAsImage(const std::vector<std::vector<double>>& water, const std::string& filename) {
//    int height = water.size();
//    int width = water[0].size();
//
//    // Å‘å…[‚ğæ“¾
//    double maxDepth = 0.0;
//    for (const auto& row : water) {
//        for (double d : row) {
//            if (d > maxDepth) maxDepth = d;
//        }
//    }
//      cout << "maxDepth:" << maxDepth << "m\n";
// 
// 
//    if (maxDepth == 0) maxDepth = 1; // ƒ[ƒœZ–h~
//
//    // o—Í‰æ‘œiRGBj
//    std::vector<unsigned char> image(width * height * 3);
//
//    for (int y = 0; y < height; ++y) {
//        for (int x = 0; x < width; ++x) {
//            double d = water[y][x] / 0.25; // 0?1‚É³‹K‰»
//            d = std::clamp(d, 0.0, 1.0);
//
//            // ƒOƒ‰ƒf[ƒVƒ‡ƒ“ió‚¢‚Ù‚Ç–¾‚é‚¢…F ¨ [‚¢‚Ù‚Ç”Z‚¢Âj
//            unsigned char r = static_cast<unsigned char>(173 * (1 - d));
//            unsigned char g = static_cast<unsigned char>(216 * (1 - d));
//            unsigned char b = static_cast<unsigned char>(230 * (1 - d) + 255 * d);
//
//            int index = (y * width + x) * 3;
//            image[index + 0] = r;
//            image[index + 1] = g;
//            image[index + 2] = b;
//        }
//    }
// 
// // …[‰æ‘œ
//if (stbi_write_png(filename.c_str(), width, height, 3, image.data(), width * 3)) {
//    cout << "…[‰æ‘œ•Û‘¶¬Œ÷: " << filename << "\n\n";
//}
//else {
//
//    cerr << "…[‰æ‘œ•Û‘¶¸”s: " << filename << endl;
// 
//
//}


// 5’iŠKƒJƒ‰[‰æ‘œ
void saveWaterDepthAsImage(const vector<vector<double>>& water, const string& filename) {
    int height = water.size();
    int width = water[0].size();

    vector<unsigned char> image(width * height * 3);

    // Å‘å…[‚ğæ“¾i³‹K‰»—pj
    double maxDepth = 0.0;
    for (const auto& row : water)
        for (double d : row)
            if (d > maxDepth) maxDepth = d;

    cout << "maxDepth:" << maxDepth << "m\n";

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double depth = water[y][x];
            double ratio = (maxDepth > 0) ? depth / 0.25 : 0.0;

            int r, g, b;

            if (ratio <= 0.2) {         // ‚²‚­ó‚¢
                r = 240; g = 248; b = 255;
            }
            else if (ratio <= 0.4) {  // ó‚¢
                r = 173; g = 216; b = 230;
            }
            else if (ratio <= 0.6) {  // ’†’ö“x
                r = 100; g = 149; b = 237;
            }
            else if (ratio <= 0.8) {  // ‚â‚â[‚¢
                r = 65;  g = 105; b = 225;
            }
            else {                    // [‚¢
                r = 0;   g = 0;   b = 139;
            }

            int idx = (y * width + x) * 3;
            image[idx + 0] = (unsigned char)r;
            image[idx + 1] = (unsigned char)g;
            image[idx + 2] = (unsigned char)b;
        }
    }

    // …[‰æ‘œ
    if (stbi_write_png(filename.c_str(), width, height, 3, image.data(), width * 3)) {
        cout << "…[‰æ‘œ•Û‘¶¬Œ÷: " << filename << "\n";
    }
    else {

        cerr << "…[‰æ‘œ•Û‘¶¸”s: " << filename << endl;
    }



}