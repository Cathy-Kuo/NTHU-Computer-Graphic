#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
int WINDOW_WIDTH = 1200;
int WINDOW_HEIGHT = 600;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;
double press_x = 0;
double press_y = 0;
int light_idx = 0;
//hw2
struct Light
{
    Vector3 position;
    Vector3 spotDirection;
    Vector3 ambient;
    Vector3 diffuse;
    Vector3 specular;
    float Shininess;
    float spotExponent;
    float spotCutoff;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
}light[3];

struct iLocLight
{
    GLuint position;
    GLuint ambient;
    GLuint diffuse;
    GLuint specular;
    GLuint Shininess;
    GLuint spotDirection;
    GLuint spotCutoff;
    GLuint spotExponent;
    GLuint constantAttenuation;
    GLuint linearAttenuation;
    GLuint quadraticAttenuation;
}iLocLight[3];

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
    LightEdit = 6,
    ShineEdit = 7,
};
//hw2
GLint iLocMVP;
GLint iLocV;
GLint iLocN;
GLint iLocLightIdx;
GLint iLocKa;
GLint iLocKd;
GLint iLocKs;
GLint iLocVerpixel;

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;
    
};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

Shape quad;
Shape m_shpae;
int cur_idx = 0; // represent which model should be rendered now

static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
    Matrix4 mat;
    
    
    mat = Matrix4(
                  1, 0, 0, vec.x,
                  0, 1, 0, vec.y,
                  0, 0, 1, vec.z,
                  0, 0, 0, 1
                  );
    
    return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
    Matrix4 mat;
    
    
    mat = Matrix4(
                  vec.x, 0, 0, 0,
                  0, vec.y, 0, 0,
                  0, 0, vec.z, 0,
                  0, 0, 0, 1
                  );
    
    return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
    Matrix4 mat;
    
    
    mat = Matrix4(
                  1, 0, 0, 0,
                  0, cos(val), -sin(val), 0,
                  0, sin(val), cos(val), 0,
                  0, 0, 0, 1
                  );
    
    return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
    Matrix4 mat;
    
    
    mat = Matrix4(
                  cos(val), 0, sin(val), 0,
                  0, 1, 0, 0,
                  -sin(val), 0, cos(val), 0,
                  0, 0, 0, 1
                  );
    
    return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
    Matrix4 mat;
    
    
    mat = Matrix4(
                  cos(val), -sin(val), 0, 0,
                  sin(val), cos(val), 0, 0,
                  0, 0, 1, 0,
                  0, 0, 0, 1
                  );
    
    return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

Vector3 Normalize_V(Vector3 v)
{
    GLfloat l;
    
    l = (GLfloat)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return Vector3(v.x / l, v.y / l, v.z / l);
}

Vector3 Cross_V(Vector3 u, Vector3 v)
{
    Vector3 n;
    n.x = u.y * v.z - u.z * v.y;
    n.y = u.z * v.x - u.x * v.z;
    n.z = u.x * v.y - u.y * v.x;
    return n;
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
    Vector3 f = main_camera.center - main_camera.position;
    Vector3 eye = main_camera.position;
    Vector3 f_nor = Normalize_V(f);
    Vector3 up = main_camera.up_vector;
    Vector3 s, uu, s_nor;
    s = Cross_V(f_nor, up);
    s_nor = Normalize_V(s);
    uu = Cross_V(s_nor, f_nor);
    view_matrix = Matrix4(
                          s_nor.x, s_nor.y, s_nor.z, -(s_nor.x * eye.x) - (s_nor.y * eye.y) - (s_nor.z * eye.z),
                          uu.x, uu.y, uu.z, -(uu.x * eye.x) - (uu.y * eye.y) - (uu.z * eye.z),
                          -f_nor.x, -f_nor.y, -f_nor.z, (f_nor.x * eye.x) + (f_nor.y * eye.y) + (f_nor.z * eye.z),
                          0, 0, 0, 1
                          );
}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
    cur_proj_mode = Orthogonal;
    project_matrix = Matrix4(
                             2 / (proj.right - proj.left), 0, 0, -((proj.right + proj.left) / (proj.right - proj.left)),
                             0, 2 / (proj.top - proj.bottom), 0, -((proj.top + proj.bottom) / (proj.top - proj.bottom)),
                             0, 0, -2 / (proj.farClip - proj.nearClip), -((proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip)),
                             0, 0, 0, 1
                             );
}


// [TODO] compute persepective projection matrix
void setPerspective()
{
    cur_proj_mode = Perspective;
    project_matrix = Matrix4(
                             2 * proj.nearClip / (proj.right - proj.left),0, ((proj.right + proj.left) / (proj.right - proj.left)), 0,
                             0, 2 * proj.nearClip / (proj.top - proj.bottom), ((proj.top + proj.bottom) / (proj.top - proj.bottom)), 0,
                             0, 0, -(proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip), -2 * proj.farClip * proj.nearClip / (proj.farClip - proj.nearClip),
                             0, 0, -1, 0
                             );
}


// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
//        glViewport(0, 0, width, height);
    // [TODO] change your aspect ratio???
    proj.aspect = (GLfloat)width / (GLfloat)height;
    WINDOW_HEIGHT = height;
    WINDOW_WIDTH = width;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-100.0, 100.0, -100.0, 100.0, 1, -1);
}

void drawPlane()
{
	GLfloat vertices[18]{ 1.0, -0.9, -1.0,
		1.0, -0.9,  1.0,
		-1.0, -0.9, -1.0,
		1.0, -0.9,  1.0,
		-1.0, -0.9,  1.0,
		-1.0, -0.9, -1.0 };

	GLfloat colors[18]{ 0.0,1.0,0.0,
		0.0,0.5,0.8,
		0.0,1.0,0.0,
		0.0,0.5,0.8,
		0.0,0.5,0.8,
		0.0,1.0,0.0 };

	// [TODO] draw the plane with above vertices and color
}

// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
    T = translate(models[cur_idx].position);
    R = rotate(models[cur_idx].rotation);
    S = scaling(models[cur_idx].scale);
    
	Matrix4 MVP;
	GLfloat mvp[16];

	// [TODO] multiply all the matrix
	// [TODO] row-major ---> column-major
    
    MVP = project_matrix * view_matrix * T * R * S;
    mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
    mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
    mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
    mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];

	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
    
    glUniform3f(iLocLight[0].ambient, light[0].ambient.x, light[0].ambient.y, light[0].ambient.z);
    glUniform3f(iLocLight[1].ambient, light[1].ambient.x, light[1].ambient.y, light[1].ambient.z);
    glUniform3f(iLocLight[2].ambient, light[2].ambient.x, light[2].ambient.y, light[2].ambient.z);
    
    glUniform3f(iLocLight[0].diffuse, light[0].diffuse.x, light[0].diffuse.y, light[0].diffuse.z);
    glUniform3f(iLocLight[1].diffuse, light[1].diffuse.x, light[1].diffuse.y, light[1].diffuse.z);
    glUniform3f(iLocLight[2].diffuse, light[2].diffuse.x, light[2].diffuse.y, light[2].diffuse.z);
    
    glUniform3f(iLocLight[0].specular, light[0].specular.x, light[0].specular.y, light[0].specular.z);
    glUniform3f(iLocLight[1].specular, light[1].specular.x, light[1].specular.y, light[1].specular.z);
    glUniform3f(iLocLight[2].specular, light[2].specular.x, light[2].specular.y, light[2].specular.z);
    
    glUniform1f(iLocLight[0].Shininess, light[0].Shininess);
    glUniform1f(iLocLight[1].Shininess, light[1].Shininess);
    glUniform1f(iLocLight[2].Shininess, light[2].Shininess);
    
    glUniform3f(iLocLight[0].position, light[0].position.x, light[0].position.y, light[0].position.z);
    glUniform3f(iLocLight[0].spotDirection, light[0].spotDirection.x, light[0].spotDirection.y, light[0].spotDirection.z);
    
    glUniform3f(iLocLight[1].position, light[1].position.x, light[1].position.y, light[1].position.z);
    glUniform1f(iLocLight[1].constantAttenuation, light[1].constantAttenuation);
    glUniform1f(iLocLight[1].linearAttenuation, light[1].linearAttenuation);
    glUniform1f(iLocLight[1].quadraticAttenuation, light[1].quadraticAttenuation);
    
    glUniform3f(iLocLight[2].position, light[2].position.x, light[2].position.y, light[2].position.z);
    glUniform3f(iLocLight[2].spotDirection, light[2].spotDirection.x, light[2].spotDirection.y, light[2].spotDirection.z);
    glUniform1f(iLocLight[2].spotExponent, light[2].spotExponent);
    glUniform1f(iLocLight[2].spotCutoff, light[2].spotCutoff);
    glUniform1f(iLocLight[2].constantAttenuation, light[2].constantAttenuation);
    glUniform1f(iLocLight[2].linearAttenuation, light[2].linearAttenuation);
    glUniform1f(iLocLight[2].quadraticAttenuation, light[2].quadraticAttenuation);
    
    if(WINDOW_WIDTH/2>WINDOW_HEIGHT)
    {
        glViewport((WINDOW_WIDTH/2-WINDOW_HEIGHT)/2, 0, min(WINDOW_WIDTH/2,WINDOW_HEIGHT), min(WINDOW_WIDTH/2,WINDOW_HEIGHT));
    }
    else{
        glViewport(0, (WINDOW_HEIGHT-WINDOW_WIDTH/2)/2, min(WINDOW_WIDTH/2,WINDOW_HEIGHT), min(WINDOW_WIDTH/2,WINDOW_HEIGHT));
    }
    glUniform1i(iLocLightIdx, light_idx);
    glUniform1i(iLocVerpixel, 0);
    for (int i = 0; i < models[cur_idx].shapes.size(); i++)
    {
        //hw2
        GLfloat ambient[3], diffuse[3], specular[3];
        for (int j=0; j<3; j++){
            ambient[j] = models[cur_idx].shapes[i].material.Ka[j];
            diffuse[j] = models[cur_idx].shapes[i].material.Kd[j];
            specular[j] = models[cur_idx].shapes[i].material.Ks[j];
        }
        
        glUniform3fv(iLocKa, 1, ambient);
        glUniform3fv(iLocKd, 1, diffuse);
        glUniform3fv(iLocKs, 1, specular);
        
        Matrix4 N = view_matrix * T * R * S;
        glUniformMatrix4fv(iLocV, 1, GL_FALSE, view_matrix.getTranspose());
        glUniformMatrix4fv(iLocN, 1, GL_FALSE, N.getTranspose());
        
        glBindVertexArray(models[cur_idx].shapes[i].vao);
        glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
    }
    
    if(WINDOW_WIDTH/2>WINDOW_HEIGHT)
    {
        glViewport(WINDOW_WIDTH/2+(WINDOW_WIDTH/2-WINDOW_HEIGHT)/2, 0, min(WINDOW_WIDTH/2,WINDOW_HEIGHT), min(WINDOW_WIDTH/2,WINDOW_HEIGHT));
    }
    else{
        glViewport(WINDOW_WIDTH/2, (WINDOW_HEIGHT-WINDOW_WIDTH/2)/2, min(WINDOW_WIDTH/2,WINDOW_HEIGHT), min(WINDOW_WIDTH/2,WINDOW_HEIGHT));
    }
    glUniform1i(iLocLightIdx, light_idx);
    glUniform1i(iLocVerpixel, 1);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++) 
	{
        GLfloat ambient[3], diffuse[3], specular[3];
        for (int j=0; j<3; j++){
            ambient[j] = models[cur_idx].shapes[i].material.Ka[j];
            diffuse[j] = models[cur_idx].shapes[i].material.Kd[j];
            specular[j] = models[cur_idx].shapes[i].material.Ks[j];
        }
        glUniform3fv(iLocKa, 1, ambient);
        glUniform3fv(iLocKd, 1, diffuse);
        glUniform3fv(iLocKs, 1, specular);
        
        Matrix4 N = view_matrix * T * R * S;
        glUniformMatrix4fv(iLocV, 1, GL_FALSE, view_matrix.getTranspose());
        glUniformMatrix4fv(iLocN, 1, GL_FALSE, N.getTranspose());
        
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}
	drawPlane();
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // [TODO] Call back function for keyboard
    if (key == GLFW_KEY_Z && action == GLFW_PRESS){
        cur_idx--;
        if (cur_idx<0) cur_idx+=5;
    }
    else if (key == GLFW_KEY_X && action == GLFW_PRESS){
        cur_idx++;
        if (cur_idx>4) cur_idx-=5;
    }
    else if (key == GLFW_KEY_O && action == GLFW_PRESS){
        setOrthogonal();
    }
    else if (key == GLFW_KEY_P && action == GLFW_PRESS){
        setPerspective();
    }
    else if (key == GLFW_KEY_T && action == GLFW_PRESS){
        cur_trans_mode = GeoTranslation;
    }
    else if (key == GLFW_KEY_S && action == GLFW_PRESS){
        cur_trans_mode = GeoScaling;
    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS){
        cur_trans_mode = GeoRotation;
    }
    else if (key == GLFW_KEY_L && action == GLFW_PRESS){
        light_idx = (light_idx+1)%3;
        if (light_idx == 0) cout << "Light source: Directional Light" << endl;
        else if (light_idx == 1) cout << "Light source: Positional Light" << endl;
        else cout << "Light source: Spot Light" << endl;
    }
    else if (key == GLFW_KEY_K && action == GLFW_PRESS){
        cur_trans_mode = LightEdit;
    }
    else if (key == GLFW_KEY_J && action == GLFW_PRESS){
        cur_trans_mode = ShineEdit;
    }
    else if (key == GLFW_KEY_E && action == GLFW_PRESS){
        cur_trans_mode = ViewEye;
    }
    else if (key == GLFW_KEY_C && action == GLFW_PRESS){
        cur_trans_mode = ViewCenter;
    }
    else if (key == GLFW_KEY_U && action == GLFW_PRESS){
        cur_trans_mode = ViewUp;
    }
    else if (key == GLFW_KEY_I && action == GLFW_PRESS){
        Matrix4 T, R, S;
        T = translate(models[cur_idx].position);
        R = rotate(models[cur_idx].rotation);
        S = scaling(models[cur_idx].scale);
        cout << "Translation Matrix: " << endl << T << endl;
        cout << "Rotation Matrix: " << endl << R << endl;
        cout << "Scaling Matrix: " << endl << S << endl;
        cout << "Viewing Matrix: " << endl << view_matrix << endl;
        cout << "Projection Matrix: " << endl << project_matrix << endl;
    }
    
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // [TODO] scroll up positive, otherwise it would be negtive
    if (yoffset>0){
        if (cur_trans_mode == ViewEye){
            main_camera.position.z += 0.05;
            setViewingMatrix();
        }
        else if (cur_trans_mode == ViewCenter){
            main_camera.center.z += 0.05;
            setViewingMatrix();
        }
        else if (cur_trans_mode == ViewUp){
            main_camera.up_vector.z += 0.1;
            setViewingMatrix();
        }
        else if (cur_trans_mode == GeoTranslation){
            models[cur_idx].position.z += 0.05;
        }
        else if (cur_trans_mode == GeoScaling){
            models[cur_idx].scale.z += 0.05;
        }
        else if (cur_trans_mode == GeoRotation){
            models[cur_idx].rotation.z += 0.05;
        }
        else if (cur_trans_mode == LightEdit){
            if (light_idx == 0 || light_idx == 1){
                light[light_idx].diffuse.x -= 0.01;
                light[light_idx].diffuse.y -= 0.01;
                light[light_idx].diffuse.z -= 0.01;
            }
            else{
                light[light_idx].spotCutoff -= 0.1;
            }
        }
        else if (cur_trans_mode == ShineEdit){
            light[light_idx].Shininess += 1;
        }
    }
    else{
        if (cur_trans_mode == ViewEye){
            main_camera.position.z -= 0.05;
            setViewingMatrix();
        }
        else if (cur_trans_mode == ViewCenter){
            main_camera.center.z -= 0.05;
            setViewingMatrix();
        }
        else if (cur_trans_mode == ViewUp){
            main_camera.up_vector.z -= 0.1;
            setViewingMatrix();
        }
        else if (cur_trans_mode == GeoTranslation){
            models[cur_idx].position.z -= 0.05;
        }
        else if (cur_trans_mode == GeoScaling){
            models[cur_idx].scale.z -= 0.05;
        }
        else if (cur_trans_mode == GeoRotation){
            models[cur_idx].rotation.z -= 0.05;
        }
        else if (cur_trans_mode == LightEdit){
            if (light_idx == 0 || light_idx == 1){
                light[light_idx].diffuse.x += 0.01;
                light[light_idx].diffuse.y += 0.01;
                light[light_idx].diffuse.z += 0.01;
            }
            else{
                light[light_idx].spotCutoff += 0.1;
            }
        }
        else if (cur_trans_mode == ShineEdit){
            light[light_idx].Shininess -= 1;
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // [TODO] mouse press callback function
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        mouse_pressed = true;
        glfwGetCursorPos(window, &press_x, &press_y);
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        mouse_pressed = false;
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    // [TODO] cursor position callback function
    if (mouse_pressed == true){
        double x = xpos - press_x;//+
        double y = ypos - press_y;//-
        press_x = xpos;
        press_y = ypos;
        if (cur_trans_mode == ViewEye){
            main_camera.position.x -= x*0.01;
            main_camera.position.y += y*0.01;
            setViewingMatrix();
        }
        else if (cur_trans_mode == ViewCenter){
            main_camera.center.x -= x*0.01;
            main_camera.center.y += y*0.01;
            setViewingMatrix();
        }
        else if (cur_trans_mode == ViewUp){
            main_camera.up_vector.x += x*0.008;
            main_camera.up_vector.y += y*0.008;
            setViewingMatrix();
        }
        else if (cur_trans_mode == GeoTranslation){
            models[cur_idx].position.x += x*0.008;
            models[cur_idx].position.y -= y*0.008;
        }
        else if (cur_trans_mode == GeoScaling){
            models[cur_idx].scale.x += x*0.005;
            models[cur_idx].scale.y -= y*0.005;
        }
        else if (cur_trans_mode == GeoRotation){
            models[cur_idx].rotation.x += y*0.01;
            models[cur_idx].rotation.y += x*0.01;
        }
        else if (cur_trans_mode == LightEdit){
            if (light_idx == 2){
                light[light_idx].position.x += x * 0.005;
                light[light_idx].position.y -= y * 0.005;
            }
            else {
                light[light_idx].position.x += x * 0.05;
                light[light_idx].position.y -= y * 0.05;
            }
        }
    }
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();
    
	// attach shaders to program objectans
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	iLocMVP = glGetUniformLocation(p, "mvp");
    //hw2
    iLocVerpixel = glGetUniformLocation(p, "Verpixel");
    iLocV = glGetUniformLocation(p, "um4v");
    iLocN = glGetUniformLocation(p, "um4n");
    iLocLightIdx = glGetUniformLocation(p, "lightIdx");
    iLocKa = glGetUniformLocation(p, "material.Ka");
    iLocKd = glGetUniformLocation(p, "material.Kd");
    iLocKs = glGetUniformLocation(p, "material.Ks");
    
    iLocLight[0].position = glGetUniformLocation(p, "light[0].position");
    iLocLight[0].ambient = glGetUniformLocation(p, "light[0].Ia");
    iLocLight[0].diffuse = glGetUniformLocation(p, "light[0].Id");
    iLocLight[0].specular = glGetUniformLocation(p, "light[0].Is");
    iLocLight[0].Shininess = glGetUniformLocation(p, "light[0].Shininess");
    iLocLight[0].spotDirection = glGetUniformLocation(p, "light[0].spotDirection");
    iLocLight[0].spotCutoff = glGetUniformLocation(p, "light[0].spotCutoff");
    iLocLight[0].spotExponent = glGetUniformLocation(p, "light[0].spotExponent");
    iLocLight[0].constantAttenuation = glGetUniformLocation(p, "light[0].constantAttenuation");
    iLocLight[0].linearAttenuation = glGetUniformLocation(p, "light[0].linearAttenuation");
    iLocLight[0].quadraticAttenuation = glGetUniformLocation(p, "light[0].quadraticAttenuation");
    
    iLocLight[1].position = glGetUniformLocation(p, "light[1].position");
    iLocLight[1].ambient = glGetUniformLocation(p, "light[1].Ia");
    iLocLight[1].diffuse = glGetUniformLocation(p, "light[1].Id");
    iLocLight[1].specular = glGetUniformLocation(p, "light[1].Is");
    iLocLight[1].Shininess = glGetUniformLocation(p, "light[1].Shininess");
    iLocLight[1].spotDirection = glGetUniformLocation(p, "light[1].spotDirection");
    iLocLight[1].spotCutoff = glGetUniformLocation(p, "light[1].spotCutoff");
    iLocLight[1].spotExponent = glGetUniformLocation(p, "light[1].spotExponent");
    iLocLight[1].constantAttenuation = glGetUniformLocation(p, "light[1].constantAttenuation");
    iLocLight[1].linearAttenuation = glGetUniformLocation(p, "light[1].linearAttenuation");
    iLocLight[1].quadraticAttenuation = glGetUniformLocation(p, "light[1].quadraticAttenuation");
    
    iLocLight[2].position = glGetUniformLocation(p, "light[2].position");
    iLocLight[2].ambient = glGetUniformLocation(p, "light[2].Ia");
    iLocLight[2].diffuse = glGetUniformLocation(p, "light[2].Id");
    iLocLight[2].specular = glGetUniformLocation(p, "light[2].Is");
    iLocLight[2].Shininess = glGetUniformLocation(p, "light[2].Shininess");
    iLocLight[2].spotDirection = glGetUniformLocation(p, "light[2].spotDirection");
    iLocLight[2].spotCutoff = glGetUniformLocation(p, "light[2].spotCutoff");
    iLocLight[2].spotExponent = glGetUniformLocation(p, "light[2].spotExponent");
    iLocLight[2].constantAttenuation = glGetUniformLocation(p, "light[2].constantAttenuation");
    iLocLight[2].linearAttenuation = glGetUniformLocation(p, "light[2].linearAttenuation");
    iLocLight[2].quadraticAttenuation = glGetUniformLocation(p, "light[2].quadraticAttenuation");

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);
		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		
			glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 1;
	proj.farClip = 10.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);
    
    light[0].position = Vector3(1.0f, 1.0f, 1.0f);
    light[0].spotDirection = Vector3(0.0f, 0.0f, 0.0f);
    light[0].ambient = Vector3(0.15f, 0.15f, 0.15f);
    light[0].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    light[0].specular = Vector3(1.0f, 1.0f, 1.0f);
    light[0].Shininess = 64;
    
    light[1].position = Vector3(0.0f, 2.0f, 1.0f);
    light[1].ambient = Vector3(0.15f, 0.15f, 0.15f);
    light[1].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    light[1].specular = Vector3(1.0f, 1.0f, 1.0f);
    light[1].Shininess = 64;
    light[1].constantAttenuation = 0.01f;
    light[1].linearAttenuation = 0.8f;
    light[1].quadraticAttenuation = 0.1f;
    
    light[2].position = Vector3(0.0f, 0.0f, 2.0f);
    light[2].ambient = Vector3(0.15f, 0.15f, 0.15f);
    light[2].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    light[2].specular = Vector3(1.0f, 1.0f, 1.0f);
    light[2].Shininess = 64;
    light[2].spotDirection = Vector3(0.0f, 0.0f, -1.0f);
    light[2].spotExponent = 50;
    light[2].spotCutoff = 30;
    light[2].constantAttenuation = 0.05f;
    light[2].linearAttenuation = 0.3f;
    light[2].quadraticAttenuation = 0.6f;

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();
    
	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
    
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here
    int idx = 0;
    for (int i = 0; i<5; i++){
        LoadModels(model_list[idx]);
        idx++;
        if (idx==4) cur_idx = 0;
    }
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "106062212 HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();
    cout << "Light source: Directional Light" << endl;
	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
