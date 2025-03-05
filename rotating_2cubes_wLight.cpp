#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Vertex shader source code
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    out vec3 FragPos;
    out vec3 Normal;
    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
    }
    )";

// Fragment shader source code
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec3 FragPos;
    in vec3 Normal;
    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    void main()
    {
        // Ambient
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColor;

        // Diffuse 
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        // Result
        vec3 result = (ambient + diffuse) * objectColor;
        FragColor = vec4(result, 1.0);
    }
    )";

// Function to compile and link shaders
unsigned int compileShader(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set GLFW window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "3D Cubes", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    
    // Load OpenGL functions using GLAD
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
}

// Define the vertices and normals of a cube
float cubeVertices[] = {
    // Positions          // Normals
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // Back face
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  // Front face
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  // Left face
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,


0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // Right face
0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

// Bottom face
-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

// Top face
-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f
};

// Create a single VBO for both cubes
unsigned int VBO;
glGenBuffers(1, &VBO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

// Create VAOs for both cubes
unsigned int VAO1, VAO2;
glGenVertexArrays(1, &VAO1);
glGenVertexArrays(1, &VAO2);

// Set up VAO1 for the first cube
glBindVertexArray(VAO1);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // Position attribute
glEnableVertexAttribArray(0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // Normal attribute
glEnableVertexAttribArray(1);

// Set up VAO2 for the second cube
glBindVertexArray(VAO2);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // Position attribute
glEnableVertexAttribArray(0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // Normal attribute
glEnableVertexAttribArray(1);

// Unbind the VAOs and VBO
glBindBuffer(GL_ARRAY_BUFFER, 0);
glBindVertexArray(0);

// Compile and link shaders
unsigned int shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);

// Set up the camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

glEnable(GL_DEPTH_TEST);

// Main loop
while (!glfwWindowShouldClose(window)) {
    // Input handling
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Rendering
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use the shader program
    glUseProgram(shaderProgram);

    // Create the view and projection matrices
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // Get the location of the model, view, and projection matrices and color uniform
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    int lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");

    // Pass the view and projection matrices to the shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Set the light properties
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos = glm::vec3(1.0f, 1.0f, 2.0f);
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));

    // Create rotation matrix for the first cube (yellow)
    float currentTime = glfwGetTime();
    float angle1 = currentTime * glm::radians(50.0f); // Rotate 50 degrees per second
    glm::mat4 model1 = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) 
                     * glm::rotate(glm::mat4(1.0f), angle1, glm::vec3(1.0f, 1.0f, 1.0f));

    // Pass the rotation matrix and color to the shader for the first cube
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model1));
glm::vec3 objectColor1 = glm::vec3(1.0f, 1.0f, 0.2f); // Yellow color
glUniform3fv(colorLoc, 1, glm::value_ptr(objectColor1));

// Bind the VAO for the first cube and draw it
glBindVertexArray(VAO1);
glDrawArrays(GL_TRIANGLES, 0, 36);

// Create rotation matrix for the second cube (green)
float angle2 = -currentTime * glm::radians(50.0f); // Rotate -50 degrees per second
glm::mat4 model2 = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)) 
                 * glm::rotate(glm::mat4(1.0f), angle2, glm::vec3(1.0f, 1.0f, 1.0f));

// Pass the rotation matrix and color to the shader for the second cube
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model2));
glm::vec3 objectColor2 = glm::vec3(0.0f, 1.0f, 0.0f); // Green color
glUniform3fv(colorLoc, 1, glm::value_ptr(objectColor2));

// Bind the VAO for the second cube and draw it
glBindVertexArray(VAO2);
glDrawArrays(GL_TRIANGLES, 0, 36);

// Swap buffers and poll events
glfwSwapBuffers(window);
glfwPollEvents();
}

// Clean up
glDeleteVertexArrays(1, &VAO1);
glDeleteVertexArrays(1, &VAO2);
glDeleteBuffers(1, &VBO);
glDeleteProgram(shaderProgram);

// Terminate GLFW
glfwTerminate();
return 0;
}
