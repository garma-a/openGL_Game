#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>
using namespace std;
struct Point { float x, y; };
enum TransformState { NONE, ROTATING, SCALING, TRANSLATING };
TransformState animState = NONE;
float rotationAngle = 0.0f;
float scaleX = 1.0f, scaleY = 1.0f;
float xPosition = 0.0f;
const float fixedYTranslation = -0.45f;

const float btnWidth = 0.4f;
const float btnHeight = 0.15f;
const float startX = 1.0f;
const float startY = 1.3f;

Point C = { -0.19f, 0.22f };
Point J = { 0.0868217183051f, -0.1648736175654f };
Point K = { 0.1791466626111f, 0.1704647152895f };
Point L = { 0.17f, 0.12f };
Point V = { -0.190603f, 0.104435f };
Point U = { -0.190379428f, 0.147325f };
Point M = { 0.1563837994214f, 0.0702369668246f };
Point N = { 0.1428833172613f, 0.0260031595577f };
Point O = { 0.1255255544841f, -0.034028436019f };
Point P = { 0.1083060602568f, -0.0906767814784f };
Point E = { -0.1918071081506f, -0.1261294538101f };
Point D = { 0.0149105199898f, 0.2009292586765f };
Point F = { 0.0900375217946f, 0.075448248813f };
Point G = { 0.209481255616f, -0.2818738463073f };
Point H = { 0.1973637753732f, -0.1485870330482f };
Point Q = { -0.4505797469309f, -0.1002713264674f };
Point R = { -0.418280326925f, -0.0240434096451f };
Point T = { -0.4101074332966f, -0.0473358791016f };
Point A1 = { -0.4260809102686f, -0.0387258427088f };
Point B1 = { -0.4074644156699f, -0.0105983755651f };
Point C1 = { -0.39f, -0.03f };
Point D1 = { -0.4502533573048f, -0.1125694607039f };
Point E1 = { -0.4453575129136f, -0.113638863681f };
Point F1 = { -0.1189261430209f, 0.3574302133705f };
Point G1 = { 0.113375691707f, 0.3487150076033f };
Point H1 = { -0.1277578013355f, 0.2543633318204f };
Point I1 = { 0.0011059228335f, 0.2565397081329f };
Point J1 = { -0.0361857297224f, 0.3252692692493f };
Point K1 = { -0.0837737651992f, 0.3063059826644f };
Point L1 = { 0.0332670788113f, 0.3294833329349f };
Point M1 = { 0.0538456887472f, 0.2978778552933f };

GLuint eveDisplayList;

void drawSegment(Point p1, Point p2) {
    glLineWidth(1.5f);
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINES);
    glVertex2f(p1.x, p1.y);
    glVertex2f(p2.x, p2.y);
    glEnd();
}

void drawSemicircle(Point p1, Point p2) {
    Point center = { (p1.x + p2.x) / 2,(p1.y + p2.y) / 2 };
    float radius = sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2)) / 2;
    float startAngle = atan2(p1.y - center.y, p1.x - center.x) * 180 / M_PI;

    glLineWidth(1.5f);
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINE_STRIP);
    for (float angle = startAngle;angle <= startAngle + 180;angle += 1.0f) {
        float rad = angle * M_PI / 180.0f;
        glVertex2f(center.x + radius * cos(rad), center.y + radius * sin(rad));
    }
    glEnd();
}

void drawEllipse(Point f1, Point f2, Point p, bool fill,
    float r, float g, float b, bool hover) {
    float d = sqrt(pow(f2.x - f1.x, 2) + pow(f2.y - f1.y, 2));
    float dist1 = sqrt(pow(p.x - f1.x, 2) + pow(p.y - f1.y, 2));
    float dist2 = sqrt(pow(p.x - f2.x, 2) + pow(p.y - f2.y, 2));
    float a = (dist1 + dist2) / 2.0f;
    float b_ellipse = sqrt(a * a - (d / 2.0f) * (d / 2.0f));
    Point center = { (f1.x + f2.x) / 2.0f,(f1.y + f2.y) / 2.0f };
    float angle = atan2(f2.y - f1.y, f2.x - f1.x);

    glPushMatrix();
    if (hover) glTranslatef(0.0f, 0.02f, 0.0f);
    glTranslatef(center.x, center.y, 0.0f);
    glRotatef(angle * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);

    if (fill) {
        glColor3f(r, g, b);
        glBegin(GL_POLYGON);
        for (int i = 0;i < 360;i += 2) {
            float rad = i * M_PI / 180.0f;
            glVertex2f(a * cos(rad), b_ellipse * sin(rad));
        }
        glEnd();
    }

    glLineWidth(1.5f);
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0;i < 360;i += 2) {
        float rad = i * M_PI / 180.0f;
        glVertex2f(a * cos(rad), b_ellipse * sin(rad));
    }
    glEnd();
    glPopMatrix();
}

void drawDownwardSemicircle(Point start, Point end) {
    Point center = { (start.x + end.x) / 2.0f,(start.y + end.y) / 2.0f };
    float radius = sqrt(pow(end.x - start.x, 2) + pow(end.y - start.y, 2)) / 2.0f;

    glLineWidth(1.5f);
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINE_STRIP);
    for (int angle = 180;angle <= 360;angle += 2) {
        float rad = angle * M_PI / 180.0f;
        glVertex2f(center.x + radius * cos(rad), center.y + radius * sin(rad));
    }
    glEnd();
}

void createEveDisplayList() {
    eveDisplayList = glGenLists(1);
    glNewList(eveDisplayList, GL_COMPILE);

    glColor3f(0.9f, 0.9f, 0.9f);
    drawSegment(V, C1);
    drawSegment(U, B1);
    drawSegment(J, P);
    drawSegment(P, O);
    drawSegment(O, N);
    drawSegment(N, L);
    drawSegment(L, K);
    drawSegment(C, E);
    drawSegment(A1, B1);
    drawSegment(A1, Q);
    drawSegment(B1, R);
    drawSegment(A1, R);
    drawSegment(C1, T);
    drawSegment(T, E1);
    drawSegment(E1, D1);
    drawSegment(D1, Q);

    drawEllipse(C, K, D, false, 0.9f, 0.9f, 0.9f, false);
    drawEllipse(F, G, H, true, 1.0f, 1.0f, 1.0f, true);
    drawEllipse(F1, G1, H1, false, 0.9f, 0.9f, 0.9f, false);

    drawEllipse(F1, G1, I1, true, 0.0f, 0.0f, 0.0f, false);
    drawEllipse(F1, J1, K1, true, 0.0f, 0.0f, 0.5f, false);
    drawEllipse(G1, L1, M1, true, 0.0f, 0.0f, 0.5f, false);

    drawDownwardSemicircle(J, E);

    glEndList();
}

void drawButton(float x, float y, const char* label, bool active) {
    glColor3f(active ? 0.8f : 0.6f, 0.6f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + btnWidth, y);
    glVertex2f(x + btnWidth, y - btnHeight);
    glVertex2f(x, y - btnHeight);
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(x + 0.05f, y - 0.1f);
    for (const char* c = label;*c != '\0';c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT && state == GLUT_DOWN) {
        float wx = (x / 800.0f) * 3.0f - 1.5f;
        float wy = 1.5f - (y / 600.0f) * 3.0f;

        if (wx > startX && wx < startX + btnWidth) {
            if (wy<startY && wy>startY - btnHeight) animState = ROTATING;
            else if (wy<startY - btnHeight * 1.2 && wy>startY - btnHeight * 2.2) animState = SCALING;
            else if (wy<startY - btnHeight * 2.4 && wy>startY - btnHeight * 3.4) animState = TRANSLATING;
        }
    }
}

void display() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1.5, 1.5, -1.5, 1.5);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRasterPos2f(startX, startY + 0.1f);
    string label = "Select transformation:";
    for (char c : label) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

    drawButton(startX, startY, "Rotation", animState == ROTATING);
    drawButton(startX, startY - btnHeight * 1.2, "Scaling", animState == SCALING);
    drawButton(startX, startY - btnHeight * 2.4, "Translation", animState == TRANSLATING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    switch (animState) {
    case ROTATING:
        glTranslatef(xPosition, fixedYTranslation, 0.0f);
        glRotatef(rotationAngle, 0.0f, 0.0f, 1.0f);
        rotationAngle += 0.5f;
        break;
    case SCALING:
        glTranslatef(xPosition, fixedYTranslation, 0.0f);
        scaleX = fabs(sinf(glutGet(GLUT_ELAPSED_TIME) / 500.0f)) * 0.5f + 0.5f;
        scaleY = scaleX;
        glScalef(scaleX, scaleY, 1.0f);
        break;
    case TRANSLATING:
        xPosition = sinf(glutGet(GLUT_ELAPSED_TIME) / 500.0f) * 0.5f;
        glTranslatef(xPosition, fixedYTranslation, 0.0f);
        break;
    default:
        glTranslatef(0.0f, fixedYTranslation, 0.0f);
        break;
    }

    glCallList(eveDisplayList);
    glutSwapBuffers();
    glutPostRedisplay();
}

void init() {
    createEveDisplayList();
    glutMouseFunc(mouse);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Eve Transformation Controls");
    gluOrtho2D(-1.5, 1.5, -1.5, 1.5);
    init();
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}