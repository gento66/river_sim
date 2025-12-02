#ifndef MIX_IMAGE_H
#define MIX_IMAGE_H

#include <string>

using namespace std;

// ミックス画像の作成関数
void MixImage(
    const string& terrainFile,
    const string& waterFile,
    const string& outputFile
);

#endif // MIX_IMAGE_H