#include "Render.h"
#include <Windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>
#include <iomanip>
#include <sstream>
#include "GUItextRectangle.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>

#ifdef _DEBUG
#include <Debugapi.h> 
#define PI 3.14159265358979323846
#define radius 1.95256

struct debug_print
{
    template<class C>
    debug_print& operator<<(const C& a)
    {
        OutputDebugStringA((std::stringstream() << a).str().c_str());
        return *this;
    }
} debout;
#else
#define PI 3.14159265358979323846
struct debug_print
{
    template<class C>
    debug_print& operator<<(const C& a)
    {
        return *this;
    }
} debout;
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;

bool texturing = true;
bool lightning = true;
bool alpha = false;

void switchModes(OpenGL* sender, KeyEventArg arg)
{
    auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));
    switch (key)
    {
    case 'L':
        lightning = !lightning;
        break;
    case 'T':
        texturing = !texturing;
        break;
    case 'A':
        alpha = !alpha;
        break;
    }
}

GuiTextRectangle text;
GLuint texId;

void initRender()
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    int x, y, n;
    unsigned char* data = stbi_load("texture.png", &x, &y, &n, 4);
    if (data)
    {
        unsigned char* _tmp = new unsigned char[x * 4];
        for (int i = 0; i < y / 2; ++i)
        {
            std::memcpy(_tmp, data + i * x * 4, x * 4);
            std::memcpy(data + i * x * 4, data + (y - 1 - i) * x * 4, x * 4);
            std::memcpy(data + (y - 1 - i) * x * 4, _tmp, x * 4);
        }
        delete[] _tmp;

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    else
    {
        debout << "Failed to load texture\n";
    }

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    camera.caclulateCameraPos();
    gl.WheelEvent.reaction(&camera, &Camera::Zoom);
    gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
    gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
    gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
    gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);

    gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
    gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
    gl.KeyUpEvent.reaction(&light, &Light::StopDrug);

    gl.KeyDownEvent.reaction(switchModes);
    text.setSize(512, 180);

    camera.setPosition(2, 1.5, 1.5);
}

struct Vertex {
    double x, y, z;
    Vertex(double x, double y, double z) : x(x), y(y), z(z) {}
};

const Vertex A(-0.5, 0.0, 0.0);
const Vertex A1(-0.5, 0.0, 2.0);
const Vertex B(-3.0, -2.0, 0.0);
const Vertex B1(-3.0, -2.0, 2.0);
const Vertex C(0.5, -4.0, 0.0);
const Vertex C1(0.5, -4.0, 2.0);
const Vertex D(2.0, -2.0, 0.0);
const Vertex D1(2.0, -2.0, 2.0);
const Vertex E(0.5, 0.0, 0.0);
const Vertex E1(0.5, 0.0, 2.0);
const Vertex F(2.5, 2.5, 0.0);
const Vertex F1(2.5, 2.5, 2.0);
const Vertex G(0.0, 4.5, 0.0);
const Vertex G1(0.0, 4.5, 2.0);
const Vertex H(-3.0, 2.0, 0.0);
const Vertex H1(-3.0, 2.0, 2.0);

void drawQuad(const Vertex& v1, const Vertex& v2, const Vertex& v3, const Vertex& v4) {
    Vertex edge1(v2.x - v1.x, v2.y - v1.y, v2.z - v1.z);
    Vertex edge2(v3.x - v1.x, v3.y - v1.y, v3.z - v1.z);

    Vertex normal(
        edge1.y * edge2.z - edge1.z * edge2.y,
        edge1.z * edge2.x - edge1.x * edge2.z,
        edge1.x * edge2.y - edge1.y * edge2.x
    );

    double length = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (length > 0) {
        normal.x /= length;
        normal.y /= length;
        normal.z /= length;
    }

    glBegin(GL_QUADS);
    glNormal3d(normal.x, normal.y, normal.z);
    glTexCoord2d(0, 0); glVertex3d(v1.x, v1.y, v1.z);
    glTexCoord2d(1, 0); glVertex3d(v2.x, v2.y, v2.z);
    glTexCoord2d(1, 1); glVertex3d(v3.x, v3.y, v3.z);
    glTexCoord2d(0, 1); glVertex3d(v4.x, v4.y, v4.z);
    glEnd();
}

void drawFigure() {
    if (lightning) {
        glEnable(GL_LIGHTING);
        glEnable(GL_NORMALIZE);
    }
    else {
        glDisable(GL_LIGHTING);
    }

    // Боковые грани
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    float side_mat[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, side_mat);

    drawQuad(A, B, B1, A1);
    drawQuad(B, C, C1, B1);
    drawQuad(C, D, D1, C1);
    drawQuad(D, E, E1, D1);
    drawQuad(E, F, F1, E1);
    drawQuad(F, G, G1, F1);
    drawQuad(G, H, H1, G1);
    drawQuad(H, A, A1, H1);

    // Верхняя грань (крышка)
    if (alpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    if (texturing) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);
    }

    float top_mat_ambient[] = { 0.5f, 0.5f, 0.3f, 1.0f };
    float top_mat_diffuse[] = { 1.0f, 1.0f, 0.8f, 1.0f };
    float top_mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, top_mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, top_mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, top_mat_specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

    // Рисуем верхнюю грань
    glBegin(GL_POLYGON);
    glNormal3d(0, 0, 1);
    glTexCoord2d(0.5, 0.5); glVertex3d(A1.x, A1.y, A1.z);
    glTexCoord2d(0.0, 0.0); glVertex3d(B1.x, B1.y, B1.z);
    glTexCoord2d(0.2, -0.2); glVertex3d(C1.x, C1.y, C1.z);
    glTexCoord2d(0.8, -0.2); glVertex3d(D1.x, D1.y, D1.z);
    glTexCoord2d(1.0, 0.0); glVertex3d(E1.x, E1.y, E1.z);
    glTexCoord2d(0.8, 0.8); glVertex3d(F1.x, F1.y, F1.z);
    glTexCoord2d(0.2, 1.0); glVertex3d(G1.x, G1.y, G1.z);
    glTexCoord2d(0.0, 0.8); glVertex3d(H1.x, H1.y, H1.z);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3d(0, 0, 1);
    glTexCoord2d(0.5, 0.5); glVertex3d(A1.x, A1.y, A1.z);
    glTexCoord2d(0.0, 0.0); glVertex3d(B1.x, B1.y, B1.z);
    glTexCoord2d(0.2, -0.2); glVertex3d(C1.x, C1.y, C1.z);
    glTexCoord2d(0.8, -0.2); glVertex3d(D1.x, D1.y, D1.z);
    glTexCoord2d(1.0, 0.0); glVertex3d(E1.x, E1.y, E1.z);
    glTexCoord2d(0.8, 0.8); glVertex3d(F1.x, F1.y, F1.z);

    // Добавляем текстуру на полуокружность между F1 и H1
    double center_x = (-3.0 + 0.0) / 2.0;
    double center_y = (2.0 + 4.5) / 2.0;
    for (int i = 0; i <= 36; ++i) {
        double angle = PI * i / 36;
        double x = center_x + radius * cos(angle);
        double y = center_y + radius * sin(angle);
        // Текстурные координаты для полуокружности
        glTexCoord2d(0.5 + 0.5 * cos(angle), 0.5 + 0.5 * sin(angle));
        glVertex3d(x, y, 2.0);
    }

    // Нижняя грань
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    float bottom_mat_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    float bottom_mat_diffuse[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, bottom_mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, bottom_mat_diffuse);

    glBegin(GL_POLYGON);
    glNormal3d(0, 0, -1);
    glTexCoord2d(0.5, 0.5); glVertex3d(A1.x, A1.y, A1.z);
    glTexCoord2d(0.0, 0.0); glVertex3d(B1.x, B1.y, B1.z);
    glTexCoord2d(0.2, -0.2); glVertex3d(C1.x, C1.y, C1.z);
    glTexCoord2d(0.8, -0.2); glVertex3d(D1.x, D1.y, D1.z);
    glTexCoord2d(1.0, 0.0); glVertex3d(E1.x, E1.y, E1.z);
    glTexCoord2d(0.8, 0.8); glVertex3d(F1.x, F1.y, F1.z);
    glTexCoord2d(0.2, 1.0); glVertex3d(G1.x, G1.y, G1.z);
    glTexCoord2d(0.0, 0.8); glVertex3d(H1.x, H1.y, H1.z);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3d(0, 0, -1);
    glVertex3d(A.x, A.y, A.z);
    glVertex3d(B.x, B.y, B.z);
    glVertex3d(C.x, C.y, C.z);
    glVertex3d(D.x, D.y, D.z);
    glVertex3d(E.x, E.y, E.z);
    glVertex3d(F.x, F.y, F.z);
    glVertex3d(G.x, G.y, G.z);
    glVertex3d(H.x, H.y, H.z);
    glEnd();

    int num_triangles = 180;

    // Включаем текстуру для полуокружности, если нужно
    if (texturing) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);
    }

    // Нижняя полуокружность с текстурой
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < num_triangles; i++) {
        double angle1 = atan2(2.0 - center_y, -3.0 - center_x) - (double)i / num_triangles * PI;
        double angle2 = atan2(2.0 - center_y, -3.0 - center_x) - (double)(i + 1) / num_triangles * PI;

        double x1 = center_x + radius * cos(angle1);
        double y1 = center_y + radius * sin(angle1);
        double x2 = center_x + radius * cos(angle2);
        double y2 = center_y + radius * sin(angle2);

        glColor3d(0.2, 0.9, 0.5);
        glVertex3d(x1, y1, 0.0);
        glVertex3d(x2, y2, 0.0);
        glVertex3d(center_x, center_y, 0.0);

    }
    glEnd();

    // Верхний треугольный веер
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < num_triangles; i++) {
        double angle1 = atan2(2.0 - center_y, -3.0 - center_x) - (double)i / num_triangles * PI;
        double angle2 = atan2(2.0 - center_y, -3.0 - center_x) - (double)(i + 1) / num_triangles * PI;

        double x1 = center_x + radius * cos(angle1);
        double y1 = center_y + radius * sin(angle1);
        double x2 = center_x + radius * cos(angle2);
        double y2 = center_y + radius * sin(angle2);

        glColor3d(0.0, 0.0, 0.0);
        glVertex3d(x1, y1, 2.0);
        glVertex3d(x2, y2, 2.0);
        glTexCoord2d(0.5 + 0.5 * cos(angle1), 0.5 + 0.5 * sin(angle1));
        glTexCoord2d(0.5 + 0.5 * cos(angle2), 0.5 + 0.5 * sin(angle2));
        glVertex3d(center_x, center_y, 2.0);
    }
    glEnd();

    // Боковые грани
    glBegin(GL_QUADS);
    glColor3d(0.1, 0.5, 0.9);
    for (int i = 0; i < num_triangles; i++) {
        double angle = atan2(2.0 - center_y, -3.0 - center_x) - (double)i / num_triangles * PI;
        double next_angle = atan2(2.0 - center_y, -3.0 - center_x) - (double)(i + 1) / num_triangles * PI;

        double x = center_x + radius * cos(angle);
        double y = center_y + radius * sin(angle);
        double nx = center_x + radius * cos(next_angle);
        double ny = center_y + radius * sin(next_angle);

        glVertex3d(x, y, 0.0);
        glVertex3d(x, y, 2.0);
        glVertex3d(nx, ny, 2.0);
        glVertex3d(nx, ny, 0.0);
    }
    glEnd();

    if (!texturing) {
        glDisable(GL_TEXTURE_2D);
    }
}

void Render(double delta_time) {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (gl.isKeyPressed('F')) {
        light.SetPosition(camera.x(), camera.y(), camera.z());
    }
    else {
        light.SetPosition(0.0f, 0.0f, 5.0f);
    }

    float light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    float light_diffuse[] = { 1.5f, 1.5f, 1.5f, 1.0f };
    float light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 50.0f);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01f);

    camera.SetUpCamera();
    light.SetUpLight();
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    drawFigure();
    light.DrawLightGizmo();

    // Отрисовка текста
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "T - " << (texturing ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"текстур" << std::endl;
    ss << "L - " << (lightning ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"освещение" << std::endl;
    ss << "A - " << (alpha ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"альфа-наложение" << std::endl;
    ss << L"F - Свет из камеры" << std::endl;
    ss << L"G - двигать свет по горизонтали" << std::endl;
    ss << L"G+ЛКМ двигать свет по вертекали" << std::endl;
    ss << L"Коорд. света: (" << std::setw(7) << light.x() << "," << std::setw(7) << light.y() << "," << std::setw(7) << light.z() << ")" << std::endl;
    ss << L"Коорд. камеры: (" << std::setw(7) << camera.x() << "," << std::setw(7) << camera.y() << "," << std::setw(7) << camera.z() << ")" << std::endl;
    ss << L"Параметры камеры: R=" << std::setw(7) << camera.distance() << ",fi1=" << std::setw(7) << camera.fi1() << ",fi2=" << std::setw(7) << camera.fi2() << std::endl;
    ss << L"delta_time: " << std::setprecision(5) << delta_time << std::endl;

    text.setPosition(10, gl.getHeight() - 10 - 180);
    text.setText(ss.str().c_str());
    text.Draw();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}