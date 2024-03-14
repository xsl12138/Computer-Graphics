#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vmath.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include "shader.h"
#include "camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

//窗口大小参数
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
float aspact = (float)4.0 / (float)3.0;

//旋转参数
static GLfloat xRot = 20.0f;
static GLfloat yRot = 20.0f;

//句柄参数
GLuint vertex_array_object; // == VAO句柄
GLuint vertex_buffer_object; // == VBO句柄
GLuint element_buffer_object;//==EBO句柄
GLuint texture_buffer_object1[4]; // 纹理对象句柄1
GLuint texture_buffer_object2[4]; // 纹理对象句柄2
//int shader_program;//着色器程序句柄

//指定摄像机
Camera camera(glm::vec3(0.0f, 0.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 指定时间参数
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float a = 6.0f, b = 3.0f;
float r = 1.0f;

//球的数据参数
std::vector<float> sphereVertices;
std::vector<int> sphereIndices;
const int Y_SEGMENTS = 20;
const int X_SEGMENTS = 20;
const float Radio = 2.0;	//调整此参数将影响贴图质量，经测试取值2最佳
const GLfloat  PI = 3.14159265358979323846f;

std::map<int, std::string> mapp;
//指定光照
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

void initial(void)
{
	mapp[0] = "image/earth.jpg";
	mapp[1] = "image/sun.jpg";
	mapp[2] = "image/moon.jpg";
	//进行球体顶点和三角面片的计算
	// 生成球的顶点
	for (int y = 0; y <= Y_SEGMENTS; y++)
	{
		for (int x = 0; x <= X_SEGMENTS; x++)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * Radio * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * Radio * PI) * std::sin(ySegment * PI);
			glm::vec3 normal = glm::normalize(glm::vec3(xPos, yPos, zPos));
			//位置
			sphereVertices.push_back(xPos);
			sphereVertices.push_back(yPos);
			sphereVertices.push_back(zPos);
			//法向量
			sphereVertices.push_back(normal.x);
			sphereVertices.push_back(normal.y);
			sphereVertices.push_back(normal.z);
			//添加纹理坐标
			sphereVertices.push_back(xSegment);
			sphereVertices.push_back(ySegment);
		}
	}

	// 生成球的顶点
	for (int i = 0; i < Y_SEGMENTS; i++)
	{
		for (int j = 0; j < X_SEGMENTS; j++)
		{

			sphereIndices.push_back(i * (X_SEGMENTS + 1) + j);
			sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j);
			sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);

			sphereIndices.push_back(i * (X_SEGMENTS + 1) + j);
			sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
			sphereIndices.push_back(i * (X_SEGMENTS + 1) + j + 1);
		}
	}

	// 球
	glGenVertexArrays(1, &vertex_array_object);
	glGenBuffers(1, &vertex_buffer_object);
	//生成并绑定球体的VAO和VBO
	glBindVertexArray(vertex_array_object);

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
	// 将顶点数据绑定至当前默认的缓冲中
	glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), &sphereVertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &element_buffer_object);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(int), &sphereIndices[0], GL_STATIC_DRAW);

	// 设置顶点属性指针
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// 纹理坐标属性
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));//最后一个参数：从sphereVertices的第七项（序号6）开始为纹理属性
	glEnableVertexAttribArray(1);
	// 法线属性
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));//最后一个参数：从sphereVertices的第四项（序号3）开始为法线属性
	glEnableVertexAttribArray(2);

	for (int i = 0; i < 3; i++) {
		//加载纹理数据
		glGenTextures(1, &texture_buffer_object1[i]);
		glBindTexture(GL_TEXTURE_2D, texture_buffer_object1[i]);
		//指定纹理的参数
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//加载纹理
		int width, height, nrchannels;//纹理长宽，通道数
		stbi_set_flip_vertically_on_load(true);
		//加载纹理图片
		unsigned char* data = stbi_load(mapp[i].c_str(), &width, &height, &nrchannels, 0);
		//std::cout << mapp[i].c_str() << ' ' << width  << ' '<< height << ' ' << nrchannels << std::endl;
		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);	//必须用GL_RGB（加载并生成具有3个颜色 channel 的纹理）而不能是GL_RGBA（加载并生成具有4个颜色 channel 的纹理）
			//生成Mipmap纹理
			glGenerateMipmap(GL_TEXTURE_2D);

			glGenTextures(1, &texture_buffer_object2[i]);
			glBindTexture(GL_TEXTURE_2D, texture_buffer_object2[i]);
			//指定纹理的参数
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			//生成Mipmap纹理
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);//释放资源
	}

	// 解绑VAO和VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//设定点线面的属性
	glPointSize(15);//设置点的大小
	glLineWidth(5);//设置线宽

	//启动剔除操作
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	//开启深度测试
	glEnable(GL_DEPTH_TEST);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (key)
	{
	case GLFW_KEY_3:
		glEnable(GL_CULL_FACE);    //打开背面剔除
		glCullFace(GL_BACK);          //剔除多边形的背面
		break;
	case GLFW_KEY_4:
		glDisable(GL_CULL_FACE);     //关闭背面剔除
		break;
	default:
		break;
	}
}

//鼠标滚轮回调函数
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	//对摄像机进行操作
	camera.ProcessMouseScroll((float)yoffset);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}

	float xoffset = (float)xpos - lastX;
	float yoffset = lastY - (float)ypos; // y坐标系进行反转

	lastX = (float)xpos;
	lastY = (float)ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void reshaper(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	if (height == 0)
	{
		aspact = (float)width;
	}
	else
	{
		aspact = (float)width / (float)height;
	}

}

void Draw(Shader& lightingShader)
{
	//处理时间
	float currentFrame = (float)glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// 清空颜色缓冲和深度缓冲区
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// be sure to activate shader when setting uniforms/drawing objects
	// 激活光照
	lightingShader.use();
	lightingShader.setVec3("light.position", glm::vec3(0.0f, 0.0f, 0.0f));
	lightingShader.setVec3("light.direction", camera.Front);
	lightingShader.setVec3("viewPos", camera.Position);
	lightingShader.setVec3("lightPos", lightPos);

	//光源属性
	/*
	一个光源对它的ambient、diffuse和specular光照分量有着不同的强度。
	环境光照通常被设置为一个比较低的强度，因为我们不希望环境光颜色太过主导。
	光源的漫反射分量通常被设置为我们希望光所具有的那个颜色，通常是一个比较明亮的白色。
	镜面光分量通常会保持为vec3(1.0)，以最大强度发光。
	*/
	lightingShader.setVec3("light.ambient", 1.2f, 1.2f, 1.2f);//环境光强度向量（三维为RGB强度，下同）
	lightingShader.setVec3("light.diffuse", 1.0f, 1.0f, 1.0f);	//漫反射强度向量
	lightingShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);	//高光（镜面光）强度向量
	lightingShader.setFloat("light.constant", 1.0f);
	lightingShader.setFloat("light.linear", 0.045f);	
	lightingShader.setFloat("light.quadratic", 0.0075f);

	//反光度
	lightingShader.setFloat("shininess", 1.25f);

	//glm::perspective函数：创建一个透视投影矩阵，用于将3D空间中的坐标投影到2D平面上，参数一：视野角度（弧度制）；参数二：投影平面的宽高比；参数三四：投影平面到视点的最近和最远距离
	glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = camera.GetViewMatrix();
	//地球
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_buffer_object1[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_buffer_object2[0]);

	lightingShader.setMat4("projection", projection);	//定义投影变换矩阵
	lightingShader.setMat4("view", view);	//定义视图变换矩阵
	float currentTime = (float)glfwGetTime();

	glBindVertexArray(vertex_array_object);
	//glm::rotate函数：用于将物体进行旋转（不是指让物体动态旋转，而是将物体旋转到指定角度获取视图）
	//glm::translate函数：将指定矩阵位移至对应位置
	glm::mat4 scale_earth = glm::scale(glm::mat4(1.0f), glm::vec3(0.6, 0.6, 0.6));
	glm::mat4 rot_tilt_earth = glm::rotate(glm::mat4(1.0f), glm::radians(135.0f), glm::vec3(0.0f, 0.0f, 1.0f));	//倾斜角度
	glm::mat4 trans_revolution_earth = glm::translate(glm::mat4(1.0f), glm::vec3(-a * std::cos(PI / 3 * currentTime), 0.0f, b * std::sin(PI / 3 * currentTime)));	//公转
	glm::mat4 rot_self_earth = glm::rotate(glm::mat4(1.0f), -10 * currentTime * glm::radians(45.0f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));	//自转
	glm::mat4 model_earth = trans_revolution_earth * rot_self_earth * rot_tilt_earth * scale_earth;

	lightingShader.setMat4("model", model_earth);		//定义模型变换矩阵
	glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

	//太阳
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_buffer_object1[1]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_buffer_object2[1]);

	lightingShader.setMat4("projection", projection);
	lightingShader.setMat4("view", view);
	glm::mat4 scale_sun = glm::scale(glm::mat4(1.0f), glm::vec3(2.4, 2.4, 2.4));
	glm::mat4 rot_self_sun = glm::rotate(glm::mat4(1.0f), currentTime * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 model_sun = rot_self_sun * scale_sun;

	lightingShader.setMat4("model", model_sun);
	glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

	//月亮
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_buffer_object1[2]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_buffer_object2[2]);

	lightingShader.setMat4("projection", projection);
	lightingShader.setMat4("view", view);

	glBindVertexArray(vertex_array_object);
	glm::mat4 scale_moon = glm::scale(glm::mat4(1.0f), glm::vec3(0.2, 0.2, 0.2));
	glm::mat4 trans_revolution_moon = glm::translate(glm::mat4(1.0f), glm::vec3(r * std::cos(PI * 7 * currentTime), 0.0f, r * std::sin(PI * 7 * currentTime)));
	glm::mat4 rot_tilt_moon = glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 rot_self_moon = glm::rotate(glm::mat4(1.0f), -365 * currentTime * glm::radians(45.0f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
	glm::mat4 model_moon = trans_revolution_earth * rot_tilt_moon * trans_revolution_moon * rot_self_moon * scale_moon;

	lightingShader.setMat4("model", model_moon);
	glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);                                                      // 解除绑定

}

int main()
{
	glfwInit(); // 初始化GLFW

	// OpenGL版本为3.3，主次版本号均设为3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

	// 使用核心模式(无需向后兼容性)
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// 创建窗口(宽、高、窗口名称)
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sphere", NULL, NULL);

	if (window == NULL)
	{
		std::cout << "Failed to Create OpenGL Context" << std::endl;
		glfwTerminate();
		return -1;
	}

	// 将窗口的上下文设置为当前线程的主上下文
	glfwMakeContextCurrent(window);

	// 初始化GLAD，加载OpenGL函数指针地址的函数
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	printf("按3开启背面剔除，按4关闭背面剔除（默认为关闭）\n");
	printf("背面剔除状态下，太阳会更亮\n");
	printf("代码379至382行可以启用摄像机\n\n");
	initial();//初始化
	glDisable(GL_CULL_FACE);     //关闭背面剔除

	//窗口大小改变时调用reshaper函数
	glfwSetFramebufferSizeCallback(window, reshaper);

	//窗口中有键盘操作时调用key_callback函数
	glfwSetKeyCallback(window, key_callback);

	////鼠标回调函数
	//glfwSetCursorPosCallback(window, mouse_callback);
	////当鼠标滚轮滚动时调用函数scroll_callback
	//glfwSetScrollCallback(window, scroll_callback);

	Shader lightingShader("shader/colors.vs", "shader/colors.fs");	//指定顶点着色器
	lightingShader.use();
	//lightingShader.setInt("texture1", 0);
	//Shader lightingShader = Shader("shader/colors.vs", "shader/colors.fs");
	
	while (!glfwWindowShouldClose(window))
	{
		Draw(lightingShader);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// 解绑和删除VAO和VBO
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteVertexArrays(1, &vertex_array_object);
	glDeleteBuffers(1, &vertex_buffer_object);

	glfwDestroyWindow(window);

	glfwTerminate();
	return 0;
}
