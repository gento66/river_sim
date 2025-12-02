#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "mix_image.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include <iostream>
#include <vector>
#include <algorithm>



void MixImage(const string& terrainFile,
    const string& waterFile,
    const string& outputFile)
{
    int width1, height1, channels1;
    int width2, height2, channels2;

    // 地形画像を読み込み（RGBに変換）
    unsigned char* terrain = stbi_load(terrainFile.c_str(), &width1, &height1, &channels1, 3);
    unsigned char* water = stbi_load(waterFile.c_str(), &width2, &height2, &channels2, 3);

    if (!terrain || !water) {
        std::cerr << "画像の読み込みに失敗しました\n";
        if (terrain) stbi_image_free(terrain);
        if (water) stbi_image_free(water);
        return;
    }

    if (width1 != width2 || height1 != height2) {
        std::cerr << "画像サイズが一致しません\n";
        stbi_image_free(terrain);
        stbi_image_free(water);
        return;
    }

    std::vector<unsigned char> blended(width1 * height1 * 3);

    // ピクセルごとに合成
    for (int i = 0; i < width1 * height1; ++i) {
        int r_t = terrain[i * 3 + 0];
        int g_t = terrain[i * 3 + 1];
        int b_t = terrain[i * 3 + 2];

        int r_w = water[i * 3 + 0];
        int g_w = water[i * 3 + 1];
        int b_w = water[i * 3 + 2];

        // 水の透明度を青チャンネルに応じて計算
        double alpha = std::clamp(b_w / 255.0 * 0.8, 0.0, 0.8);

        // 合成
        blended[i * 3 + 0] = static_cast<unsigned char>((1 - alpha) * r_t + alpha * r_w);
        blended[i * 3 + 1] = static_cast<unsigned char>((1 - alpha) * g_t + alpha * g_w);
        blended[i * 3 + 2] = static_cast<unsigned char>((1 - alpha) * b_t + alpha * b_w);
    }

    // PNGで保存
    if (stbi_write_png(outputFile.c_str(), width1, height1, 3, blended.data(), width1 * 3)) {
        cout << "合成画像保存成功: " << outputFile << "\n\n";
    }
    else {
        cerr << "合成画像保存失敗: " << outputFile << "\n\n";
    }

    stbi_image_free(terrain);
    stbi_image_free(water);
}