// terrain_mesh_viewer.cpp
// Shows a shaded, perspective 3D mesh with wireframe overlay from a heightMap.
// Dependencies: glad, glfw, glm
// Compile & link accordingly.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <cassert>

using std::vector;
using std::cerr;
using std::cout;
using std::string;

// Vertex layout: position (vec3), normal (vec3), color (vec3)
struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float r, g, b;
};

static void glfw_error_cb(int err, const char* desc) { cerr << "GLFW Error: " << desc << "\n"; }
static void key_cb(GLFWwindow* w, int key, int sc, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(w, 1);
}

// shader helpers
static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, &log[0]);
        cerr << "Shader compile error: " << log << "\n";
        glDeleteShader(s);
        return 0;
    }
    return s;
}
static GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        string log(len, '\0');
        glGetProgramInfoLog(p, len, nullptr, &log[0]);
        cerr << "Program link error: " << log << "\n";
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

// GLSL: vertex + fragment (Phong-like diffuse + ambient, using normal & light dir)
static const char* vsSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec3 aColor;

out vec3 vNormal;
out vec3 vPos;
out vec3 vColor;

uniform mat4 uMVP;
uniform mat4 uModel;
void main(){
    vPos = vec3(uModel * vec4(aPos, 1.0));
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vColor = aColor;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* fsSrc = R"(
#version 330 core
in vec3 vNormal;
in vec3 vPos;
in vec3 vColor;
out vec4 FragColor;

uniform vec3 uLightDir; // expected normalized
uniform vec3 uViewPos;

void main(){
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir); // light direction toward surface
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * vColor;
    vec3 ambient = 0.25 * vColor;
    // simple specular (small)
    vec3 V = normalize(uViewPos - vPos);
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(R, V), 0.0), 16.0) * 0.15;
    vec3 color = ambient + diffuse + spec;
    FragColor = vec4(color, 1.0);
}
)";

// Build mesh: vertex positions (x,y,z) where x and y scaled to [-w/2, w/2], [-h/2,h/2] and z = height*heightScale.
// Compute normals using central differences on heightMap.
static void BuildMeshFromHeightMap(
    const vector<vector<double>>& heightMap,
    vector<Vertex>& outVerts,
    vector<unsigned int>& outIndices,
    float heightScale = 1.0f,
    float worldWidth = 1.0f) // how wide x axis is in world units relative to grid; y will preserve aspect
{
    assert(!heightMap.empty() && !heightMap[0].empty());
    int H = (int)heightMap.size();
    int W = (int)heightMap[0].size();

    // find min/max heights
    double minH = heightMap[0][0], maxH = heightMap[0][0];
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        double v = heightMap[y][x];
        if (v < minH) minH = v;
        if (v > maxH) maxH = v;
    }
    if (maxH == minH) maxH = minH + 1e-6;

    // world extents
    float aspect = (float)W / (float)H;
    float halfW = worldWidth * 0.5f;
    float halfH = halfW / aspect;

    // precompute normalized heights (for color) and z values (world)
    vector<vector<float>> zval(H, vector<float>(W));
    vector<vector<float>> hnorm(H, vector<float>(W));
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float norm = (float)((heightMap[y][x] - minH) / (maxH - minH));
        hnorm[y][x] = norm;
        zval[y][x] = norm * heightScale;
    }

    // compute normals by central differences on zval in grid-space (dx, dy)
    // assume grid spacing in world units: sx = worldWidth/(W-1), sy = (2*halfH)/(H-1)
    float sx = (2.0f * halfW) / float(W - 1);
    float sy = (2.0f * halfH) / float(H - 1);

    auto getZ = [&](int iy, int ix)->float {
        // clamp edges
        if (ix < 0) ix = 0; if (ix >= W) ix = W - 1;
        if (iy < 0) iy = 0; if (iy >= H) iy = H - 1;
        return zval[iy][ix];
        };

    outVerts.clear();
    outVerts.reserve(W * H);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float world_x = -halfW + (float)x * sx;
            float world_y = -halfH + (float)y * sy;
            float world_z = getZ(y, x);

            // central differences (dz/dx, dz/dy) in world units
            float dzdx = (getZ(y, x + 1) - getZ(y, x - 1)) / (2.0f * sx);
            float dzdy = (getZ(y + 1, x) - getZ(y - 1, x)) / (2.0f * sy);

            // normal vector (pointing up)
            glm::vec3 n(-dzdx, -dzdy, 1.0f);
            n = glm::normalize(n);

            float c = hnorm[y][x]; // grayscale color
            Vertex v;
            v.px = world_x; v.py = world_y; v.pz = world_z;
            v.nx = n.x; v.ny = n.y; v.nz = n.z;
            v.r = c; v.g = c; v.b = c;
            outVerts.push_back(v);
        }
    }

    // indices (triangulate grid: two triangles per cell)
    outIndices.clear();
    outIndices.reserve((W - 1) * (H - 1) * 6);
    for (int y = 0; y < H - 1; y++) {
        for (int x = 0; x < W - 1; x++) {
            unsigned int v0 = y * W + x;
            unsigned int v1 = v0 + 1;
            unsigned int v2 = v0 + W;
            unsigned int v3 = v2 + 1;
            // triangle v0,v2,v1  and v1,v2,v3  (consistent winding)
            outIndices.push_back(v0); outIndices.push_back(v2); outIndices.push_back(v1);
            outIndices.push_back(v1); outIndices.push_back(v2); outIndices.push_back(v3);
        }
    }
}

// Create GL buffers + VAO and upload data
static void CreateGLMesh(const vector<Vertex>& verts, const vector<unsigned int>& indices, GLuint& outVAO, GLsizei& outIndexCount, GLuint& outVBO, GLuint& outEBO) {
    outVAO = 0; outVBO = 0; outEBO = 0; outIndexCount = 0;
    glGenVertexArrays(1, &outVAO);
    glGenBuffers(1, &outVBO);
    glGenBuffers(1, &outEBO);

    glBindVertexArray(outVAO);

    glBindBuffer(GL_ARRAY_BUFFER, outVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // layout match: 0 = pos (vec3), 1 = normal (vec3), 2 = color (vec3)
    GLsizei stride = sizeof(Vertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, px));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, nx));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, r));

    glBindVertexArray(0);
    outIndexCount = (GLsizei)indices.size();
}

// Main function exposed to simulator:
// heightScale: multiply normalized height to get visible elevation (try 0.5..3.0)
// worldWidth: approximate width of terrain in world units (affects aspect)
void ShowTerrain3D_MeshStyle(const vector<vector<double>>& heightMap, float heightScale = 0.8f, float worldWidth = 2.0f, bool wireframe = true, bool autoRotate = false) {
    if (heightMap.empty() || heightMap[0].empty()) { cerr << "Empty heightMap\n"; return; }

    glfwSetErrorCallback(glfw_error_cb);
    if (!glfwInit()) { cerr << "glfwInit failed\n"; return; }

    // request core profile 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 700, "Terrain Mesh (shaded + wireframe)", nullptr, nullptr);
    if (!window) { cerr << "window create failed\n"; glfwTerminate(); return; }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_cb);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { cerr << "GLAD init failed\n"; glfwDestroyWindow(window); glfwTerminate(); return; }

    glEnable(GL_DEPTH_TEST);
    // optionally enable line smoothing (may be platform dependent)
    glEnable(GL_LINE_SMOOTH);

    // build verts/indices
    vector<Vertex> verts;
    vector<unsigned int> indices;
    // use normalized heights internally and multiply by heightScale
    BuildMeshFromHeightMap(heightMap, verts, indices, heightScale, worldWidth);

    // create GL buffers
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLsizei indexCount = 0;
    CreateGLMesh(verts, indices, VAO, indexCount, VBO, EBO);

    // create shader
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint program = linkProgram(vs, fs);
    glDeleteShader(vs); glDeleteShader(fs);
    if (!program) { cerr << "shader program failed\n"; /*cleanup*/ glDeleteBuffers(1, &VBO); glDeleteBuffers(1, &EBO); glDeleteVertexArrays(1, &VAO); glfwTerminate(); return; }

    // camera setup
    int H = (int)heightMap.size(), W = (int)heightMap[0].size();
    float aspect = (float)W / (float)H;
    // camera position: place at some distance looking at center
    float camDistance = std::max(worldWidth, worldWidth / aspect) * 1.8f;
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 camPos = glm::vec3(0.0f, -camDistance * 0.6f, camDistance * 0.7f); // behind & above
    glm::vec3 up(0.0f, 0.0f, 1.0f);

    // projection matrix
    int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);
    float fAspect = (fbh == 0 ? 1.0f : (float)fbw / (float)fbh);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), fAspect, 0.01f, 100.0f);

    // model transform: rotate to look more like map (X-right, Y-forward, Z-up)
    glm::mat4 model = glm::mat4(1.0f);
    // small pitch to make mesh stand out (optional)
    model = glm::rotate(model, glm::radians(15.0f), glm::vec3(1, 0, 0));

    // light direction (from NW-high)
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, -0.7f, -0.3f));

    // drawing state: wireframe overlay uses polygon mode
    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // draw shaded polygons first then overlay wireframe

    double startTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double t = glfwGetTime() - startTime;
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // camera rotation if autoRotate
        glm::vec3 cam = camPos;
        if (autoRotate) {
            float ang = (float)(t * 0.25); // radians per second
            // rotate cam position around Z axis
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), ang, glm::vec3(0, 0, 1));
            cam = glm::vec3(rot * glm::vec4(camPos, 1.0f));
        }

        glm::mat4 view = glm::lookAt(cam, center, up);
        glm::mat4 mvp = proj * view * model;

        // draw shaded surface
        glUseProgram(program);
        GLint locMVP = glGetUniformLocation(program, "uMVP");
        GLint locModel = glGetUniformLocation(program, "uModel");
        GLint locLight = glGetUniformLocation(program, "uLightDir");
        GLint locView = glGetUniformLocation(program, "uViewPos");
        glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(locLight, 1, glm::value_ptr(lightDir));
        glUniform3fv(locView, 1, glm::value_ptr(cam));

        // draw fill
        glBindVertexArray(VAO);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

        // overlay wireframe lines (slightly dark)
        if (wireframe) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f); // push fill slightly back to avoid z-fighting
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(0.6f);
            // set a simple uniform to tint lines darker: we don't have separate shader, so we can draw again with same shader but override color in vertex data:
            // easiest: draw again ? fragment shader uses per-vertex color, so we instead temporarily disable program lighting and draw lines with constant color:
            // For simplicity, use the same shader but pass a very dark lightDir so lines appear dark via shading; or just glColor is not available in core.
            // We'll draw lines by binding polygon mode and using glDrawElements: vertex colors are same (grayscale) but lines will be visible.
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        glBindVertexArray(0);
        glUseProgram(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    glDeleteProgram(program);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glfwDestroyWindow(window);
    glfwTerminate();
}

// ----------------- Example usage -----------------
// In your simulator code you can call ShowTerrain3D_MeshStyle(heightMap, heightScale, worldWidth, true, false);
// Example main (uncomment to test standalone):
/*
int main(){
    // small test heightmap
    vector<vector<double>> dem = {
        {0,0.2,0.4,0.6,0.2},
        {0.1,0.3,0.5,0.7,0.4},
        {0.2,0.4,0.6,0.8,0.5},
        {0.3,0.5,0.7,1.0,0.6},
        {0.1,0.2,0.4,0.3,0.1}
    };
    ShowTerrain3D_MeshStyle(dem, 0.6f, 2.0f, true, true);
    return 0;
}
*/

