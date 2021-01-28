#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

#define TIMER_START 60.0

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char *path);
unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct MovingObject{
    int kaktus = -1;
    int laptop = -1;
    int lazybag = -1;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    bool gameStart = false;
    double startTime;
    bool diamondColected = false;
    bool dollarCollected = false;
    //glm::vec3 backpackPosition = glm::vec3(0.0f);
    //float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(-2.32,0.54,5.87)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;
MovingObject movingObject;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/6.1.skybox.vs", "resources/shaders/6.1.skybox.fs");
    Shader transpShader("resources/shaders/transparentobj.vs", "resources/shaders/transparentobj.fs");

    // load models
    Model ourModelLazyBag("resources/objects/lazybag/10216_Bean_Bag_Chair_v2_max2008_it2.obj");
    Model ourModelLapTop("resources/objects/laptop/Laptop_High-Polay_HP_BI_2_obj.obj");
    Model ourModelKaktus("resources/objects/kaktus/kwiatek.obj");
    ourModelLazyBag.SetShaderTextureNamePrefix("material.");


    glEnable(GL_DEPTH_TEST);

    // setting coordinates:

    // transparent background square
    float transparentVertices[] = {
            // positions         // texture coordinates (non-swapped because stbi flips picture when loading)
            0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  0.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  0.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  1.0f
    };


    // skybox
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    float slikaVertices[] = {
            // positions          // colors           // texture coords
            0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
            0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
            -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left
    };
    unsigned int indices[] = {
            0,1,3,
            1,2,3
    };

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // dollar stash VAO
    unsigned int transparentDollarVAO, transparentDollarVBO;
    glGenVertexArrays(1, &transparentDollarVAO);
    glGenBuffers(1, &transparentDollarVBO);
    glBindVertexArray(transparentDollarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentDollarVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    //DIAMOND VAO
    unsigned int transparentDiamondVAO, transparentDiamondVBO;
    glGenVertexArrays(1, &transparentDiamondVAO);
    glGenBuffers(1, &transparentDiamondVBO);
    glBindVertexArray(transparentDiamondVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentDiamondVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // SLIKA EBO
    unsigned int slikaVBO, slikaVAO, slikaEBO;
    glGenVertexArrays(1, &slikaVAO);
    glGenBuffers(1, &slikaVBO);
    glGenBuffers(1, &slikaEBO);
    glBindVertexArray(slikaVAO);
    glBindBuffer(GL_ARRAY_BUFFER, slikaVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(slikaVertices), slikaVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, slikaEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    // texture loading
    stbi_set_flip_vertically_on_load(true);

    unsigned int transparentDollarTexture = loadTexture(FileSystem::getPath("resources/textures/dollars.png").c_str());
    unsigned int transparentDiamondTexture = loadTexture(FileSystem::getPath("resources/textures/diamond.png").c_str());
    unsigned int slikaTexture = loadTexture(FileSystem::getPath("resources/textures/tmp_slika.png").c_str());

    stbi_set_flip_vertically_on_load(false);


    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/px.jpg"),
                    FileSystem::getPath("resources/textures/skybox/nx.jpg"),
                    FileSystem::getPath("resources/textures/skybox/py.jpg"),
                    FileSystem::getPath("resources/textures/skybox/ny.jpg"),
                    FileSystem::getPath("resources/textures/skybox/pz.jpg"),
                    FileSystem::getPath("resources/textures/skybox/nz.jpg")
            };

    unsigned int cubemapTexture = loadCubemap(faces);

    transpShader.use();
    transpShader.setInt("texture1", 0);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);


        // render
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // point lights
        PointLight& pointLight = programState->pointLight;
        pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
        pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
        pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
        pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

        pointLight.constant = 0.1f;
        pointLight.linear = 0.03f;
        pointLight.quadratic = 0.032f;

        ourShader.use();
        // pointLight1
        pointLight.position = glm::vec3(7.5f, 1.0f, 6.5f);
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        // pointLight2
        pointLight.position = glm::vec3(5.0f, 0.7f, 16.5f);
        ourShader.setVec3("pointLight1.position", pointLight.position);
        ourShader.setVec3("pointLight1.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight1.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight1.specular", pointLight.specular);
        ourShader.setFloat("pointLight1.constant", pointLight.constant);
        ourShader.setFloat("pointLight1.linear", pointLight.linear);
        ourShader.setFloat("pointLight1.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        //spotlight:
        ourShader.setVec3("spotLight.position", programState->camera.Position);
        ourShader.setVec3("spotLight.direction", programState->camera.Front);
        ourShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight.constant", 0.5f);
        ourShader.setFloat("spotLight.linear", 0.03);
        ourShader.setFloat("spotLight.quadratic", 0.032);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));


        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // rendering loaded models

        //LAZYBAG
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-2.5f,-1.0f,10.5f) + (float)movingObject.lazybag * glm::vec3(0.0f, 0.0f, 0.7f));
        model = glm::rotate(model,glm::radians(50.0f),glm::vec3(1.0,0,0));
        model = glm::rotate(model,glm::radians(80.0f),glm::vec3(0,0,1.0));
        model = glm::rotate(model,glm::radians(150.0f),glm::vec3(0,1.0,0));
        model = glm::scale(model, glm::vec3(0.017f,0.017f,0.017f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        ourModelLazyBag.Draw(ourShader);

        //LAPTOP
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(8.0f, -3.0f, 13.0f) + (float)movingObject.laptop * glm::vec3(0.0f, 0.0f, 2.0f));
        model = glm::rotate(model,glm::radians(30.0f),glm::vec3(0.0,1.0,0.0));
        model = glm::scale(model, glm::vec3(0.5f));
        ourShader.setMat4("model", model);
        ourModelLapTop.Draw(ourShader);

        //KAKTUS
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(6.0f,-5.5f,3.5f) + (float)movingObject.kaktus * glm::vec3(-2.0f, 0.0f, 0.0f));
        model = glm::rotate(model,glm::radians(-90.0f),glm::vec3(1.0,0,0.0));
        model = glm::scale(model, glm::vec3(0.065f,0.065f,0.065f));
        ourShader.setMat4("model", model);
        ourModelKaktus.Draw(ourShader);

        // transparent objects
        // DOLLAR object
        if(!programState->dollarCollected){
            transpShader.use();
            projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                          (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
            view = programState->camera.GetViewMatrix();
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-3.5f, -7.0f, 25.0f));
            model = glm::scale(model, glm::vec3(2.5f, 2.5f, 2.5f));

            transpShader.setMat4("model", model);
            transpShader.setMat4("projection", projection);
            transpShader.setMat4("view", view);

            glBindVertexArray(transparentDollarVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, transparentDollarTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //DIAMOND object
        if(!programState->diamondColected){
            transpShader.use();
            projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                          (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
            view = programState->camera.GetViewMatrix();
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(10.0f, -5.0f, 3.5f));
            model = glm::rotate(model, glm::radians(98.0f), glm::vec3(0.0, 1.0, 0.0));

            transpShader.setMat4("model", model);
            transpShader.setMat4("projection", projection);
            transpShader.setMat4("view", view);

            glBindVertexArray(transparentDiamondVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, transparentDiamondTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // picture
        glBindTexture(GL_TEXTURE_2D, slikaTexture);
        transpShader.use();
        projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(7.5f, 2.0f, 4.5f));
        model = glm::rotate(model,glm::radians(90.0f),glm::vec3(0.0,1.0,0.0));
        model = glm::scale(model,glm::vec3(1.5f));

        transpShader.setMat4("model", model);
        transpShader.setMat4("projection", projection);
        transpShader.setMat4("view", view);
        glBindVertexArray(slikaVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // drawing skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.

    glDeleteVertexArrays(1, &slikaVAO);
    glDeleteBuffers(1, &slikaVBO);
    glDeleteBuffers(1, &slikaEBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    /// ovaj deo nece biti dostupan za vreme igre, samo kao pomoc u izradi
    /*
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    */

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        programState->camera.Position = glm::vec3(-2.32,0.54,5.87);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        double time = TIMER_START;
        if(programState->gameStart && !(programState->diamondColected && programState->dollarCollected)) {
            time = max(TIMER_START - glfwGetTime() + programState->startTime, 0.0);
        }
        ImGui::Begin("Money Heist");
        ImGui::Text("timer: %f sec", time);

        /// Ovaj deo nam treba za implementaciju pomeranja objekata, nije dostupan tokom igre
//
//        const Camera& c = programState->camera;
//        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
//        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
//        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);


        // GUI:
        if(!programState->gameStart){
            ImGui::Text("Nalazis se u hotelskoj sobi koju zelis da opljackas,\nali ono sto nisi znao je da je ona obezbdjena alarmom\n"
                        "Imas tacno 60 sekundi da pronadjes ono zbog cega si ovde\n\n\n"
                        "Da zapocnes igru pritisni S\n");
        }
        else if(time == 0){
            programState->CameraMouseMovementUpdateEnabled = false;
            ImGui::Text("Kraj igre, isteklo vreme\n\n\n"
                        "Za izlaz pritisni ESC\n");
        } else{
            ImGui::Text("Klikni SPACE da pomeris osvetljeni objekat,\n"
                        "da sakupis blago klikni ENTER \ni pozuri, tajmer vec odbrojava\n");
            if(programState->diamondColected && !programState->dollarCollected){
                ImGui::Text("Bravo, pronasao si dijamant\n");
            }
            if(programState->dollarCollected && !programState->diamondColected) {
                ImGui::Text("Bravo, pronasao si dolare\n");
            }
            if(programState->diamondColected && programState->dollarCollected){
                ImGui::Text("Misija uspesna!\n Sve si pronasao na vreme\n\nKlikni ESC za izlaz");
            }
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        if (!programState->gameStart) {
            programState->startTime = glfwGetTime();
        }
        programState->gameStart = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS && programState->gameStart) {
        double xpos = programState->camera.Front.x;
        double ypos = programState->camera.Front.y;
        double zpos = programState->camera.Front.z;

        // kaktus
        if ((xpos - 0.93) * (xpos - 0.93) + (ypos + 0.33) * (ypos + 0.33) + (zpos + 0.13) * (zpos + 0.13) < 0.1 && movingObject.kaktus == -1)
            movingObject.kaktus = 1;
        else if (movingObject.kaktus == 1)
            movingObject.kaktus = -1;
        // lazybag
        if ((xpos + 0.06) * (xpos + 0.06) + (ypos + 0.21) * (ypos + 0.21) + (zpos - 0.97) * (zpos - 0.97) < 0.1 && movingObject.lazybag == -1)
            movingObject.lazybag = 1;
        else if (movingObject.lazybag == 1)
            movingObject.lazybag = -1;
        // laptop
        if ((xpos - 0.88) * (xpos - 0.88) + (ypos + 0.20) * (ypos + 0.20) + (zpos - 0.41) * (zpos - 0.41) < 0.1 && movingObject.laptop == -1)
            movingObject.laptop = 1;
        else if (movingObject.laptop == 1)
            movingObject.laptop = -1;
    }

    if(key == GLFW_KEY_ENTER && action == GLFW_PRESS){
        if(movingObject.kaktus == 1){
            programState->diamondColected = true;
        }
        else if(movingObject.lazybag == 1){
            programState->dollarCollected = true;
        }

        if(programState->diamondColected && programState->dollarCollected){
            programState->CameraMouseMovementUpdateEnabled = false;
        }
    }
}


unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


