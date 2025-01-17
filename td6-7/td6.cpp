#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <sys/timeb.h>

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLUT/glut.h>

#else
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#endif

#define concat(first, second) first second

#include "config.h"
#include "GLError.h"
#include "repere.h"

#define ENABLE_SHADERS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

repere rep(1.0);

unsigned int progid;
unsigned int mvpid;
unsigned int viewid;
unsigned int modelid;
unsigned int projid;


glm::mat4 model;
glm::mat4 view;
glm::mat4 proj;
glm::mat4 mvp;

float angle = 0.0f;
float scale = 0.0f;
float inc = 1.5f;
float yawRate = 0.0f;
float pitchRate = 0.0f;

unsigned int vaoids[1];

unsigned int nbtriangles;

float x, y, z;

std::array<float, 3> eye = {0.0f, 0.0f, 5.0f};

#define NBMESHES 4

struct shaderProg
{
    unsigned int progid; // ID du shader
    unsigned int mid;    // ID de la matrice de modelisation qui est passée en variable uniform au shader de vertex
    unsigned int vid;
    unsigned int pid;
    unsigned int LightID;
} shaders[NBMESHES];

struct maillage
{
    shaderProg shader;
    unsigned int vaoids; // VaoID qui contient les VBO où ce trouve ce maillage (et ses normales etc..) dans la VRAM
    unsigned int nbtriangles;
    float angle;
    float scale; // stocke l'opération de normalisation de la taille du maillage (réalisé lors de la lecture du maillage)
    float inc;
    float x, y, z; // stocke la position du centre du  maillage (réalisé lors de la lecture du maillage) il faut utiliser cette info lors du dessin afin de toujours veiller à recentrer le maillage en (0,0,0)
} maillages[NBMESHES];

const float YAW = -1.5707963f; //-90deg
const float PITCH = 0.0f;
const float SPEED = 0.0f;

struct camera
{
    glm::vec3 Position = {0.0f, 0.0f, 5.0f};
    glm::vec3 Front = {0.0f, 0.0f, -1.0f};
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp = {0.0f, 1.0f, 0.0f};
    // euler Angles
    float Yaw = YAW;
    float Pitch = PITCH;
    // camera options
    float MovementSpeed = SPEED;

} globalcamera;

void updateCameraVectors(camera *mycamera)
{
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(mycamera->Yaw) * cos(mycamera->Pitch);
    front.y = sin(mycamera->Pitch);
    front.z = sin(mycamera->Yaw) * cos(mycamera->Pitch);
    mycamera->Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    mycamera->Right = glm::normalize(glm::cross(mycamera->Front, mycamera->WorldUp)); // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    mycamera->Up = glm::normalize(glm::cross(mycamera->Right, mycamera->Front));
}

int getMilliCount()
{
    timeb tb;
    ftime(&tb);
    int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
    return nCount;
}

int getMilliSpan(int nTimeStart)
{
    int nSpan = getMilliCount() - nTimeStart;
    if (nSpan < 0)
        nSpan += 0x100000 * 1000;
    return nSpan;
}

int lastTime = 0;
double elapsed;

void calcTime()
{

    if (lastTime != 0)
    {

        elapsed = double(getMilliSpan(lastTime)) / 1000.0;
        globalcamera.Yaw += yawRate * elapsed;
        globalcamera.Pitch += pitchRate * elapsed;
        updateCameraVectors(&globalcamera);

        if (globalcamera.MovementSpeed != 0)
        {
            float velocity = globalcamera.MovementSpeed * elapsed;
            globalcamera.Position += globalcamera.Front * velocity;
        }
    }

    lastTime = getMilliCount();
}

void displayMesh(const maillage& mesh, const glm::mat4& modelMatrix)
{
    glUseProgram(mesh.shader.progid);

    glm::mat4 model = modelMatrix;
    model = glm::scale(model, glm::vec3(mesh.scale));
    model = glm::translate(model, glm::vec3(-mesh.x, -mesh.y, -mesh.z));

    glm::mat4 mvp = proj * view * model;

    glUniformMatrix4fv(mesh.shader.LightID, 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(mesh.shader.vid, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(mesh.shader.mid, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(mesh.shader.pid, 1, GL_FALSE, &proj[0][0]);

    glBindVertexArray(mesh.vaoids);

    glDrawElements(GL_TRIANGLES, mesh.nbtriangles * 3, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    glUseProgram(0);
}

void display() {
    calcTime();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    view = glm::lookAt(globalcamera.Position, globalcamera.Position + globalcamera.Front, globalcamera.Up);

    float decal = 1.0f;

    glm::vec3 translations[] = {
        glm::vec3(-decal, -decal, 0.0f),
        glm::vec3(decal, decal, 0.0f),
        glm::vec3(-decal, decal, 0.0f),
        glm::vec3(decal, -decal, 0.0f)
    };

    glm::vec3 rotationAxes[] = {
        glm::vec3(1.0f, 1.0f, 2.0f),
        glm::vec3(0.0f, 2.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 0.0f)
    };

    for (int i = 0; i < 4; ++i) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, translations[i]);
        model = glm::rotate(model, glm::degrees(maillages[i].angle), rotationAxes[i]);
        displayMesh(maillages[i], model);
    }

    glutSwapBuffers();
}


void idle()
{
    for (auto &maillage : maillages)
    {
        maillage.angle += 0.0001f;
        if (maillage.angle >= 360.0f)
        {
            maillage.angle = 0.0f;
        }
    }
    glutPostRedisplay();
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    proj = glm::perspective(45.0f, w / static_cast<float>(h), 0.1f, 100.0f);
}

void special(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_LEFT:
        yawRate = -inc;
        break;
    case GLUT_KEY_RIGHT:
        yawRate = inc;
        break;
    case GLUT_KEY_UP:
        pitchRate = inc;
        break;
    case GLUT_KEY_DOWN:
        pitchRate = -inc;
        break;
    }
    glutPostRedisplay();
}

void specialUp(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_LEFT:
        yawRate = 0.0f;
        break;
    case GLUT_KEY_RIGHT:
        yawRate = 0.0f;
        break;
    case GLUT_KEY_UP:
        pitchRate = 0.0f;
        break;
    case GLUT_KEY_DOWN:
        pitchRate = 0.0f;
        break;
    }
    glutPostRedisplay();
}

void key(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'z':
        globalcamera.MovementSpeed = inc;
        break;
    case 's':
        globalcamera.MovementSpeed = inc;
        break;
    }
    updateCameraVectors(&globalcamera);
    glutPostRedisplay();
}

void keyUp(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'z':
        globalcamera.MovementSpeed = 0.0f;
        break;
    case 's':
        globalcamera.MovementSpeed = 0.0f;
        break;
    case 'a':
        globalcamera.Yaw = 0.0f;
        break;
    case 'e':
        globalcamera.Yaw = 0.0f;
        break;
    }
    updateCameraVectors(&globalcamera);
    glutPostRedisplay();
}

maillage initVAOs(maillage &mesh, const std::string &chemin)
{
    unsigned int vboids[4];

    std::ifstream ifs(MY_SHADER_PATH + chemin);
    if (!ifs)
    {
        throw std::runtime_error("can't find the meshe!! Check the name and the path of this file? ");
    }

    std::string off;

    unsigned int nbpoints, tmp;

    ifs >> off;
    ifs >> nbpoints;
    ifs >> mesh.nbtriangles;
    ifs >> tmp;

    std::vector<float> vertices(nbpoints * 3);
    std::vector<float> colors(nbpoints * 3);
    std::vector<unsigned int> indices(mesh.nbtriangles * 3);
    std::vector<float> normals(nbpoints * 3);


    for (unsigned int i = 0; i < vertices.size(); ++i)
    {
        ifs >> vertices[i];
    }

    for (unsigned int i = 0; i < mesh.nbtriangles; ++i)
    {
        ifs >> tmp;
        ifs >> indices[i * 3];
        ifs >> indices[i * 3 + 1];
        ifs >> indices[i * 3 + 2];
    }

    /**
     * Calcul de la boîte englobante du modèle
     */
    float dx, dy, dz;
    float xmin, xmax, ymin, ymax, zmin, zmax;

    xmin = xmax = vertices[0];
    ymin = ymax = vertices[1];
    zmin = zmax = vertices[2];
    for (unsigned int i = 1; i < nbpoints; ++i)
    {
        if (xmin > vertices[i * 3])
            xmin = vertices[i * 3];
        if (xmax < vertices[i * 3])
            xmax = vertices[i * 3];
        if (ymin > vertices[i * 3 + 1])
            ymin = vertices[i * 3 + 1];
        if (ymax < vertices[i * 3 + 1])
            ymax = vertices[i * 3 + 1];
        if (zmin > vertices[i * 3 + 2])
            zmin = vertices[i * 3 + 2];
        if (zmax < vertices[i * 3 + 2])
            zmax = vertices[i * 3 + 2];
    }

    x = (xmin + xmax) / 2.0f;
    y = (ymin + ymax) / 2.0f;
    z = (zmin + zmax) / 2.0f;

    // calcul des dimensions de la boîte englobante

    dx = xmax - xmin;
    dy = ymax - ymin;
    dz = zmax - zmin;

    // calcul du coefficient de mise à l'échelle

    mesh.scale = 2.0f / fmax(dx, fmax(dy, dz));
    std::cout << mesh.scale;

    // Calcul des normales.
    for (std::size_t i = 0; i < indices.size(); i += 3)
    {
        auto x0 = vertices[3 * indices[i]] - vertices[3 * indices[i + 1]];
        auto y0 = vertices[3 * indices[i] + 1] - vertices[3 * indices[i + 1] + 1];
        auto z0 = vertices[3 * indices[i] + 2] - vertices[3 * indices[i + 1] + 2];

        auto x1 = vertices[3 * indices[i]] - vertices[3 * indices[i + 2]];
        auto y1 = vertices[3 * indices[i] + 1] - vertices[3 * indices[i + 2] + 1];
        auto z1 = vertices[3 * indices[i] + 2] - vertices[3 * indices[i + 2] + 2];

        auto x01 = y0 * z1 - y1 * z0;
        auto y01 = z0 * x1 - z1 * x0;
        auto z01 = x0 * y1 - x1 * y0;

        auto norminv = 1.0f / std::sqrt(x01 * x01 + y01 * y01 + z01 * z01);
        x01 *= norminv;
        y01 *= norminv;
        z01 *= norminv;

        normals[3 * indices[i]] += x01;
        normals[3 * indices[i] + 1] += y01;
        normals[3 * indices[i] + 2] += z01;

        normals[3 * indices[i + 1]] += x01;
        normals[3 * indices[i + 1] + 1] += y01;
        normals[3 * indices[i + 1] + 2] += z01;

        normals[3 * indices[i + 2]] += x01;
        normals[3 * indices[i + 2] + 1] += y01;
        normals[3 * indices[i + 2] + 2] += z01;
    }

    for (std::size_t i = 0; i < normals.size(); i += 3)
    {
        auto &x = normals[i];
        auto &y = normals[i + 1];
        auto &z = normals[i + 2];

        auto norminv = 1.0f / std::sqrt(x * x + y * y + z * z);

        x *= norminv;
        y *= norminv;
        z *= norminv;
    }

    check_gl_error();

    // Génération d'un Vertex Array Object contenant 3 Vertex Buffer Objects.
    glGenVertexArrays(1, &mesh.vaoids);
    glBindVertexArray(mesh.vaoids);

    // Génération de 4 VBO.
    glGenBuffers(4, vboids);

    // VBO contenant les sommets.

    glBindBuffer(GL_ARRAY_BUFFER, vboids[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // L'attribut in_pos du vertex shader est associé aux données de ce VBO.
    auto pos = glGetAttribLocation(mesh.shader.progid, "in_pos");
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos);
    check_gl_error();

    
    check_gl_error();
    // VBO contenant les indices.

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboids[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    check_gl_error();
    // VBO contenant les normales.

    glBindBuffer(GL_ARRAY_BUFFER, vboids[3]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);

    auto normal = glGetAttribLocation(mesh.shader.progid, "in_normal");
    glVertexAttribPointer(normal, 3, GL_FLOAT, GL_TRUE, 0, 0);
    glEnableVertexAttribArray(normal);
    check_gl_error();

    glBindVertexArray(0);

    mesh.angle = 0.0f;
    mesh.inc = 0.5f;
    mesh.x = x;
    mesh.y = y;
    mesh.z = z;

    return mesh;
}

shaderProg initShaders(const std::string &vert, const std::string &frag)
{
    unsigned int vsid, fsid;
    int status;
    int logsize;
    std::string log;

    std::ifstream vs_ifs(MY_SHADER_PATH + vert);
    std::ifstream fs_ifs(MY_SHADER_PATH + frag);

    auto begin = vs_ifs.tellg();
    vs_ifs.seekg(0, std::ios::end);
    auto end = vs_ifs.tellg();
    vs_ifs.seekg(0, std::ios::beg);
    auto size = end - begin;

    std::string vs;
    vs.resize(size);
    vs_ifs.read(&vs[0], size);

    begin = fs_ifs.tellg();
    fs_ifs.seekg(0, std::ios::end);
    end = fs_ifs.tellg();
    fs_ifs.seekg(0, std::ios::beg);
    size = end - begin;

    std::string fs;
    fs.resize(size);
    fs_ifs.read(&fs[0], size);

    vsid = glCreateShader(GL_VERTEX_SHADER);
    char const *vs_char = vs.c_str();
    glShaderSource(vsid, 1, &vs_char, nullptr);
    glCompileShader(vsid);

    // Get shader compilation status.
    glGetShaderiv(vsid, GL_COMPILE_STATUS, &status);

    if (!status)
    {
        std::cerr << "Error: vertex shader compilation failed.\n";
        glGetShaderiv(vsid, GL_INFO_LOG_LENGTH, &logsize);
        log.resize(logsize);
        glGetShaderInfoLog(vsid, log.size(), &logsize, &log[0]);
        std::cerr << log << std::endl;
    }

    fsid = glCreateShader(GL_FRAGMENT_SHADER);
    char const *fs_char = fs.c_str();
    glShaderSource(fsid, 1, &fs_char, nullptr);
    glCompileShader(fsid);

    // Get shader compilation status.
    glGetShaderiv(fsid, GL_COMPILE_STATUS, &status);

    if (!status)
    {
        std::cerr << "Error: fragment shader compilation failed.\n";
        glGetShaderiv(fsid, GL_INFO_LOG_LENGTH, &logsize);
        log.resize(logsize);
        glGetShaderInfoLog(fsid, log.size(), &logsize, &log[0]);
        std::cerr << log << std::endl;
    }

    progid = glCreateProgram();

    glAttachShader(progid, vsid);
    glAttachShader(progid, fsid);

    glLinkProgram(progid);

    glUseProgram(progid);

    mvpid = glGetUniformLocation(progid, "mvp");
    viewid = glGetUniformLocation(progid, "view");
    modelid = glGetUniformLocation(progid, "model");
    projid = glGetUniformLocation(progid, "proj");

    shaderProg shader = {progid, modelid, viewid, projid, mvpid};
    return shader;
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
#if defined(__APPLE__) && defined(ENABLE_SHADERS)
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_3_2_CORE_PROFILE);
#else

    glutInitContextVersion(3, 2);
    // glutInitContextVersion( 4, 5 );
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glewInit();
#endif

    glutInitWindowSize(640, 480);

    glutCreateWindow(argv[0]);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);
    glutKeyboardFunc(key);
    glutKeyboardUpFunc(keyUp);

    // Initialisation de la bibliothèque GLEW.
#if not defined(__APPLE__)
    glewInit();
#endif

    glEnable(GL_DEPTH_TEST);
    check_gl_error();

    shaders[0] = initShaders("/shaders/phong.vert.glsl", "/shaders/phong.frag.glsl");
    maillages[0].shader = shaders[0];
    initVAOs(maillages[0], "/meshes/space_shuttle2.off");

    shaders[1] = initShaders("/shaders/phong.vert.glsl", "/shaders/toon.frag.glsl");
    maillages[1].shader = shaders[1];
    initVAOs(maillages[1], "/meshes/space_station2.off");

    shaders[2] = initShaders("/shaders/phong.vert.glsl", "/shaders/phongVert.frag.glsl");
    maillages[2].shader = shaders[2];
    initVAOs(maillages[2], "/meshes/millenium_falcon.off");

    shaders[3] = initShaders("/shaders/phong.vert.glsl", "/shaders/phongRouge.frag.glsl");
    maillages[3].shader = shaders[3];
    initVAOs(maillages[3], "/meshes/rabbit.off");

    rep.init();
    check_gl_error();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glutMainLoop();

    return 0;
}
