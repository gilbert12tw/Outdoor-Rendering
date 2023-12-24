#include "src\Shader.h"
#include "src\SceneRenderer.h"
#include <GLFW\glfw3.h>
#include "src\MyImGuiPanel.h"
#include <glm/gtx/quaternion.hpp>

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
#include "src\MyPoissonSample.h"

#pragma comment (lib, "lib-vc2015\\glfw3.lib")
#pragma comment(lib, "assimp-vc141-mt.lib")

int FRAME_WIDTH = 1920;
int FRAME_HEIGHT = 1440;

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

struct {
    GLuint fbo;
    GLuint position_map;
    GLuint normal_map;
    GLuint diffuse_map;
    GLuint depth_map;
    GLuint vao;
} gbuffer;

struct {
    GLuint fbo;
    GLuint depth_map;
    GLuint vao;
    GLuint vbo;
} hiz;

// loading
void loadModel(Shape &modelShape, std::string modelPath, int hasTangent);
void loadTexture();
void loadAllSpatialSample();
void loadIndirectModel();

void computeDrawCommands();
void setUpGbuffer();

void compileShaderProgram();
void drawMagicRock();
void drawAirplane();
void drawIndirectRender();
void drawBuilding();
void drawFoliage();
void drawGbuffer();

void genHizMap();
void flushDepth();

void processBtnInput();

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

Shape mergeObject;
ShaderProgram* indirectRenderShaderProgram;

ShaderProgram* cullingShaderProgram, *clearShaderProgram;

// shader program
ShaderProgram* gbufferShaderProgram;

ShaderProgram* hizShaderProgram;
ShaderProgram* hizMipMapShaderProgram;

// ==============================================

// SSBO handle
GLuint wholeDataBufferHandle, visibleDataBufferHandle, cmdBufferHandle, indirectBuildingBufferHandle;

// ==============================================
// uniform variable
bool normalMaping = true;
int gBufferIdx = 5;
int depthLevel = 0;
bool enableHiz = true;

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

        processBtnInput();

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

    // Gbuffer
    setUpGbuffer();

	// shader program
	compileShaderProgram();
	
	// load model
	loadModel(magicRock, "assets\\MagicRock\\magicRock.obj", 1);
	loadModel(airplane, "assets\\airplane.obj", 0);
    loadIndirectModel();

    stbi_set_flip_vertically_on_load(true);
	loadTexture();

	// load spatial sample
	loadAllSpatialSample();

	return true;
}

void setUpGbuffer() {
    glGenFramebuffers(1, &gbuffer.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);

    // for diffuse
    glGenTextures(1, &gbuffer.diffuse_map);
    glBindTexture(GL_TEXTURE_2D, gbuffer.diffuse_map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // for normal
    glGenTextures(1, &gbuffer.normal_map);
    glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // for coordinate
    glGenTextures(1, &gbuffer.position_map);
    glBindTexture(GL_TEXTURE_2D, gbuffer.position_map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // for depth
    glGenTextures(1, &gbuffer.depth_map);
    glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map);
    //glTexStorage2D(GL_TEXTURE_2D,1,GL_DEPTH_COMPONENT32F, FRAME_WIDTH, FRAME_HEIGHT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    // attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gbuffer.diffuse_map, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gbuffer.normal_map, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gbuffer.position_map, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbuffer.depth_map, 0);

    // vao
    glGenVertexArrays(1, &gbuffer.vao);
    glBindVertexArray(gbuffer.vao);

    // vbo
    GLuint gbufferVBO;
    glGenBuffers(1, &gbufferVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gbufferVBO);

    // vertex
    float verts[18] = {
            -1.0, -1.0, 0.5,
            1.0, -1.0, 0.5,
            -1.0, 1.0, 0.5,

            -1.0, 1.0, 0.5,
            1.0, -1.0, 0.5,
            1.0, 1.0, 0.5

    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);


    // for Hi-z
    glGenFramebuffers(1, &hiz.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, hiz.fbo);

    // for depth
    glGenTextures(1, &hiz.depth_map);
    glBindTexture(GL_TEXTURE_2D, hiz.depth_map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    // mip map (Hi-z)
    glGenerateMipmap(GL_TEXTURE_2D);

    // attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, hiz.depth_map, 0);

    // vao
    glGenVertexArrays(1, &hiz.vao);
    glBindVertexArray(hiz.vao);

    // vbo
    glGenBuffers(1, &hiz.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, hiz.vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
}

void loadModel(Shape &modelShape, std::string modelPath, int hasTangent) {
	// load model
	// ===========================================
	Assimp::Importer importer;
	
	glGenVertexArrays(1, &modelShape.vao);
	glBindVertexArray(modelShape.vao);

	int flags = aiProcessPreset_TargetRealtime_MaxQuality;
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

void loadIndirectModel() {
    // load model
    const std::string modelPaths[] = {
        "assets\\grassB",
        "assets\\bush01_lod2",
        "assets\\bush05_lod2",
        "assets\\Medieval_Building_LowPoly\\medieval_building_lowpoly_1",
        "assets\\Medieval_Building_LowPoly\\medieval_building_lowpoly_2",
    };

    Assimp::Importer importer;
    glGenVertexArrays(1, &mergeObject.vao);
    glGenBuffers(1, &mergeObject.vbo_position);
    glGenBuffers(1, &mergeObject.vbo_normal);
    glGenBuffers(1, &mergeObject.vbo_texcoord);
    glGenBuffers(1, &mergeObject.ibo);

    glBindVertexArray(mergeObject.vao);

    const unsigned long long SUM_VERTECIES = 571641;
    glBindBuffer(GL_ARRAY_BUFFER, mergeObject.vbo_position);
    glBufferData(GL_ARRAY_BUFFER, SUM_VERTECIES * 3 * sizeof(float), nullptr, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, mergeObject.vbo_normal);
    glBufferData(GL_ARRAY_BUFFER, SUM_VERTECIES * 3 * sizeof(float), nullptr, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, mergeObject.vbo_texcoord);
    glBufferData(GL_ARRAY_BUFFER, SUM_VERTECIES * 3 * sizeof(float), nullptr, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mergeObject.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, SUM_VERTECIES * sizeof(unsigned int), nullptr, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    int numVertices = 0, numFaces = 0;
    for (int model_id = 0; model_id < 5; model_id++) {
        std::string modelPath = modelPaths[model_id];
        std::string modelFile = modelPath + ".obj";

        const aiScene *scene = importer.ReadFile(modelFile, aiProcessPreset_TargetRealtime_MaxQuality);

        if (scene == nullptr) {
            std::cout << "failed to load model\n";
            std::cout << "model file : " << modelFile << "\n";
            return;
        }

        // Goemetry
        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {

            aiMesh *mesh = scene->mMeshes[i];

            glBindVertexArray(mergeObject.vao);

            // vertex buffer object
            glBindBuffer(GL_ARRAY_BUFFER, mergeObject.vbo_position);
            glBufferSubData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(float), mesh->mNumVertices * sizeof(aiVector3D),
                            mesh->mVertices);

            // normal buffer object
            glBindBuffer(GL_ARRAY_BUFFER, mergeObject.vbo_normal);
            glBufferSubData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(float), mesh->mNumVertices * sizeof(aiVector3D),
                            mesh->mNormals);

            // texcoord buffer object
            glBindBuffer(GL_ARRAY_BUFFER, mergeObject.vbo_texcoord);
            // change the texcoord from 2D to 3D (add model id)
            for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
                mesh->mTextureCoords[0][i].z = float(model_id);
            }
            glBufferSubData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(float), mesh->mNumVertices * sizeof(aiVector3D),
                            mesh->mTextureCoords[0]);

            // index buffer object
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mergeObject.ibo);
            unsigned int *indices = new unsigned int[mesh->mNumFaces * 3];
            for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
                indices[i * 3 + 0] = mesh->mFaces[i].mIndices[0];
                indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
                indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
                //if (i < 100) std::cout << indices[i * 3 + 0] << " " << indices[i * 3 + 1] << " " << indices[i * 3 + 2] << "\n";
            }
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, numFaces * 3 * sizeof(unsigned int),mesh->mNumFaces * 3 * sizeof(unsigned int), indices);
            numVertices += mesh->mNumVertices;
            numFaces += mesh->mNumFaces;

            //std::cout << "num vertices : " << mesh->mNumVertices << "\n";
            //std::cout << "num faces : " << mesh->mNumFaces << "\n";
            delete[] indices;
        }

        //std::cout << "num vertices : " << numVertices << "\n";
        //std::cout << "num faces : " << numFaces << "\n";

        importer.FreeScene();
    }

    glBindVertexArray(0);
}

void loadAllSpatialSample() {
	// load spatial sample
	// ===========================================
    // load samples
    MyPoissonSample* sample[5];
    sample[0] = MyPoissonSample::fromFile("assets\\poissonPoints_621043_after.ppd2");
    sample[1] = MyPoissonSample::fromFile("assets\\poissonPoints_2797.ppd2");
    sample[2] = MyPoissonSample::fromFile("assets\\poissonPoints_1010.ppd2");
    sample[3] = MyPoissonSample::fromFile("assets\\cityLots_sub_0.ppd2");
    sample[4] = MyPoissonSample::fromFile("assets\\cityLots_sub_1.ppd2");
    int numSample = sample[0]->m_numSample + sample[1]->m_numSample + sample[2]->m_numSample + sample[3]->m_numSample + sample[4]->m_numSample;

    // std::cout << "num sample : " << numSample << "\n";
    // std::cout << "suample 0 : " << sample[0]->m_numSample << "\n";
    // std::cout << "suample 1 : " << sample[1]->m_numSample << "\n";
    // std::cout << "suample 2 : " << sample[2]->m_numSample << "\n";
    // std::cout << "suample 3 : " << sample[3]->m_numSample << "\n";
    // std::cout << "suample 4 : " << sample[4]->m_numSample << "\n";

    float *instance = new float[numSample * 20];
    //MyInstance *instance = new MyInstance[numSample];
    int preNumSample = 0;
    for (int c = 0; c < 5; c++) {
        // query position and rotation
        for(int i = 0; i < sample[c]->m_numSample; i++){
            glm::vec4 position(sample[c]->m_positions[i * 3 + 0], sample[c]->m_positions[i * 3 + 1], sample[c]->m_positions[i * 3 + 2], c);
            glm::vec3 rad(sample[c]->m_radians[i * 3 + 0], sample[c]->m_radians[i * 3 + 1], sample[c]->m_radians[i * 3 + 2]);
            // calculate rotation matrix

            glm::quat q = glm::quat(rad);
            // store the rotation matrix in ssbo
            glm::mat4 rotationMatrix = glm::toMat4(q);

            //instance[preNumSample + i].position = position;
            //instance[preNumSample + i].modelMat = rotationMatrix;
            instance[(preNumSample + i) * 20 + 0] = position.x;
            instance[(preNumSample + i) * 20 + 1] = position.y;
            instance[(preNumSample + i) * 20 + 2] = position.z;
            instance[(preNumSample + i) * 20 + 3] = c;
            // store the rotation matrix in ssbo
            for (int r = 0; r < 16; r++) {
                instance[(preNumSample + i) * 20 + 4 + r] = rotationMatrix[r / 4][r % 4];
            }
        }
        preNumSample = preNumSample + sample[c]->m_numSample;
    }

    // SSBO
    glGenBuffers(1, &wholeDataBufferHandle);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, wholeDataBufferHandle);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, numSample * 20 * sizeof(float),instance, GL_MAP_READ_BIT);

    // visible SSBO
    glGenBuffers(1, &visibleDataBufferHandle);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, visibleDataBufferHandle);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, numSample * sizeof(unsigned  int), nullptr, GL_MAP_READ_BIT);

    /*
    0 num vertices : 641
      num faces : 814
    1 num vertices : 999
      num faces : 546
    2 num vertices : 1119
      num faces : 880
    3 num vertices : 858
      num faces : 462
    4 num vertices : 1314
      num faces : 714
     */

    // Draw Command
    DrawCommand drawCommands[5];
    drawCommands[0].count = 814 * 3;
    drawCommands[0].instanceCount = 0;
    drawCommands[0].firstIndex = 0;
    drawCommands[0].baseVertex = 0;
    drawCommands[0].baseInstance = 0;

    drawCommands[1].count = 546 * 3;
    drawCommands[1].instanceCount = 0;
    drawCommands[1].firstIndex = drawCommands[0].count;
    drawCommands[1].baseVertex = 641;
    drawCommands[1].baseInstance = sample[0]->m_numSample;

    drawCommands[2].count = 880 * 3;
    drawCommands[2].instanceCount = 0;
    drawCommands[2].firstIndex = drawCommands[0].count + drawCommands[1].count;
    drawCommands[2].baseVertex = 999 + 641;
    drawCommands[2].baseInstance = sample[0]->m_numSample + sample[1]->m_numSample;

    drawCommands[3].count = 462 * 3;
    drawCommands[3].instanceCount = 0;
    drawCommands[3].firstIndex = drawCommands[0].count + drawCommands[1].count + drawCommands[2].count;
    drawCommands[3].baseVertex = 1119 + 999 + 641;
    drawCommands[3].baseInstance = sample[0]->m_numSample + sample[1]->m_numSample + sample[2]->m_numSample;

    drawCommands[4].count = 714 * 3;
    drawCommands[4].instanceCount = 0;
    drawCommands[4].firstIndex = drawCommands[0].count + drawCommands[1].count + drawCommands[2].count + drawCommands[3].count;
    drawCommands[4].baseVertex = 858 + 1119 + 999 + 641;
    drawCommands[4].baseInstance = sample[0]->m_numSample + sample[1]->m_numSample + sample[2]->m_numSample + sample[3]->m_numSample;

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cmdBufferHandle);

    glGenBuffers(1, &cmdBufferHandle);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cmdBufferHandle);
    glNamedBufferStorage(cmdBufferHandle, sizeof(drawCommands), drawCommands, GL_MAP_READ_BIT);

    // indirect building buffer
    glGenBuffers(1, &indirectBuildingBufferHandle);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuildingBufferHandle);
    glNamedBufferStorage(indirectBuildingBufferHandle, sizeof(drawCommands), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void loadTexture() {

	// load magic rock
	std::string texturePath = "assets\\MagicRock\\StylMagicRocks_AlbedoTransparency.png";
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
    const int NUM_TEXTURE = 5;
    const int IMG_WIDTH = 1024;
    const int IMG_HEIGHT = 1024;
    const int IMG_CHANNEL = 4;
    unsigned char *textureArrayData = new unsigned char[IMG_WIDTH * IMG_HEIGHT * IMG_CHANNEL * NUM_TEXTURE];
    // merge the texture into one array
    std::string texture_paths[NUM_TEXTURE] = {
            "assets\\grassB_albedo.png",
            "assets\\bush01.png",
            "assets\\bush05.png",
            "assets\\Medieval_Building_LowPoly\\Medieval_Building_LowPoly_V1_Albedo_small.png",
            "assets\\Medieval_Building_LowPoly\\Medieval_Building_LowPoly_V2_Albedo_small.png"
    };

    for (int i = 0; i < NUM_TEXTURE; i++) {
        std::string fileName = texture_paths[i];
        int width, height, channel;
        unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &channel, 0);
        if (data == nullptr) {
            std::cout << "failed to load texture\n";
            std::cout << "file name : " << fileName << "\n";
            return;
        }
        memcpy(textureArrayData + i * IMG_WIDTH * IMG_HEIGHT * IMG_CHANNEL, data, IMG_WIDTH * IMG_HEIGHT * IMG_CHANNEL);
        stbi_image_free(data);
    }
    // create texture array
    GLuint textureArray;
    glGenTextures(1, &textureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);

    // the internal format for glTexStorageXD must be "Sized Internal Formats“
    // max mipmap level = log2(1024) + 1 = 11
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 11, GL_RGBA8, IMG_WIDTH, IMG_HEIGHT, NUM_TEXTURE);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, IMG_WIDTH, IMG_HEIGHT, NUM_TEXTURE, GL_RGBA, GL_UNSIGNED_BYTE,
                    textureArrayData);

    // set the texture parameters
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Mipmap
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    // set the texture unit
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);

    // release memory
    delete[] textureArrayData;
}

void compileShaderProgram() {
    auto createShaderProgram = [](std::string vs_path, std::string fs_path, std::string gs_path = "") -> ShaderProgram* {
        ShaderProgram* shaderProgram = new ShaderProgram();
        shaderProgram->init();

        // vertex shader
        Shader* vsShader = new Shader(GL_VERTEX_SHADER);
        vsShader->createShaderFromFile(vs_path);

        // fragment shader
        Shader* fsShader = new Shader(GL_FRAGMENT_SHADER);
        fsShader->createShaderFromFile(fs_path);

        // geometry shader
        Shader* gsShader = nullptr;
        if (gs_path != "") {
            gsShader = new Shader(GL_GEOMETRY_SHADER);
            gsShader->createShaderFromFile(gs_path);
        }

        shaderProgram->attachShader(vsShader);
        shaderProgram->attachShader(fsShader);
        if (gsShader != nullptr) {
            shaderProgram->attachShader(gsShader);
        }
        shaderProgram->checkStatus();
        if (shaderProgram->status() != ShaderProgramStatus::READY) {
            return nullptr;
        }
        shaderProgram->linkProgram();

        vsShader->releaseShader();
        fsShader->releaseShader();
        if (gsShader != nullptr) {
            gsShader->releaseShader();
        }

        return shaderProgram;
    };

	// magic rock
	magicRockShaderProgram = createShaderProgram("src\\shader\\magicRock.vs.glsl", "src\\shader\\magicRock.fs.glsl");

	// airplane
	airplaneShaderProgram = createShaderProgram("src\\shader\\airplane.vs.glsl", "src\\shader\\airplane.fs.glsl");

    // indirect render shader
    indirectRenderShaderProgram = createShaderProgram("src\\shader\\indirectRender.vs.glsl", "src\\shader\\indirectRender.fs.glsl");

    // gbuffer shader
    gbufferShaderProgram = createShaderProgram("src\\shader\\deferred.vs.glsl", "src\\shader\\deferred.fs.glsl");

    // hiz fbo shadder
    hizShaderProgram = createShaderProgram("src\\shader\\flushdepth.vs.glsl", "src\\shader\\flushdepth.fs.glsl");

    // hiz mip map shader
    //hizMipMapShaderProgram = createShaderProgram("src\\shader\\hizMipMap.vs.glsl", "src\\shader\\hizMipMap.fs.glsl", "src\\shader\\hizMipMap.gs.glsl");
    hizMipMapShaderProgram = createShaderProgram("src\\shader\\hizMipMap.vs.glsl", "src\\shader\\hizMipMap.fs.glsl");

    // ====== computer shader =============================
    // culling shader
    cullingShaderProgram = new ShaderProgram();
    cullingShaderProgram->init();

    // compute shader
    Shader *csShader = new Shader(GL_COMPUTE_SHADER);
    csShader->createShaderFromFile("src\\shader\\culling.cs.glsl");

    cullingShaderProgram->attachShader(csShader);
    cullingShaderProgram->checkStatus();
    if (cullingShaderProgram->status() != ShaderProgramStatus::READY) {
        return;
    }
    cullingShaderProgram->linkProgram();

    // clear shader
    clearShaderProgram = new ShaderProgram();
    clearShaderProgram->init();

    // compute shader
    csShader = new Shader(GL_COMPUTE_SHADER);
    csShader->createShaderFromFile("src\\shader\\clear.cs.glsl");

    clearShaderProgram->attachShader(csShader);
    clearShaderProgram->checkStatus();
    if (clearShaderProgram->status() != ShaderProgramStatus::READY) {
        return;
    }
    clearShaderProgram->linkProgram();
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
    glm::vec3 cameraPos = m_myCameraManager->playerViewOrig();

	// pass uniform
	unsigned int programId = magicRockShaderProgram->programId();
	glUniformMatrix4fv(glGetUniformLocation(programId, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
	glUniformMatrix4fv(glGetUniformLocation(programId, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));
    glUniform3fv(glGetUniformLocation(programId, "cameraPos"), 1, glm::value_ptr(cameraPos));
    glUniform1i(glGetUniformLocation(programId, "normalMaping"), normalMaping);

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

void drawBuilding() {
    indirectRenderShaderProgram->useProgram();

    void *cmd_data = malloc(sizeof(DrawCommand) * 5);
    glGetNamedBufferSubData(cmdBufferHandle, 0, sizeof(DrawCommand) * 5, cmd_data);

    // pass first two draw command to indirect render shader
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuildingBufferHandle);
    void *cmd_offset = ((DrawCommand*)cmd_data + 3);
    glNamedBufferSubData(indirectBuildingBufferHandle, 0, sizeof(DrawCommand) * 2, cmd_offset);


    const glm::ivec4 playerViewport = m_myCameraManager->playerViewport();
    const glm::ivec4 godViewport = m_myCameraManager->godViewport();

    GLuint programId = indirectRenderShaderProgram->programId();

    // pass uniform
    glm::mat4 viewMat = m_myCameraManager->playerViewMatrix();
    glm::mat4 projMat = m_myCameraManager->playerProjectionMatrix();
    glm::mat4 viewProjMat = projMat * viewMat;
    glUniformMatrix4fv(glGetUniformLocation(programId, "viewProjMat"), 1, false, glm::value_ptr(viewProjMat));

    // set viewport to player viewport
    defaultRenderer->setViewport(playerViewport[0], playerViewport[1], playerViewport[2], playerViewport[3]);
    glBindVertexArray(mergeObject.vao);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 2, 0);

    // ------------------------------------
    // god viewport

    // pass uniform
    viewMat = m_myCameraManager->godViewMatrix();
    projMat = m_myCameraManager->godProjectionMatrix();
    viewProjMat = projMat * viewMat;
    glUniformMatrix4fv(glGetUniformLocation(programId, "viewProjMat"), 1, false, glm::value_ptr(viewProjMat));

    // set viewport to god viewport
    defaultRenderer->setViewport(godViewport[0], godViewport[1], godViewport[2], godViewport[3]);
    glBindVertexArray(mergeObject.vao);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 2, 0);

    // unbind
    glBindVertexArray(0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void drawFoliage() {
    // shader
    indirectRenderShaderProgram->useProgram();

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cmdBufferHandle);

    const glm::ivec4 playerViewport = m_myCameraManager->playerViewport();
    const glm::ivec4 godViewport = m_myCameraManager->godViewport();

    GLuint programId = indirectRenderShaderProgram->programId();

    // pass uniform
    glm::mat4 viewMat = m_myCameraManager->playerViewMatrix();
    glm::mat4 projMat = m_myCameraManager->playerProjectionMatrix();
    glm::mat4 viewProjMat = projMat * viewMat;
    glUniformMatrix4fv(glGetUniformLocation(programId, "viewProjMat"), 1, false, glm::value_ptr(viewProjMat));

    // set viewport to player viewport
    defaultRenderer->setViewport(playerViewport[0], playerViewport[1], playerViewport[2], playerViewport[3]);
    glBindVertexArray(mergeObject.vao);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 3, 0);

    // ------------------------------------
    // god viewport

    // pass uniform
    viewMat = m_myCameraManager->godViewMatrix();
    projMat = m_myCameraManager->godProjectionMatrix();
    viewProjMat = projMat * viewMat;
    glUniformMatrix4fv(glGetUniformLocation(programId, "viewProjMat"), 1, false, glm::value_ptr(viewProjMat));

    // set viewport to god viewport
    defaultRenderer->setViewport(godViewport[0], godViewport[1], godViewport[2], godViewport[3]);
    glBindVertexArray(mergeObject.vao);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 3, 0);

    // unbind
    glBindVertexArray(0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void drawIndirectRender() {
    // shader
    indirectRenderShaderProgram->useProgram();

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cmdBufferHandle);

    const glm::ivec4 playerViewport = m_myCameraManager->playerViewport();
    const glm::ivec4 godViewport = m_myCameraManager->godViewport();

    GLuint programId = indirectRenderShaderProgram->programId();

    // pass uniform
    glm::mat4 viewMat = m_myCameraManager->playerViewMatrix();
    glm::mat4 projMat = m_myCameraManager->playerProjectionMatrix();
    glUniformMatrix4fv(glGetUniformLocation(programId, "viewMat"), 1, false, glm::value_ptr(viewMat));
    glUniformMatrix4fv(glGetUniformLocation(programId, "projMat"), 1, false, glm::value_ptr(projMat));

    // set viewport to player viewport
    defaultRenderer->setViewport(playerViewport[0], playerViewport[1], playerViewport[2], playerViewport[3]);
    glBindVertexArray(mergeObject.vao);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 5, 0);

    // ------------------------------------
    // god viewport

    // pass uniform
    viewMat = m_myCameraManager->godViewMatrix();
    projMat = m_myCameraManager->godProjectionMatrix();
    glUniformMatrix4fv(glGetUniformLocation(programId, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
    glUniformMatrix4fv(glGetUniformLocation(programId, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));

    // set viewport to god viewport
    defaultRenderer->setViewport(godViewport[0], godViewport[1], godViewport[2], godViewport[3]);
    glBindVertexArray(mergeObject.vao);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 5, 0);

    // unbind
    glBindVertexArray(0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void drawGbuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw gbuffer
    gbufferShaderProgram->useProgram();
    int programId = gbufferShaderProgram->programId();
    glm::vec3 eyePos = m_myCameraManager->playerViewOrig();
    glUniform1i(glGetUniformLocation(programId, "frame_width"), FRAME_WIDTH);
    glUniform1i(glGetUniformLocation(programId, "gbuffer_mode"), gBufferIdx);
    glUniform1i(glGetUniformLocation(programId, "depthLevel"), depthLevel);
    glUniform3fv(glGetUniformLocation(programId, "eye_position"), 1, glm::value_ptr(eyePos));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer.diffuse_map); // diffuse
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);  // normal
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gbuffer.position_map); // coord
    glActiveTexture(GL_TEXTURE3);
    //glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map); // depth
    glBindTexture(GL_TEXTURE_2D, hiz.depth_map); // depth

    // texture
    glUniform1i(glGetUniformLocation(programId,"diffuse_map"), 0);
    glUniform1i(glGetUniformLocation(programId,"normal_map"), 1);
    glUniform1i(glGetUniformLocation(programId,"position_map"), 2);
    glUniform1i(glGetUniformLocation(programId,"depth_map"), 3);

    // set viewport to all
    defaultRenderer->setViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);

    glBindVertexArray(gbuffer.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
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

	// start rendering
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// start new frame
	defaultRenderer->setViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);
	defaultRenderer->startNewFrame();

    //  =======================  bind frame buffer =======================

    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);
    const GLenum gbufferAttachments[] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, gbufferAttachments);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // =============================================

	// rendering with player view
	//defaultRenderer->setViewport(playerViewport[0], playerViewport[1], playerViewport[2], playerViewport[3]);
    defaultRenderer->setViewport(FRAME_WIDTH / 2, 0, FRAME_WIDTH / 2, FRAME_HEIGHT );
	defaultRenderer->setView(playerVM);
	defaultRenderer->setProjection(playerProjMat);
	defaultRenderer->renderPass();

	// rendering with god view
	defaultRenderer->setViewport(godViewport[0], godViewport[1], godViewport[2], godViewport[3]);
	defaultRenderer->setView(godVM);
	defaultRenderer->setProjection(godProjMat);
	defaultRenderer->renderPass();

	// ===============================

    computeDrawCommands();

	drawMagicRock();
	drawAirplane();
    drawBuilding();

    // ========= flush depth buffer to hiz map =========

    flushDepth();
    genHizMap();

    // =============================================

    // rebind gbuffer fbo
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);
    glDrawBuffers(3, gbufferAttachments);
    glEnable(GL_DEPTH_TEST);

    drawFoliage();

    drawGbuffer();

	// =============================================

	ImGui::Begin("My name is window");
	m_imguiPanel->update();
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void flushDepth() {
    // bind hiz fbo
    glBindFramebuffer(GL_FRAMEBUFFER, hiz.fbo);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glDepthFunc(GL_ALWAYS);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    // attach depth map
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, hiz.depth_map, 0);

    // draw hiz
    hizShaderProgram->useProgram();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map); // depth
    // uniform
    glUniform1i(glGetUniformLocation(hizShaderProgram->programId(), "depth_map"), 0);
    glBindVertexArray(hiz.vao);
    // view port to player viewport
    //defaultRenderer->setViewport(playerViewport[0], playerViewport[1], playerViewport[2], playerViewport[3]);

    // view port to all
    defaultRenderer->setViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDepthFunc(GL_LEQUAL);
}

void genHizMap() {
    // bind hiz fbo
    glBindFramebuffer(GL_FRAMEBUFFER, hiz.fbo);

    hizMipMapShaderProgram->useProgram();
    // disable color buffer as we will render only a depth image
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hiz.depth_map);
    // we have to disable depth testing but allow depth writes
    glDepthFunc(GL_ALWAYS);
    // calculate the number of mipmap levels for NPOT texture
    int numLevels = 1 + (int)floorf(log2f(fmaxf(FRAME_WIDTH, FRAME_HEIGHT)));
    int currentWidth = FRAME_WIDTH;
    int currentHeight = FRAME_HEIGHT;
    GLuint programId = hizMipMapShaderProgram->programId();
    glUniform1i(glGetUniformLocation(programId, "LastMip"), 0);
    for (int i=1; i<numLevels; i++) {
        glUniform2i(glGetUniformLocation(programId, "LastMipSize"), currentWidth, currentHeight);
        // calculate next viewport size
        currentWidth /= 2;
        currentHeight /= 2;
        // ensure that the viewport size is always at least 1x1
        currentWidth = currentWidth > 0 ? currentWidth : 1;
        currentHeight = currentHeight > 0 ? currentHeight : 1;
        glViewport(0, 0, currentWidth, currentHeight);
        // bind next level for rendering but first restrict fetches only to previous level
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, i-1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, i-1);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, hiz.depth_map, i);
        // clear
        //glClear(GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(hiz.vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
    }
    // reset mipmap level range for the depth image
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels-1);
    // reenable color buffer writes, reset viewport and reenable depth test
    glDepthFunc(GL_LEQUAL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);
}

void computeDrawCommands() {
    // unbind buffer
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);


    // clear buffer
    clearShaderProgram->useProgram();
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // culling shader
    cullingShaderProgram->useProgram();
    // pass camera position
    glm::vec3 cameraPosition = m_myCameraManager->playerViewOrig();
    glm::vec3 lookCenter = m_myCameraManager->playerCameraLookCenter();
    glm::mat4 viewMat = m_myCameraManager->playerViewMatrix();
    glm::mat4 projMat = m_myCameraManager->playerProjectionMatrix();
    glm::mat4 viewProjMat = projMat * viewMat;
    glm::vec4 playerViewport = m_myCameraManager->playerViewport();

    GLuint programId = cullingShaderProgram->programId();
    // std::cout << cameraPosition.x << " " << cameraPosition.y << " " << cameraPosition.z << "\n";

    // texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hiz.depth_map); // depth
    glUniform1i(glGetUniformLocation(programId, "depth_map"), 0);

    glUniform1i(glGetUniformLocation(programId, "enableHiz"), enableHiz);
    glUniform3fv(glGetUniformLocation(programId, "cameraPos"), 1, glm::value_ptr(cameraPosition));
    glUniformMatrix4fv(glGetUniformLocation(programId, "viewProjMat"), 1, false, glm::value_ptr(viewProjMat));
    glUniform2iv(glGetUniformLocation(programId, "Frame"), 1, glm::value_ptr(glm::ivec2(FRAME_WIDTH, FRAME_HEIGHT)));

    glDispatchCompute(571641, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    return;
    // get draw commands from cmd buffer
    void *cmd_data = malloc(sizeof(DrawCommand) * 5);
    glGetNamedBufferSubData(cmdBufferHandle, 0, sizeof(DrawCommand) * 5, cmd_data);

    // output draw commands to screen
    DrawCommand *drawCommands = (DrawCommand *) cmd_data;
    for (int i = 0; i < 5; i++) {
        std::cout << "draw command " << i << "\n";
        //std::cout << "count : " << drawCommands[i].count << "\n";
        std::cout << "instanceCount : " << drawCommands[i].instanceCount << "\n";
        //std::cout << "firstIndex : " << drawCommands[i].firstIndex << "\n";
        //std::cout << "baseVertex : " << drawCommands[i].baseVertex << "\n";
        std::cout << "baseInstance : " << drawCommands[i].baseInstance << "\n\n";
    }
    free(cmd_data);
}


////////////////////////////////////////////////

void processBtnInput() {
    // teleport
    const int teleportIdx = m_imguiPanel->getTeleportIdx();
    if (teleportIdx != -1) {
        m_myCameraManager->teleport(teleportIdx);
    }

    normalMaping = m_imguiPanel->getNormalMapping();
    gBufferIdx = m_imguiPanel->getGBufferIdx();
    depthLevel = m_imguiPanel->getDepthLevel();
    enableHiz = m_imguiPanel->getHiZ();
}


void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && cursorPos[0] < FRAME_WIDTH / 2.0) {
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

    glViewport(0, 0, w, h);
    m_myCameraManager->resize(w, h);
    defaultRenderer->resize(w, h);
    updateWhenPlayerProjectionChanged(0.1, m_myCameraManager->playerCameraFar());

    // for diffuse
    glBindTexture(GL_TEXTURE_2D, gbuffer.diffuse_map);
    //glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8, w, h);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // for normal
    glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
    //glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA32F, w, h);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // for coordinate
    glBindTexture(GL_TEXTURE_2D, gbuffer.position_map);
    //glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA32F, w, h);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // for depth
    glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map);
    //glTexStorage2D(GL_TEXTURE_2D,1,GL_DEPTH_COMPONENT32F, w, h);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, hiz.depth_map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);
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
