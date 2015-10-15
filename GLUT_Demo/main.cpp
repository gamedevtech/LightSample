#include <cmath>
#include <vector>
#include <map>
#include <iostream>
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#include <sys/time.h>

#include "imgui.h"
#include "imgui_impl_glut.h"

#define WINFOW_TITLE    "Lightin demo"
#define WINDOW_WIDTH    800
#define WINDOW_HEIGHT   600
#define UPDATE_TIME     17 /* ((1 / 60) * 1000) rounded up */
#define X .525731112119133606
#define Z .850650808352039932

void Initialize();
void Shutdown();
void DoGUI();
void display();
void updateTimer(int windowId);
void reshape(int width, int height);
void keyDown(unsigned char key, int x, int y);
void keyUp(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void mouseDrag(int x, int y);
void mouseMove(int x, int y);

void glPerspective(float fov, float aspectRatio, float znear, float zfar);
void reshapeOrtho(int w, int h);
void reshapePerspective(int w, int h);
void (*activeReshape)(int,int) = reshapePerspective;

double GetMilliseconds();
void Normalize(GLfloat *a);
void DrawTriangle(GLfloat *a, GLfloat *b, GLfloat *c, int div, float r);
void DrawSphere(int ndiv, float radius=1.0);
void DrawPlane(int subdiv);

static GLfloat vdata[12][3] = {
    {-X, 0.0, Z}, {X, 0.0, Z}, {-X, 0.0, -Z}, {X, 0.0, -Z},
    {0.0, Z, X}, {0.0, Z, -X}, {0.0, -Z, X}, {0.0, -Z, -X},
    {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0}
};

static GLuint tindices[20][3] = {
    {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},
    {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},
    {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6},
    {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11}
};

static double prevTime = 0.0;
static float rotation = 270.0f;
static int glutWindowId = 0;

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(0,0);
    glutInitWindowSize(WINDOW_WIDTH , WINDOW_HEIGHT);
    glutCreateWindow(WINFOW_TITLE);
    reshape(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    glutWindowId = glutGetWindow();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseDrag);
    glutPassiveMotionFunc(mouseMove);
    glutIgnoreKeyRepeat(false); // Key is continually held down
    glutTimerFunc(UPDATE_TIME, updateTimer, glutGetWindow());
    
    Initialize();
    atexit(Shutdown);
    prevTime = GetMilliseconds();
    glutMainLoop();
    
    return 0;
}


void Shutdown() {
    ImGui_ImplGLUT_Shutdown();
}

float diffuseColor[] = {1.0f, 0.0f, 0.0f, 1.0f};
float lightDirection[] = {2.0f, 2.0f, 2.0f, 1.0f};
float eyePos[] = {-11.0f, 8.0f, -7.0f};
float target[] = {0.0f, 0.0f, 0.0f};
bool rotateModel = false;
float attenC = 1.0f;
float attenL = 0.0f;
float attenQ = 0.0f;

bool planeVisible = true;
float planeScale[] = {10.0f, 1.0f, 10.0f};
float planeTranslate[] = {0.0f, 0.0f, 0.0f};

bool sphereVisible = true;
float sphereScale[] = {1.0f, 1.0f, 1.0f};
float sphereTranslate[] = {0.0f, 3.0f, 0.0f};

void Initialize() {
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
 
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_CULL_FACE); // TODO: Enable?
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
 
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    const float noColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, noColor);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseColor);
    glLightfv(GL_LIGHT0, GL_SPECULAR, diffuseColor);
    glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);
    
    const float ambientColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
    const float globalDiffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    const float materialShine = 0.0f;
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambientColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, globalDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noColor);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, materialShine);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, attenC);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, attenL);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, attenQ);
    
    ImGui_ImplGLUT_Init();
}

void display() {
    glClearColor(0.5f, 0.6f, 0.7f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glPushMatrix();
    gluLookAt(eyePos[0], eyePos[1], eyePos[2], target[0], target[1], target[2], 0.0f, 1.0f, 0.0f);
    
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseColor);
    glLightfv(GL_LIGHT0, GL_SPECULAR, diffuseColor);
    
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, attenC);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, attenL);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, attenQ);
    
    glColor3f(1.0f, 1.0f, 1.0f);
    if (sphereVisible) {
        glPushMatrix();
        glTranslatef(sphereTranslate[0], sphereTranslate[1], sphereTranslate[2]);
        glScalef(sphereScale[0], sphereScale[1], sphereScale[2]);
        DrawSphere(5);
        glPopMatrix();
    }
    
    if (planeVisible) {
        glPushMatrix();
        glTranslatef(planeTranslate[0], planeTranslate[1], planeTranslate[2]);
        glScalef(planeScale[0], planeScale[1], planeScale[2]);
        DrawPlane(400);
        glPopMatrix();
    }
    
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(lightDirection[0], lightDirection[1], lightDirection[2]);
    glEnd();
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);
    glEnd();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    
    DoGUI();
    
    glPopMatrix();
    glutSwapBuffers();
}

void DoGUI() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoMove;
    glDisable(GL_LIGHTING);
    ImGui_ImplGLUT_NewFrame((int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y, 1.0f / 60.0f);
    
    static bool showLightConfig = true;
    static bool showAppConfig = true;
    bool directional = lightDirection[3] == 1.0f;
    static bool firstRun = true;
    
    if (firstRun) {
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(330,120));
    }
    ImGui::Begin("Light", &showLightConfig, window_flags);
    ImGui::ColorEdit3("Diffuse Color", (float*)&diffuseColor);
    float dir = lightDirection[3];
    ImGui::DragFloat3("Light Pos/Dir", lightDirection, 0.5f);
    lightDirection[3] = dir;
    
    ImGui::PushItemWidth(50.0f);
    if (ImGui::DragFloat("C", &attenC, 0.001f)) {
        if (attenC <= 0.0f) {
            attenC = 0.0f;
        }
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("L", &attenL, 0.001f)) {
        if (attenL <= 0.0f) {
            attenL = 0.0f;
        }
    }
    ImGui::SameLine();
    if (ImGui::DragFloat("Q", &attenQ, 0.001f)) {
        if (attenQ <= 0.0f) {
            attenQ = 0.0f;
        }
    }
    ImGui::SameLine();
    ImGui::Text("Attenuation");
    ImGui::PopItemWidth();
    if (ImGui::Checkbox("Point", &directional)) {
        if (directional) {
            lightDirection[3] = 1.0f;
        }
        else {
            lightDirection[3] = 0.0f;
        }
    }
    ImGui::End();
    
    
    if (firstRun) {
        ImGui::SetNextWindowPos(ImVec2(350,10));
        ImGui::SetNextWindowSize(ImVec2(310,100));
    }
    ImGui::Begin("Camera", &showLightConfig, window_flags);
    ImGui::DragFloat3("Eye Position", eyePos, 0.25f);
    ImGui::DragFloat3("Target", target, 0.25f);
    ImGui::Checkbox("Rotate View", &rotateModel);
    ImGui::End();
    
    if (firstRun) {
        ImGui::SetNextWindowPos(ImVec2(670,10));
        ImGui::SetNextWindowSize(ImVec2(120,80));
    }
    ImGui::Begin("Application", &showAppConfig, window_flags);
    
    if (ImGui::Button("Quit")) {
        exit(0);
    }
    if (ImGui::Button("Reset")) {
        exit(0);
    }
    ImGui::End();
    
    if (firstRun) {
        ImGui::SetNextWindowPos(ImVec2(10, 140));
        ImGui::SetNextWindowSize(ImVec2(330,170));
    }
    ImGui::Begin("Scene", &showLightConfig, window_flags);
    ImGui::Checkbox("Plane", &planeVisible);
    
    ImGui::Indent();
    ImGui::PushID(900);
    ImGui::DragFloat3("Scale", planeScale, 0.5f);
    ImGui::PopID();
    ImGui::Unindent();
    
    ImGui::Indent();
    ImGui::PushID(1000);
    ImGui::DragFloat3("Translate", planeTranslate, 0.5f);
    ImGui::PopID();
    ImGui::Unindent();
    
    ImGui::Checkbox("Sphere", &sphereVisible);
    
    ImGui::Indent();
    ImGui::PushID(1100);
    ImGui::DragFloat3("Scale", sphereScale, 0.5f);
    ImGui::PopID();
    ImGui::Unindent();
    
    ImGui::Indent();
    ImGui::PushID(1200);
    ImGui::DragFloat3("Translate", sphereTranslate, 0.5f);
    ImGui::PopID();
    ImGui::Unindent();

    ImGui::End();
    
    
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    ImGui::Render();
    glEnable(GL_LIGHTING);
    firstRun = false;
}

void updateTimer(int windowId) {
    double curTime = GetMilliseconds();
    double deltaTime = (curTime - prevTime) * 0.001;
    
    if (rotateModel) {
        rotation += 60.0f * deltaTime;
        while (rotation > 360.0f) {
            rotation -= 360.0f;
        }
    }
    
    prevTime = curTime;
    glutTimerFunc(UPDATE_TIME, updateTimer, windowId);
    glutPostRedisplay();
}

void reshape(int width, int height) {
    ImGui::GetIO().DisplaySize.x = width;
    ImGui::GetIO().DisplaySize.y = height;
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, width, height);
    activeReshape(width, height);
    glMatrixMode(GL_MODELVIEW);
}

void keyDown(unsigned char key, int x, int y) {
}

void keyUp(unsigned char key, int x, int y) {
    ImGuiIO& io = ImGui::GetIO();
    //unsigned char keys[] = { key, '\0'};
    //io.AddInputCharactersUTF8((const char*)keys);
    io.AddInputCharacter(key);
    io.MousePos = ImVec2((float)x, (float)y);
}

void mouseDrag(int x, int y) {
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
}

void mouseMove(int x, int y) {
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
}

void mouse(int button, int state, int x, int y) {
    ImGuiIO& io = ImGui::GetIO();
    
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            io.MouseDown[0] = true;
        } else /* GLUT_UP */ {
            io.MouseDown[0] = false;
        }
    } else if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            io.MouseDown[1] = true;
        } else /* GLUT_UP */ {
            io.MouseDown[1] = false;
        }
    }
}

void reshapeOrtho(int w, int h) {
    glOrtho(0.0f, 1.0f, 1.0f, 0.0f, -100.0f, 100.0f);
}

void reshapePerspective(int w, int h) {
    glPerspective(60.0f, (float)w / (float)h, 0.01f, 1000.0f);
}

void glPerspective(float fov, float aspectRatio, float znear, float zfar) {
    float ymax = znear * tanf(fov * M_PI / 360.0f);
    float xmax = ymax * aspectRatio;
    
    glFrustum(-xmax, xmax, -ymax, ymax, znear, zfar);
}

double GetMilliseconds() {
#if !WIN32
    static timeval s_tTimeVal;
    gettimeofday(&s_tTimeVal, NULL);
    double time = s_tTimeVal.tv_sec * 1000.0; // sec to ms
    time += s_tTimeVal.tv_usec / 1000.0; // us to ms
#else
    double time = (double)GetTickCount() * 0.001;
#endif
    return time;
}

void Normalize(GLfloat *a) {
    GLfloat d=sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
    a[0]/=d; a[1]/=d; a[2]/=d;
}

void DrawPlane(int subdiv) {
    glBegin(GL_TRIANGLES);
    for (int x = 0; x < subdiv; ++x) {
        for (int z = 0; z < subdiv; ++z) {
            float divisionStep = 2.0f / ((float)subdiv);
            
            float xMin = -1.0f + ((float)x) * divisionStep;
            float xMax = xMin + divisionStep;
            
            float zMin = -1.0f + ((float)z) * divisionStep;
            float zMax = zMin + divisionStep;
            
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(xMax, 0.0f, zMin);
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(xMin, 0.0f, zMin);
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(xMin, 0.0f, zMax);
            
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(xMax, 0.0f, zMin);
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(xMin, 0.0f, zMax);
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(xMax, 0.0f, zMax);
        }
    }
    glEnd();
}

void DrawTriangle(GLfloat *a, GLfloat *b, GLfloat *c, int div, float r) {
    if (div<=0) {
        glNormal3fv(b); glVertex3f(b[0]*r, b[1]*r, b[2]*r);
        glNormal3fv(a); glVertex3f(a[0]*r, a[1]*r, a[2]*r);
        glNormal3fv(c); glVertex3f(c[0]*r, c[1]*r, c[2]*r);
    } else {
        GLfloat ab[3], ac[3], bc[3];
        for (int i=0;i<3;i++) {
            ab[i]=(a[i]+b[i])/2;
            ac[i]=(a[i]+c[i])/2;
            bc[i]=(b[i]+c[i])/2;
        }
        Normalize(ab); Normalize(ac); Normalize(bc);
        DrawTriangle(a, ab, ac, div-1, r);
        DrawTriangle(b, bc, ab, div-1, r);
        DrawTriangle(c, ac, bc, div-1, r);
        DrawTriangle(ab, bc, ac, div-1, r);  //<--Comment this line and sphere looks really cool!
    }
}

void DrawSphere(int ndiv, float radius) {
    glBegin(GL_TRIANGLES);
    for (int i=0;i<20;i++) {
        DrawTriangle(vdata[tindices[i][0]], vdata[tindices[i][1]], vdata[tindices[i][2]], ndiv, radius);
    }
    glEnd();
}