#include <cstdlib>
#include "Program.h"
#include <windows.h>

// functions
void process_input(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void bspline_curve();
float calculate_bspline_curve(float& t, float& t3, float& t2, float& a1, float& a2, float& a3, float& a4);
void bspline_inex();
void create_object();
void check_collision(double posX, double posY);
void reset();

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// mouse
double posX, posY;
double lastX, lastY;

// VAOs e VBOs
GLuint VAO_POINTS, VBO_POINTS;
GLuint VAO_BSPLINE_CURVE, VBO_BSPLINE_CURVE;
GLuint VAO_BSPLINE_INTERNAL, VBO_BSPLINE_INTERNAL;
GLuint VAO_BSPLINE_EXTERNAL, VBO_BSPLINE_EXTERNAL;

// vetor
vector<float> points_array, bspline_curve_array, bspline_internal_array, bspline_external_array;

// aux
bool isDone, once, isPointSelected = false;
int lastPoint = 0.0;

Program::Program() {}

Program::~Program() {}

GLFWwindow* Program::GLFWInit()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Pista", nullptr, nullptr);

	if (window == nullptr) {
		std::cout << "Failed to create GLFW Window" << std::endl;
		glfwTerminate();

		return 0;
	}

	glfwMakeContextCurrent(window);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK) {
		std::cout << "Failed no init GLEW." << std::endl;
		return 0;
	}
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
	glViewport(0, 0, screenWidth, screenHeight);

	coreShader = Shader("Shaders/Core/core.vert", "Shaders/Core/core.frag");
	coreShader.Use();
	coreShader.setMatrix4fv("projection", glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f));

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl << endl;
	// Info
	cout << "Comandos:" << endl;
	cout << "ESC - Sair" << endl;
	cout << "Espaço - Gerar BSpline" << endl;
	cout << "P - Gerar objeto" << endl;
	cout << "R - Resetar" << endl;
	cout << "Seta UP - Valor +1" << endl;
	cout << "Seta DOWN - Valor -1" << endl;
	cout << "W - Valor +10" << endl;
	cout << "S - Valor -10" << endl;

	return window;
}

void Program::Run(Shader shader, GLFWwindow* window)
{
	coreShader.Use();

	glGenBuffers(1, &VBO_POINTS);
	glGenVertexArrays(1, &VAO_POINTS);

	glGenBuffers(1, &VBO_BSPLINE_CURVE);
	glGenVertexArrays(1, &VAO_BSPLINE_CURVE);

	glGenBuffers(1, &VBO_BSPLINE_INTERNAL);
	glGenVertexArrays(1, &VAO_BSPLINE_INTERNAL);

	glGenBuffers(1, &VBO_BSPLINE_EXTERNAL);
	glGenVertexArrays(1, &VAO_BSPLINE_EXTERNAL);

	GLint color = glGetUniformLocation(coreShader.program, "FragPos");

	// game loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		process_input(window);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		coreShader.Use();

		glUniform3f(color, 1.0, 0.0, 0.0);
		glBindVertexArray(VAO_POINTS);
		glPointSize(10);
		glDrawArrays(GL_POINTS, 0, (GLsizei)points_array.size() / 3);

		if (isDone) {
			if (!once) {
				bspline_curve();
				once = true;
			}
			glUniform3f(color, 1.0, 1.0, 1.0);
			glBindVertexArray(VAO_BSPLINE_CURVE);
			glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)bspline_curve_array.size() / 3);

			glUniform3f(color, 0.0, 1.0, 0.0);
			glBindVertexArray(VAO_BSPLINE_INTERNAL);
			glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)bspline_internal_array.size() / 3);

			glBindVertexArray(VAO_BSPLINE_EXTERNAL);
			glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)bspline_external_array.size() / 3);
		}

		glfwSwapBuffers(window);
	}
}

void Program::Finish()
{
	glDeleteVertexArrays(1, &VAO_POINTS);
	glDeleteVertexArrays(1, &VAO_BSPLINE_CURVE);
	glDeleteVertexArrays(1, &VAO_BSPLINE_INTERNAL);
	glDeleteBuffers(1, &VBO_POINTS);
	glDeleteBuffers(1, &VBO_BSPLINE_CURVE);
	glDeleteBuffers(1, &VBO_BSPLINE_INTERNAL);
	points_array.clear();
	bspline_curve_array.clear();
	bspline_internal_array.clear();
	coreShader.Delete();
	glfwTerminate();
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void process_input(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
		
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		if (isDone)
			create_object();
		Sleep(300);
	}

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isDone) {
		isDone = true;
		cout << endl << "Selecione um ponto de controle e altere sua altura com as setas para cima e para baixo.";
	}

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		reset();
		Sleep(100);
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		if (isDone && isPointSelected) {
			points_array[lastPoint] += 1;
			cout << "                       Valor = " << points_array[lastPoint] << endl;
		}
		Sleep(100);
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		if (isDone && isPointSelected) {
			points_array[lastPoint] -= 1;
			cout << "                       Valor = " << points_array[lastPoint] << endl;
		}
		Sleep(100);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		if (isDone && isPointSelected) {
			points_array[lastPoint] += 10;
			cout << "                       Valor = " << points_array[lastPoint] << endl;
		}
		Sleep(100);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		if (isDone && isPointSelected) {
			points_array[lastPoint] -= 10;
			cout <<                        "Valor = " << points_array[lastPoint] << endl;
		}
		Sleep(100);
	}
}

// glfw: whenever the mouse is clicked, this callback is called
// -------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (!isDone) {
			glfwGetCursorPos(window, &posX, &posY);

			lastX = posX;
			lastY = posY;

			//cout << "X-" << posX;
			//cout << "  Y-" << posY << endl;

			points_array.push_back(posX);
			points_array.push_back(posY);
			points_array.push_back(0.0f);

			glBindVertexArray(VAO_POINTS);
			glBindBuffer(GL_ARRAY_BUFFER, VBO_POINTS);
			glBufferData(GL_ARRAY_BUFFER, points_array.size() * sizeof(float), points_array.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(0);
		}
		else {
			glfwGetCursorPos(window, &posX, &posY);
			check_collision(posX, posY);
		}
	}
}

void bspline_curve() {
	bspline_curve_array.clear();
	bspline_internal_array.clear();
	bspline_external_array.clear();
	int size = points_array.size();

	for (int i = 0; i < size + 3; i += 3) {
		for (float t = 0; t < 1; t += 0.03f) { // 33 variacoes da dsitancia
			float t3 = pow(t, 3);
			float t2 = pow(t, 2);

			float x = calculate_bspline_curve(t, t3, t2, points_array[(i + 0) % size],
				points_array[(i + 3) % size], points_array[(i + 6) % size], points_array[(i + 9) % size]);
			float y = calculate_bspline_curve(t, t3, t2, points_array[(i + 1) % size],
				points_array[(i + 4) % size], points_array[(i + 7) % size], points_array[(i + 10) % size]);
			float z = calculate_bspline_curve(t, t3, t2, points_array[(i + 2) % size],
				points_array[(i + 5) % size], points_array[(i + 8) % size], points_array[(i + 11) % size]);

			bspline_curve_array.push_back(x);
			bspline_curve_array.push_back(y);
			bspline_curve_array.push_back(z);
		}
	}

	glBindVertexArray(VAO_BSPLINE_CURVE);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_BSPLINE_CURVE);
	glBufferData(GL_ARRAY_BUFFER, bspline_curve_array.size() * sizeof(float), bspline_curve_array.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);
	
	bspline_inex();
}

float calculate_bspline_curve(float& t, float& t3, float& t2, float& a1, float& a2, float& a3, float& a4) {
	return (
		(
			(-1 * t3 + 3 * t2 - 3 * t + 1) * a1 +
			(+3 * t3 - 6 * t2 + 0 * t + 4) * a2 +
			(-3 * t3 + 3 * t2 + 3 * t + 1) * a3 +
			(+1 * t3 + 0 * t2 + 0 * t + 0) * a4
			) / 6.0f);
}


void bspline_inex() {
	int size = bspline_curve_array.size();
	float curveDist = 20.0f;

	for (int i = 0; i < size - 3; i += 3) {
		float Ax = bspline_curve_array[(i + 0) % size];
		float Ay = bspline_curve_array[(i + 1) % size];
		float Ac = bspline_curve_array[(i + 2) % size];
		float Bx = bspline_curve_array[(i + 3) % size];
		float By = bspline_curve_array[(i + 4) % size];
		float Bc = bspline_curve_array[(i + 5) % size];

		float w = Bx - Ax;
		float height = By - Ay;
		float angle = atan(height / w);

		float angleIn, angleEx;
		if (w < 0) {
			angleIn = angle + M_PI / 2;
			angleEx = angle - M_PI / 2;
		}
		else {
			angleIn = angle - M_PI / 2;
			angleEx = angle + M_PI / 2;
		}
		float angleInEx = cos(angleIn) * curveDist + Ax;
		bspline_internal_array.push_back(angleInEx);
		angleInEx = sin(angleIn) * curveDist + Ay;
		bspline_internal_array.push_back(angleInEx);
		bspline_internal_array.push_back(Ac);

		angleInEx = cos(angleEx) * curveDist + Ax;
		bspline_external_array.push_back(angleInEx);
		angleInEx = sin(angleEx) * curveDist + Ay;
		bspline_external_array.push_back(angleInEx);
		bspline_external_array.push_back(Ac);
	}

	glBindVertexArray(VAO_BSPLINE_INTERNAL);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_BSPLINE_INTERNAL);
	glBufferData(GL_ARRAY_BUFFER, bspline_internal_array.size() * sizeof(float), bspline_internal_array.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	glBindVertexArray(VAO_BSPLINE_EXTERNAL);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_BSPLINE_EXTERNAL);
	glBufferData(GL_ARRAY_BUFFER, bspline_external_array.size() * sizeof(float), bspline_external_array.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);
}

void create_object() {
	bspline_curve();

	ofstream coords("../PistaObjeto/coords.txt");
	float temp = bspline_curve_array.size() / ((float)points_array.size() / 3);
	for (int i = 0; i < bspline_curve_array.size() - temp; i += 3) {
		coords << bspline_curve_array[i] << " " << bspline_curve_array[i + 2] << " " << bspline_curve_array[i + 1] << endl;
	}
	coords.close();

	ofstream obj("../PistaObjeto/pista.obj");
	obj << "mtllib " << "Objects/pista.mtl" << endl;
	obj << "g " << "pista" << endl;
	obj << "usemtl " << "pista" << endl;
	obj << "vn 0.0 1.0 0.0" << endl;
	obj << "vt 0.0 0.0" << endl;
	obj << "vt 0.0 1.0" << endl;
	obj << "vt 1.0 0.0" << endl;
	obj << "vt 1.0 1.0" << endl;

	int size = bspline_internal_array.size();
	int vertices_size = size / 3;
	for (int i = 0; i < size; i += 3) {
		obj << "v " << (bspline_internal_array[i]) << " " << bspline_internal_array[i + 2] << " " << (bspline_internal_array[i + 1]) << endl;
	}
	for (int i = 0; i < size; i += 3) {
		obj << "v " << (bspline_external_array[i]) << " " << bspline_external_array[i + 2] << " " << (bspline_external_array[i + 1]) << endl;
	}

	for (int i = 1; i <= size / 3 - 3; i++) {
		obj << "f " << i << "/1/1 " << (i + 1) << "/2/1 " << i + vertices_size << "/4/1" << endl;
		obj << "f " << i + vertices_size << "/4/1 " << (i + 1) << "/2/1 " << i + 1 + vertices_size << "/3/1" << endl;
	}
	obj.close();

	ofstream mtl("../PistaObjeto/pista.mtl");
	mtl << "newmtl " << "pista" << endl;
	mtl << "Ka 0.5 0.5 0.5" << endl;
	mtl << "Kd 1.0 1.0 1.0" << endl;
	mtl << "Ks 0.0 0.0 0.0" << endl;
	mtl << "Ns 80.0" << endl;
	mtl << "map_Kd " << "Objects/pista.jpg" << endl;
	mtl.close();
}

void check_collision(double posX, double posY) {
	for (int i = 0; i < points_array.size(); i += 3) {
		float x = points_array[i];
		float y = points_array[i + 1];
		float z = points_array[i + 2];
		int zPos = i + 2;

		float xMin = x - 5.0f;
		float xMax = x + 5.0f;
		float yMin = y - 5.0f;
		float yMax = y + 5.0f;

		if (posX <= xMax && posX >= xMin && posY >= yMin && posY <= yMax) {
			cout << endl << "Ponto de controle: " << i / 3;
			cout << " - Valor = " << z << endl;
			lastPoint = zPos;
			isPointSelected = true;
		}
	}
}

void reset() {
	isDone = false;
	once = false;
	points_array.clear();
	bspline_curve_array.clear();
	bspline_internal_array.clear();
	bspline_external_array.clear();
}