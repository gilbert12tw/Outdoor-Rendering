#include "src\Shader.h"
#include "src\SceneRenderer.h"
#include <GLFW\glfw3.h>
#include "src\MyImGuiPanel.h"

#include "src\ViewFrustumSceneObject.h"
#include "src\terrain\MyTerrain.h"
#include "src\MyCameraManager.h"

// include assimp
#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

// inlcude stb_image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// include spatial sample
#include "src\SpatialSample.h"

#pragma comment (lib, "lib-vc2015\\glfw3.lib")
#pragma comment(lib, "assimp-vc141-mt.lib")

int FRAME_WIDTH = 1920;
int FRAME_HEIGHT = 1080;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void cursorPosCallback(GLFWwindow* window, double x, double y);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

bool initializeGL();
void resizeGL(GLFWwindow *window, int w, int h);
void paintGL();
void resize(const int w, const int h);

bool m_leftButtonPressed = false;
bool m_rightButtonPressed = false;
double cursorPos[2];

struct Shape {
	unsigned int vao;
	unsigned int vbo_position;
	unsigned int vbo_normal;
	unsigned int vbo_texcoord;
	unsigned int vbo_tangent;
	unsigned int ibo;
	int drawCount;
	unsigned int materialID;
	unsigned int normalMapID;
};

struct DrawCommand {
	GLuint count;
	GLuint instanceCount;
	GLuint firstIndex;
	GLuint baseVertex;
	GLuint baseInstance;
};

// loading
void loadModel(Shape &modelShape, std::string modelPath, int hasTangent);
void loadTexture();
void loadAllSpatialSample();

void compileShaderProgram();
void drawMagicRock();
void drawAirplane();


MyImGuiPanel* m_imguiPanel = nullptr;

void vsyncDisabled(GLFWwindow *window);

// ==============================================
SceneRenderer *defaultRenderer = nullptr;
ShaderProgram* defaultShaderProgram = new ShaderProgram();

ViewFrustumSceneObject* m_viewFrustumSO = nullptr;
MyTerrain* m_terrain = nullptr;
INANOA::MyCameraManager* m_myCameraManager = nullptr;
// ==============================================


void updateWhenPlayerProjectionChanged(const float nearDepth, const float farDepth);
void viewFrustumMultiClipCorner(const std::vector<float> &depths, const glm::mat4 &viewMat, const glm::mat4 &projMat, float *clipCorner);

// ==============================================

Shape magicRock;
ShaderProgram* magicRockShaderProgram;

Shape airplane;
ShaderProgram* airplaneShaderProgram;

// ==============================================
// SSBO
GLuint wholeDataBufferHandle, visibleDataBufferHandle;


int main(){
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(FRAME_WIDTH, FRAME_HEIGHT, "rendering", nullptr, nullptr);
	if (window == nullptr){
		std::cout << "failed to create GLFW window\n";
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

	glfwSetKeyCallback(window, keyCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetFramebufferSizeCallback(window, resizeGL);

	if (initializeGL() == false) {
		std::cout << "initialize GL failed\n";
		glfwTerminate();
		system("pause");
		return 0;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 450");

	// disable vsync
	//glfwSwapInterval(0);

	// start game-loop
	vsyncDisabled(window);
		

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

void vsyncDisabled(GLFWwindow *window) {
	double previousTimeForFPS = glfwGetTime();
	int frameCount = 0;

	static int IMG_IDX = 0;

	while (!glfwWindowShouldClose(window)) {
		// measure speed
		const double currentTime = glfwGetTime();
		frameCount = frameCount + 1;
		const double deltaTime = currentTime - previousTimeForFPS;
		if (deltaTime >= 1.0) {
			m_imguiPanel->setAvgFPS(frameCount * 1.0);
			m_imguiPanel->setAvgFrameTime(deltaTime * 1000.0 / frameCount);

			// reset
			frameCount = 0;
			previousTimeForFPS = currentTime;
		}			

		// teleport
		const int teleportIdx = m_imguiPanel->getTeleportIdx();
		if (teleportIdx != -1) {
			m_myCameraManager->teleport(teleportIdx);
		}

		glfwPollEvents();
		paintGL();
		glfwSwapBuffers(window);
	}
}



bool initializeGL(){
	// initialize shader program
	// vertex shader
	Shader* vsShader = new Shader(GL_VERTEX_SHADER);
	vsShader->createShaderFromFile("src\\shader\\oglVertexShader.glsl");
	std::cout << vsShader->shaderInfoLog() << "\n";

	// fragment shader
	Shader* fsShader = new Shader(GL_FRAGMENT_SHADER);
	fsShader->createShaderFromFile("src\\shader\\oglFragmentShader.glsl");
	std::cout << fsShader->shaderInfoLog() << "\n";

	// shader program
	ShaderProgram* shaderProgram = new ShaderProgram();
	shaderProgram->init();
	shaderProgram->attachShader(vsShader);
	shaderProgram->attachShader(fsShader);
	shaderProgram->checkStatus();
	if (shaderProgram->status() != ShaderProgramStatus::READY) {
		return false;
	}
	shaderProgram->linkProgram();

	vsShader->releaseShader();
	fsShader->releaseShader();
	
	delete vsShader;
	delete fsShader;
	// =================================================================
	// init renderer
	defaultRenderer = new SceneRenderer();
	if (!defaultRenderer->initialize(FRAME_WIDTH, FRAME_HEIGHT, shaderProgram)) {
		return false;
	}

	// =================================================================
	// initialize camera
	m_myCameraManager = new INANOA::MyCameraManager();
	m_myCameraManager->init(FRAME_WIDTH, FRAME_HEIGHT);
	
	// initialize view frustum
	m_viewFrustumSO = new ViewFrustumSceneObject(2, SceneManager::Instance()->m_fs_pixelProcessIdHandle, SceneManager::Instance()->m_fs_pureColor);
	defaultRenderer->appendDynamicSceneObject(m_viewFrustumSO->sceneObject());

	// initialize terrain
	m_terrain = new MyTerrain();
	m_terrain->init(-1); 
	defaultRenderer->appendTerrainSceneObject(m_terrain->sceneObject());
	// =================================================================	
	
	resize(FRAME_WIDTH, FRAME_HEIGHT);
	
	m_imguiPanel = new MyImGuiPanel();		


	// Add Here 
	// =================================================================
	// =================================================================

	// shader program
	compileShaderProgram();
	
	// load model
	loadModel(magicRock, "assets\\MagicRock\\magicRock.obj", 1);
	loadModel(airplane, "assets\\airplane.obj", 0);

	loadTexture();

	// load spatial sample
	loadAllSpatialSample();

	return true;
}

void loadModel(Shape &modelShape, std::string modelPath, int hasTangent) {
	// load model
	// ===========================================
	Assimp::Importer importer;
	
	glGenVertexArrays(1, &modelShape.vao);
	glBindVertexArray(modelShape.vao);

	int flags = aiProcess_Triangulate | aiProcess_FlipUVs;
	if (hasTangent == 1) {
		flags = flags | aiProcess_CalcTangentSpace;
	}
	const aiScene* scene = importer.ReadFile(modelPath, flags);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << "\n";
		return;
	}
	aiMesh* mesh = scene->mMeshes[0];

	// position
	glGenBuffers(1, &modelShape.vbo_position);
	glBindBuffer(GL_ARRAY_BUFFER, modelShape.vbo_position);
	glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(aiVector3D), mesh->mVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// normal
	glGenBuffers(1, &modelShape.vbo_normal);
	glBindBuffer(GL_ARRAY_BUFFER, modelShape.vbo_normal);
	glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(aiVector3D), mesh->mNormals, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// texcoord
	glGenBuffers(1, &modelShape.vbo_texcoord);
	glBindBuffer(GL_ARRAY_BUFFER, modelShape.vbo_texcoord);
	glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(aiVector3D), mesh->mTextureCoords[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// tangent
	if (hasTangent) {
		glGenBuffers(1, &modelShape.vbo_tangent);
		glBindBuffer(GL_ARRAY_BUFFER, modelShape.vbo_tangent);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(aiVector3D), mesh->mTangents, GL_STATIC_DRAW);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	}
	

	// index
	glGenBuffers(1, &modelShape.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelShape.ibo);
	unsigned int *indices = new unsigned int[mesh->mNumFaces * 3];
	for (int i = 0; i < mesh->mNumFaces; i++) {
		indices[i * 3 + 0] = mesh->mFaces[i].mIndices[0];
		indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
		indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
	}
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->mNumFaces * 3 * sizeof(unsigned int), indices, GL_STATIC_DRAW);
	delete[] indices;

	modelShape.drawCount = mesh->mNumFaces * 3;
	
	importer.FreeScene();

	// ===========================================
}

void loadAllSpatialSample() {
	// load spatial sample
	// ===========================================
	SpatialSample* spatialSample[5];
	spatialSample[0] = SpatialSample::importBinaryFile("assets\\poissonPoints_621043_after.ppd2");
	spatialSample[1] = SpatialSample::importBinaryFile("assets\\poissonPoints_2797.ppd2");
	spatialSample[2] = SpatialSample::importBinaryFile("assets\\poissonPoints_1010.ppd2");
	spatialSample[3] = SpatialSample::importBinaryFile("assets\\cityLots_sub_0.ppd2");
	spatialSample[4] = SpatialSample::importBinaryFile("assets\\cityLots_sub_1.ppd2");
    int numSample = spatialSample[0]->numSample() + spatialSample[1]->numSample() + spatialSample[2]->numSample() + spatialSample[3]->numSample() + spatialSample[4]->numSample();
	if (spatialSample == nullptr) {
		std::cout << "Failed to load spatial sample\n";
		return;
	}

    // new a buffer
    float *instanceOffset = new float[numSample * 4];
    int preNumSample = 0;
    for (int c = 0; c < 5; c++) {
        for (int i = 0; i < spatialSample[c]->numSample(); i++) {
            const float *position = spatialSample[c]->position(i);
            instanceOffset[(preNumSample + i) * 3 + 0] = position[0];
            instanceOffset[(preNumSample + i) * 3 + 1] = position[1];
            instanceOffset[(preNumSample + i) * 3 + 2] = position[2];
            instanceOffset[(preNumSample + i) * 3 + 3] = 1.0f;
        }
        preNumSample = preNumSample + spatialSample[c]->numSample();
    }
    // SSBO
    glGenBuffers(1, &wholeDataBufferHandle);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, wholeDataBufferHandle);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, numSample * 4 * sizeof(float), instanceOffset, GL_MAP_READ_BIT);

    // visible SSBO
    glGenBuffers(1, &visibleDataBufferHandle);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, visibleDataBufferHandle);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, numSample * 4 * sizeof(float), nullptr, GL_MAP_READ_BIT);

    // Draw Command
    DrawCommand drawCommands[5];
    drawCommands[0].count = 2442;
    drawCommands[0].instanceCount = 0;
    drawCommands[0].firstIndex = 0;
    drawCommands[0].baseVertex = 0;
    drawCommands[0].baseInstance = 0;

    drawCommands[1].count = 1638;
    drawCommands[1].instanceCount = 0;
    drawCommands[1].firstIndex = 2442;
    drawCommands[1].baseVertex = 641;
    drawCommands[1].baseInstance = spatialSample[0]->numSample();


    drawCommands[2].count = 2640;
    drawCommands[2].instanceCount = 0;
    drawCommands[2].firstIndex = 2442 + 1638;
    drawCommands[2].baseVertex = 999 + 641;
    drawCommands[2].baseInstance = spatialSample[0]->numSample() + spatialSample[1]->numSample();
}

void loadTexture() {

	// load magic rock
	std::string texturePath = "assets\\MagicRock\\StylMagicRocks_AlbedoTransparency.png";
	//std::string texturePath = "assets\\bush01.png";
	int width, height, nrChannels;
	unsigned char *data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
	if (data != nullptr) {
		//std::cout << "width: " << width << " height: " << height << " nrChannels: " << nrChannels << "\n";

		glGenTextures(1, &magicRock.materialID);
		glBindTexture(GL_TEXTURE_2D, magicRock.materialID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		std::cout << "Failed to load texture\n";
	}

	// load normal map
	texturePath = "assets\\MagicRock\\StylMagicRocks_NormalOpenGL.png";
	data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
	if (data) {
		//std::cout << "width: " << width << " height: " << height << " nrChannels: " << nrChannels << "\n";
		glGenTextures(1, &magicRock.normalMapID);
		glBindTexture(GL_TEXTURE_2D, magicRock.normalMapID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		std::cout << "Failed to load texture\n";
	}
	// ===========================================
	// load airplane
	texturePath = "assets\\Airplane_smooth_DefaultMaterial_BaseMap.jpg";
	data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);

	if (data != nullptr) {
		//std::cout << "width: " << width << " height: " << height << " nrChannels: " << nrChannels << "\n";

		glGenTextures(1, &airplane.materialID);
		glBindTexture(GL_TEXTURE_2D, airplane.materialID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		std::cout << "Failed to load texture\n";
	}

	// ===========================================
    // load grass texture

}

void compileShaderProgram() {
	// magic rock
	magicRockShaderProgram = new ShaderProgram();
	magicRockShaderProgram->init();

	// vertex shader
	Shader* vsShader = new Shader(GL_VERTEX_SHADER);
	vsShader->createShaderFromFile("src\\shader\\magicRock.vs.glsl");

	// fragment shader
	Shader* fsShader = new Shader(GL_FRAGMENT_SHADER);
	fsShader->createShaderFromFile("src\\shader\\magicRock.fs.glsl");

	magicRockShaderProgram->attachShader(vsShader);
	magicRockShaderProgram->attachShader(fsShader);
	magicRockShaderProgram->checkStatus();
	if (magicRockShaderProgram->status() != ShaderProgramStatus::READY) {
		return;
	}
	magicRockShaderProgram->linkProgram();
	
	vsShader->releaseShader();
	fsShader->releaseShader();

	// airplane
	airplaneShaderProgram = new ShaderProgram();
	airplaneShaderProgram->init();

	// vertex shader
	vsShader = new Shader(GL_VERTEX_SHADER);
	vsShader->createShaderFromFile("src\\shader\\airplane.vs.glsl");

	// fragment shader
	fsShader = new Shader(GL_FRAGMENT_SHADER);
	fsShader->createShaderFromFile("src\\shader\\airplane.fs.glsl");

	airplaneShaderProgram->attachShader(vsShader);
	airplaneShaderProgram->attachShader(fsShader);
	airplaneShaderProgram->checkStatus();
	if (airplaneShaderProgram->status() != ShaderProgramStatus::READY) {
		return;
	}
	airplaneShaderProgram->linkProgram();
}

void resizeGL(GLFWwindow *window, int w, int h){
	resize(w, h);
}

// Draw
// ==============================================

void drawMagicRock() {

	// shader program
	magicRockShaderProgram->useProgram();

	// uniform
	glm::mat4 viewMat = m_myCameraManager->playerViewMatrix();
	glm::mat4 projMat = m_myCameraManager->playerProjectionMatrix();

	// pass uniform
	unsigned int programId = magicRockShaderProgram->programId();
	glUniformMatrix4fv(glGetUniformLocation(programId, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
	glUniformMatrix4fv(glGetUniformLocation(programId, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));

	// pass texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, magicRock.materialID);
	glUniform1i(glGetUniformLocation(programId, "textureRock"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, magicRock.normalMapID);
	glUniform1i(glGetUniformLocation(programId, "normalMap"), 1);

	// (x, y, w, h)
	const glm::ivec4 playerViewport = m_myCameraManager->playerViewport();

	// (x, y, w, h)
	const glm::ivec4 godViewport = m_myCameraManager->godViewport();

	// set viewport to player viewport
	defaultRenderer->setViewport(playerViewport[0], playerViewport[1], playerViewport[2], playerViewport[3]);
	glBindVertexArray(magicRock.vao);
	glDrawElements(GL_TRIANGLES, magicRock.drawCount, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);

	// set viewport to god viewport
	defaultRenderer->setViewport(godViewport[0], godViewport[1], godViewport[2], godViewport[3]);
	
	// pass uniform
	viewMat = m_myCameraManager->godViewMatrix();
	projMat = m_myCameraManager->godProjectionMatrix();
	glUniformMatrix4fv(glGetUniformLocation(programId, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
	glUniformMatrix4fv(glGetUniformLocation(programId, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));

	glBindVertexArray(magicRock.vao);
	glDrawElements(GL_TRIANGLES, magicRock.drawCount, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

void drawAirplane() {
	// shader program
	airplaneShaderProgram->useProgram();

	// (x, y, w, h)
	const glm::ivec4 playerViewport = m_myCameraManager->playerViewport();
	const glm::ivec4 godViewport = m_myCameraManager->godViewport();

	// uniform
	glm::mat4 viewMat = m_myCameraManager->playerViewMatrix();
	glm::mat4 projMat = m_myCameraManager->playerProjectionMatrix();
	glm::mat4 modelMat = m_myCameraManager->airplaneModelMatrix();
	glm::vec3 offset = m_myCameraManager->airplanePosition();

	//modelMat = glm::translate(modelMat, offset);

	// pass uniform
	unsigned int programId = airplaneShaderProgram->programId();
	glUniformMatrix4fv(glGetUniformLocation(programId, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
	glUniformMatrix4fv(glGetUniformLocation(programId, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));
	glUniformMatrix4fv(glGetUniformLocation(programId, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
	glUniform3fv(glGetUniformLocation(programId, "offset"), 1, glm::value_ptr(offset));

	// texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, airplane.materialID);
	glUniform1i(glGetUniformLocation(programId, "textureAirplane"), 0);

	glBindVertexArray(airplane.vao);

	// set viewport to player viewport
	defaultRenderer->setViewport(playerViewport[0], playerViewport[1], playerViewport[2], playerViewport[3]);
	glDrawElements(GL_TRIANGLES, airplane.drawCount, GL_UNSIGNED_INT, nullptr);

	// set viewport to god viewport
	defaultRenderer->setViewport(godViewport[0], godViewport[1], godViewport[2], godViewport[3]);

	// pass uniform
	viewMat = m_myCameraManager->godViewMatrix();
	projMat = m_myCameraManager->godProjectionMatrix();
	glUniformMatrix4fv(glGetUniformLocation(programId, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
	glUniformMatrix4fv(glGetUniformLocation(programId, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));

	glDrawElements(GL_TRIANGLES, airplane.drawCount, GL_UNSIGNED_INT, nullptr);
}


void paintGL() {
	// update cameras and airplane
	// god camera
	m_myCameraManager->updateGodCamera();
	// player camera
	m_myCameraManager->updatePlayerCamera();
	const glm::vec3 PLAYER_CAMERA_POSITION = m_myCameraManager->playerViewOrig();
	m_myCameraManager->adjustPlayerCameraHeight(m_terrain->terrainData()->height(PLAYER_CAMERA_POSITION.x, PLAYER_CAMERA_POSITION.z));
	// airplane
	m_myCameraManager->updateAirplane();
	const glm::vec3 AIRPLANE_POSTION = m_myCameraManager->airplanePosition();
	m_myCameraManager->adjustAirplaneHeight(m_terrain->terrainData()->height(AIRPLANE_POSTION.x, AIRPLANE_POSTION.z));

	// prepare parameters
	const glm::mat4 playerVM = m_myCameraManager->playerViewMatrix();
	const glm::mat4 playerProjMat = m_myCameraManager->playerProjectionMatrix();
	const glm::vec3 playerViewOrg = m_myCameraManager->playerViewOrig();

	const glm::mat4 godVM = m_myCameraManager->godViewMatrix();
	const glm::mat4 godProjMat = m_myCameraManager->godProjectionMatrix();

	const glm::mat4 airplaneModelMat = m_myCameraManager->airplaneModelMatrix();

	// (x, y, w, h)
	const glm::ivec4 playerViewport = m_myCameraManager->playerViewport();

	// (x, y, w, h)
	const glm::ivec4 godViewport = m_myCameraManager->godViewport();

	// ====================================================================================
	// update player camera view frustum
	m_viewFrustumSO->updateState(playerVM, playerViewOrg);

	// update geography
	m_terrain->updateState(playerVM, playerViewOrg, playerProjMat, nullptr);
	// =============================================

	// =============================================
	// start rendering
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// start new frame
	defaultRenderer->setViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);
	defaultRenderer->startNewFrame();

	// rendering with player view		
	defaultRenderer->setViewport(playerViewport[0], playerViewport[1], playerViewport[2], playerViewport[3]);
	defaultRenderer->setView(playerVM);
	defaultRenderer->setProjection(playerProjMat);
	defaultRenderer->renderPass();

	// rendering with god view
	defaultRenderer->setViewport(godViewport[0], godViewport[1], godViewport[2], godViewport[3]);
	defaultRenderer->setView(godVM);
	defaultRenderer->setProjection(godProjMat);
	defaultRenderer->renderPass();
	// ===============================

	drawMagicRock();
	drawAirplane();

	// =============================================

	ImGui::Begin("My name is window");
	m_imguiPanel->update();
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

////////////////////////////////////////////////
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		m_myCameraManager->mousePress(RenderWidgetMouseButton::M_LEFT, cursorPos[0], cursorPos[1]);
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		m_myCameraManager->mouseRelease(RenderWidgetMouseButton::M_LEFT, cursorPos[0], cursorPos[1]);
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		m_myCameraManager->mousePress(RenderWidgetMouseButton::M_RIGHT, cursorPos[0], cursorPos[1]);
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		m_myCameraManager->mouseRelease(RenderWidgetMouseButton::M_RIGHT, cursorPos[0], cursorPos[1]);
	}
}
void cursorPosCallback(GLFWwindow* window, double x, double y){
	cursorPos[0] = x;
	cursorPos[1] = y;

	m_myCameraManager->mouseMove(x, y);
}
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	auto setKeyStatus = [](const RenderWidgetKeyCode code, const int action) {
		if (action == GLFW_PRESS) {
			m_myCameraManager->keyPress(code);
		}
		else if (action == GLFW_RELEASE) {
			m_myCameraManager->keyRelease(code);
		}
		};

	// =======================================
	if (key == GLFW_KEY_W) { setKeyStatus(RenderWidgetKeyCode::KEY_W, action); }
	else if (key == GLFW_KEY_A) { setKeyStatus(RenderWidgetKeyCode::KEY_A, action); }
	else if (key == GLFW_KEY_S) { setKeyStatus(RenderWidgetKeyCode::KEY_S, action); }
	else if (key == GLFW_KEY_D) { setKeyStatus(RenderWidgetKeyCode::KEY_D, action); }
	else if (key == GLFW_KEY_T) { setKeyStatus(RenderWidgetKeyCode::KEY_T, action); }
	else if (key == GLFW_KEY_Z) { setKeyStatus(RenderWidgetKeyCode::KEY_Z, action); }
	else if (key == GLFW_KEY_X) { setKeyStatus(RenderWidgetKeyCode::KEY_X, action); }
	else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	// teleport 0, 1, 2, 3
	else if (key == GLFW_KEY_0 && action == GLFW_PRESS) {
		m_myCameraManager->teleport(0);
	}
	else if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		m_myCameraManager->teleport(1);
	}
	else if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		m_myCameraManager->teleport(2);
	}

}
void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {}

void updateWhenPlayerProjectionChanged(const float nearDepth, const float farDepth) {
	// get view frustum corner
	const int NUM_CASCADE = 2;
	const float HY = 0.0;

	float dOffset = (farDepth - nearDepth) / NUM_CASCADE;
	float *corners = new float[(NUM_CASCADE + 1) * 12];
	std::vector<float> depths(NUM_CASCADE + 1);
	for (int i = 0; i < NUM_CASCADE; i++) {
		depths[i] = nearDepth + dOffset * i;
	}
	depths[NUM_CASCADE] = farDepth;
	// get viewspace corners
	glm::mat4 tView = glm::lookAt(glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	// calculate corners of view frustum cascade
	viewFrustumMultiClipCorner(depths, tView, m_myCameraManager->playerProjectionMatrix(), corners);
	
	// update view frustum scene object
	for (int i = 0; i < NUM_CASCADE + 1; i++) {
		float *layerBuffer = m_viewFrustumSO->cascadeDataBuffer(i);
		for (int j = 0; j < 12; j++) {
			layerBuffer[j] = corners[i * 12 + j];
		}
	}
	m_viewFrustumSO->updateDataBuffer();

	delete corners;
}
void resize(const int w, const int h) {
	FRAME_WIDTH = w;
	FRAME_HEIGHT = h;

	m_myCameraManager->resize(w, h);
	defaultRenderer->resize(w, h);
	updateWhenPlayerProjectionChanged(0.1, m_myCameraManager->playerCameraFar());
}
void viewFrustumMultiClipCorner(const std::vector<float> &depths, const glm::mat4 &viewMat, const glm::mat4 &projMat, float *clipCorner) {
	const int NUM_CLIP = depths.size();

	// Calculate Inverse
	glm::mat4 viewProjInv = glm::inverse(projMat * viewMat);

	// Calculate Clip Plane Corners
	int clipOffset = 0;
	for (const float depth : depths) {
		// Get Depth in NDC, the depth in viewSpace is negative
		glm::vec4 v = glm::vec4(0, 0, -1 * depth, 1);
		glm::vec4 vInNDC = projMat * v;
		if (fabs(vInNDC.w) > 0.00001) {
			vInNDC.z = vInNDC.z / vInNDC.w;
		}
		// Get 4 corner of clip plane
		float cornerXY[] = {
			-1, 1,
			-1, -1,
			1, -1,
			1, 1
		};
		for (int j = 0; j < 4; j++) {
			glm::vec4 wcc = {
				cornerXY[j * 2 + 0], cornerXY[j * 2 + 1], vInNDC.z, 1
			};
			wcc = viewProjInv * wcc;
			wcc = wcc / wcc.w;

			clipCorner[clipOffset * 12 + j * 3 + 0] = wcc[0];
			clipCorner[clipOffset * 12 + j * 3 + 1] = wcc[1];
			clipCorner[clipOffset * 12 + j * 3 + 2] = wcc[2];
		}
		clipOffset = clipOffset + 1;
	}
}
