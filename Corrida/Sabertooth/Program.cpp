#include <cstdlib>
#include "Program.h"
#include "Mesh.h"

// functions
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
bool checkCollision(glm::vec3 cubePosition, glm::vec3 shotPosition);

// camera
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// mouse
bool firstMouse = true;
float yawa = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitcha = 0.0f;
float lastX = 800.0f / 2.0f;
float lastY = 600.0 / 2.0f;
float fov = 45.0f;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// gun
bool shotPressed = false;
int iterations = 0;
glm::vec3 shotMove;
glm::vec3 lastCameraFront;

// aux
int cont = 0;
bool collision[11] = {
	false, false, false, false, false,
	false, false, false, false, false, false
};

// lighting
glm::vec3 lightPos(0.0f, 2.2f, 0.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

// car
int pistaPos = 0;

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

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "FPS_Game", nullptr, nullptr);

	if (window == nullptr) {
		std::cout << "Failed to create GLFW Window" << std::endl;
		glfwTerminate();

		return 0;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

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
	lightShader = Shader("Shaders/Core/core2.vert", "Shaders/Core/core2.frag");
	carShader = Shader("Shaders/Core/core3.vert", "Shaders/Core/core3.frag");
	coreShader.Use();

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	return window;
}

void Program::Run(Shader shader, GLFWwindow* window, GLuint VAO, GLuint VAOlight, glm::vec3 objectsPositions[], glm::vec3 cubesPositions[], vector<Mesh*> meshs, vector<vec3*> objPoints)
{
	coreShader.Use();

	for (Mesh* mesh : meshs) {
		for (Group* group : mesh->getGroups()) {
			Material* material = mesh->getMaterial(group->getMaterial());
			if (group->getName() == "pista") {
				coreShader.LoadTexture(_strdup(material->getTexture().c_str()), "texture1", group->getName());
				coreShader.setVec3("materialAmbient", vec3(material->getAmbient()->x, material->getAmbient()->y, material->getAmbient()->z));
				coreShader.setVec3("materialDiffuse", vec3(material->getDiffuse()->x, material->getDiffuse()->y, material->getDiffuse()->z));
				coreShader.setVec3("materialSpecular", vec3(material->getSpecular()->x, material->getSpecular()->y, material->getSpecular()->z));
				coreShader.setFloat("materialShininess", material->getShininess());
			}
			else {
				carShader.LoadTexture(_strdup(material->getTexture().c_str()), "texture1", group->getName());
				carShader.setVec3("materialAmbient", vec3(material->getAmbient()->x, material->getAmbient()->y, material->getAmbient()->z));
				carShader.setVec3("materialDiffuse", vec3(material->getDiffuse()->x, material->getDiffuse()->y, material->getDiffuse()->z));
				carShader.setVec3("materialSpecular", vec3(material->getSpecular()->x, material->getSpecular()->y, material->getSpecular()->z));
				carShader.setFloat("materialShininess", material->getShininess());
			}
			

			vector<float> vertices;
			vector<float> normais;
			vector<float> textures;

			for (Face* face : group->getFaces()) {
				for (int verticeID : face->getVertices()) {
					glm::vec3* vertice = mesh->vertice(verticeID - 1);
					vertices.push_back(vertice->x);
					vertices.push_back(vertice->y);
					vertices.push_back(vertice->z);

					group->increaseNumVertices();
				}

				for (int normalID : face->getNormais()) {
					glm::vec3* normal = mesh->normal(normalID - 1);
					normais.push_back(normal->x);
					normais.push_back(normal->y);
					normais.push_back(normal->z);
				}

				for (int textureID : face->getTextures()) {
					glm::vec2* texture = mesh->texture(textureID - 1);
					textures.push_back(texture->x);
					textures.push_back(texture->y);
				}
			}

			GLuint VBOvertices, VBOnormais, VBOtextures, VAO;
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBOvertices);
			glGenBuffers(1, &VBOnormais);
			glGenBuffers(1, &VBOtextures);

			// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
			glBindVertexArray(VAO);

			// Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VBOvertices);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(0);

			// Normais
			glBindBuffer(GL_ARRAY_BUFFER, VBOnormais);
			glBufferData(GL_ARRAY_BUFFER, normais.size() * sizeof(float), normais.data(), GL_STATIC_DRAW);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(1);

			// Textures
			glBindBuffer(GL_ARRAY_BUFFER, VBOtextures);
			glBufferData(GL_ARRAY_BUFFER, textures.size() * sizeof(float), textures.data(), GL_STATIC_DRAW);

			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(2);

			group->setVAO(&VAO);
			glBindVertexArray(0); // Unbind VAO
		}
	}
	// game loop
	while (!glfwWindowShouldClose(window)) {

		glfwPollEvents();

		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		coreShader.Use();

		glm::mat4 view(1.0f);
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		// pass projection matrix to shader (note that in this case it could change every frame)
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
		coreShader.setMatrix4fv("projection", proj);
		coreShader.setMatrix4fv("view", view);
		coreShader.setVec3("ligthDiff", glm::vec3(0.5f, 0.5f, 0.5f));
		coreShader.setVec3("ligthSpec", glm::vec3(1.0f, 1.0f, 1.0f));
		coreShader.setVec3("lightColor", lightColor);
		coreShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
		coreShader.setVec3("viewPos", cameraPos);

		proj = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
		carShader.setMatrix4fv("projection", proj);
		carShader.setMatrix4fv("view", view);
		carShader.setVec3("ligthDiff", glm::vec3(0.5f, 0.5f, 0.5f));
		carShader.setVec3("ligthSpec", glm::vec3(1.0f, 1.0f, 1.0f));
		carShader.setVec3("lightColor", lightColor);
		carShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
		carShader.setVec3("viewPos", cameraPos);

		glm::mat4 model(1.0f);
		coreShader.setMatrix4fv("model", model);
		carShader.setMatrix4fv("model", model);
		glUniform1i(glGetUniformLocation(coreShader.program, "tex"), 0);
		for (Mesh* mesh : meshs) {
			for (Group* group : mesh->getGroups()) {
				if (group->getName() == "pista") {
					model = glm::mat4(1.0f);
					coreShader.UseTexture(group->getName());
					glBindVertexArray(group->getVAO());
					coreShader.setMatrix4fv("model", model);
					glDrawArrays(GL_TRIANGLES, 0, group->getNumVertices());
					glBindVertexArray(0);
				}
				else {
					model = glm::mat4(1.0f);
					carShader.UseTexture(group->getName());
					glBindVertexArray(group->getVAO());
					if (pistaPos == objPoints.size())
						pistaPos = 0;

					int aux;
					if (pistaPos == objPoints.size() - 1) {
						aux = 0;
					}
					else
					{
						aux = pistaPos + 1;
					}
					glm::vec3 a = (*objPoints[pistaPos]);
					glm::vec3 b = (*objPoints[aux]);

					GLfloat dx = b.x - a.x;
					GLfloat dy = b.y - a.y;
					GLfloat dz = b.z - a.z;

					GLfloat angle = -glm::atan(dz, dx);
					//GLfloat angleY = -glm::atan(dx, dy);

					if (!collision[10]) {
						model = translate(model, *objPoints[pistaPos]);
						model = rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
						//model = rotate(model, angleY, glm::vec3(0.0f, 0.0f, 1.0f));
						model = scale(model, glm::vec3(0.5, 0.5, 0.5));
						coreShader.setMatrix4fv("model", model);
						glDrawArrays(GL_TRIANGLES, 0, group->getNumVertices());
						glBindVertexArray(0);
					}
				}
			}
		}

		// render boxes
		// be sure to activate shader when setting uniforms/drawing objects
		glUniform1i(glGetUniformLocation(coreShader.program, "tex"), 1);
		coreShader.setVec3("objectColor", glm::vec3(0.19f, 0.0f, 0.39f));
		coreShader.Use();
		glBindVertexArray(VAO);
		for (unsigned int i = 1; i < 10; i++)
		{
			if (!collision[i]) {
				// calculate the model matrix for each object and pass it to shader before drawing
				model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
				model = glm::translate(model, cubesPositions[i]);
				coreShader.setMatrix4fv("model", model);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
		}

		lightShader.Use();
		lightShader.setMatrix4fv("projection", proj);
		lightShader.setMatrix4fv("view", view);
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);
		glUniform1i(glGetUniformLocation(coreShader.program, "tex"), 1);
		glBindVertexArray(VAOlight);
		// calculate the model matrix for each object and pass it to shader before drawing
		model = glm::translate(model, cubesPositions[0]);
		lightShader.setMatrix4fv("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// shots
		coreShader.Use();
		glUniform1i(glGetUniformLocation(coreShader.program, "tex"), 2);
		glBindVertexArray(VAO);
		model = glm::mat4(1.0f);
		
		if (shotPressed) {
			if (iterations == 0) {
				glm::vec3 temp = cameraPos;
				temp[1] = 1.8f;
				shotMove = temp;
				lastCameraFront = cameraFront;
			}
			iterations += 1;
			model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
			model = glm::translate(model, shotMove);
			model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
			float shotSpeed = 70.0 * deltaTime;
			shotMove += shotSpeed * lastCameraFront;
			coreShader.setMatrix4fv("model", model);

			glDrawArrays(GL_TRIANGLES, 0, 36);

			if (iterations >= 150) {
				shotPressed = false;
				iterations = 0;
			}

			for (int i = 0; i < 10; i++) {
				if (!collision[i]) {
					collision[i] = checkCollision(cubesPositions[i], shotMove);
				}
			}
			if (!collision[10])
				collision[10] = checkCollision(*objPoints[pistaPos], shotMove);
		}
		pistaPos++;
		glfwSwapBuffers(window);
	}
}

void Program::Finish(GLuint VAO, GLuint VAO1)
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteVertexArrays(1, &VAO1);
	coreShader.Delete();
	lightShader.Delete();
	glfwTerminate();
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraSpeed = 8.5 * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		shotPressed = true;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.4f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yawa += xoffset;
	pitcha += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (pitcha > 89.0f)
		pitcha = 89.0f;
	if (pitcha < -89.0f)
		pitcha = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yawa)) * cos(glm::radians(pitcha));
	front.y = sin(glm::radians(pitcha));
	front.z = sin(glm::radians(yawa)) * cos(glm::radians(pitcha));
	cameraFront = glm::normalize(front);
}

// chech collision
bool checkCollision(glm::vec3 cubePosition, glm::vec3 shotPosition) {
	float radC = 0.8f;
	float radS = 0.4f;
	float rad = radC + radS;
	float x = cubePosition.x - shotPosition.x;
	float y = cubePosition.y - shotPosition.y;
	float z = cubePosition.z - shotPosition.z;

	float interesectC = pow(x, 2) + pow(y, 2) + pow(z, 2);
	float distC = pow(rad, 2);
	if (interesectC < distC) {
		return true;
	}
	else {
		return false;
	}
}