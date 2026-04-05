// loads .obj, .mtl and its related texture file (need to be in the same folder)
// specify .obj at compile
// compile code with UCRT64 env.:
// g++ -o demo.exe demo.cpp glad.c -I/ucrt64/include -L/ucrt64/lib -lglfw3 -lOpenGL32 -lglu32 -lfreeglut -lglew32 -l:libassimp.dll.a -lgdi32 && demo models/model.obj

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Material
{
    glm::vec3 Ka, Kd, Ks;
    float Ns, d;
    Material() : Ka(0.1f), Kd(0.1f), Ks(0.1f), Ns(32.0f), d(1.0f) {}
};

const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord; // Add this

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 FragPos;
out vec3 Normal;
out vec3 ViewPos;
out vec2 TexCoord;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    ViewPos = vec3(uView * vec4(FragPos, 1.0)); // Pass view-space position
    TexCoord = aTexCoord; // Pass texture coordinates
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}   
)";

const char *fragmentShaderSource = R"(
#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 ViewPos;

uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float Ns;
uniform float d;
uniform sampler2D diffuseMap;
in vec2 TexCoord;

out vec4 FragColor;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = Ka;
    vec3 diffuse = uLightColor * Kd * texture(diffuseMap, TexCoord).rgb * diff;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);
    vec3 specular = uLightColor * Ks * spec;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, d);
}   
)";

struct Mesh
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;
    Material material;

    Mesh(const std::vector<glm::vec3> &verts,
         const std::vector<glm::vec3> &norms,
         const std::vector<glm::vec2> &uvs,
         const std::vector<unsigned int> &inds,
         const Material &mat)
        : vertices(verts), normals(norms), texCoords(uvs), indices(inds), material(mat)
    {

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(glm::vec3) +
                         normals.size() * sizeof(glm::vec3) +
                         texCoords.size() * sizeof(glm::vec2),
                     nullptr, GL_STATIC_DRAW);

        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec3), vertices.data());
        glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), normals.size() * sizeof(glm::vec3), normals.data());
        glBufferSubData(GL_ARRAY_BUFFER,
                        vertices.size() * sizeof(glm::vec3) + normals.size() * sizeof(glm::vec3),
                        texCoords.size() * sizeof(glm::vec2), texCoords.data());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)(vertices.size() * sizeof(glm::vec3)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)(vertices.size() * sizeof(glm::vec3) + normals.size() * sizeof(glm::vec3)));

        glBindVertexArray(0);
    }

    void draw(GLuint shaderProgram)
    {
        glBindVertexArray(VAO);
        glUniform3f(glGetUniformLocation(shaderProgram, "Ka"), material.Ka.x, material.Ka.y, material.Ka.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "Kd"), material.Kd.x, material.Kd.y, material.Kd.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "Ks"), material.Ks.x, material.Ks.y, material.Ks.z);
        glUniform1f(glGetUniformLocation(shaderProgram, "Ns"), material.Ns);
        glUniform1f(glGetUniformLocation(shaderProgram, "d"), material.d);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

class Model
{
public:
    std::vector<Mesh> meshes;

    Model(const std::string &path)
    {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
        processNode(scene->mRootNode, scene);
    }

private:
    void processNode(aiNode *node, const aiScene *scene)
    {
        std::cout << "Processing node: " << node->mName.C_Str() << std::endl;

        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            glm::vec3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            vertices.push_back(pos);

            glm::vec3 normal(0.0f, 0.0f, 0.0f);
            if (mesh->HasNormals())
            {
                normal.x = mesh->mNormals[i].x;
                normal.y = mesh->mNormals[i].y;
                normal.z = mesh->mNormals[i].z;
            }
            normals.push_back(normal);

            glm::vec2 uv(0.0f, 0.0f);
            if (mesh->mTextureCoords[0])
            {
                uv.x = mesh->mTextureCoords[0][i].x;
                uv.y = mesh->mTextureCoords[0][i].y;
            }
            texCoords.push_back(uv);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        // Load material color
        // Load material
        // Load material
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        Material mat;

        aiColor3D ambient(0.1f, 0.1f, 0.1f);
        aiColor3D specular(0.0f, 1.0f, 0.0f);
        aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f); // Default white, fully opaque
        float shininess = 2.0f;
        float opacity = 0.0f;

        // Only get ambient, specular, and shininess with aiColor3D/float
        material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
        material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        material->Get(AI_MATKEY_SHININESS, shininess);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
        material->Get(AI_MATKEY_OPACITY, opacity);

        mat.Ka = glm::vec3(ambient.r, ambient.g, ambient.b);
        mat.Kd = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b); // Use diffuseColor
        mat.Ks = glm::vec3(specular.r, specular.g, specular.b);
        mat.Ns = shininess;
        mat.d = opacity;

        return Mesh(vertices, normals, texCoords, indices, mat);
    }
};

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <model.obj>\n";
        return -1;
    }
    std::string modelPath = argv[1];
    std::string mtlPath = modelPath.substr(0, modelPath.find_last_of(".") + 1) + "mtl";

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "Lit Model Viewer", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    else
    {
        std::cout << "failed to load texture" << std::endl;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glUniform1i(glGetUniformLocation(shaderProgram, "diffuseMap"), 0);

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program link error: " << infoLog << std::endl;
        return -1;
    }

    Model model(modelPath);

    if (model.meshes.empty())
    {
        std::cerr << "No meshes loaded. Check file path and format.\n";
        return -1;
    }

    glm::vec3 viewPos(0.0f, 0.0f, 35.0f);
    glm::mat4 view = glm::lookAt(viewPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(10.0f), 800.0f / 600.0f, 0.1f, 200.0f);

    glm::vec3 lightPosWorld(50.0f, 0.0f, 50.0f);
    glm::vec3 lightPosView = glm::vec3(view * glm::vec4(lightPosWorld, 1.0f));
    glm::vec3 lightDir = glm::vec3(0.0f, 0.0f, -10.0f);

    // Parse the .mtl file for the texture name
    std::ifstream mtlFile(mtlPath);
    std::string line, textureFileName;

    GLuint textureID;

    while (std::getline(mtlFile, line))
    {

        std::istringstream iss(line);
        std::string prefix;
        if (iss >> prefix && prefix == "map_Kd")
        {

            std::getline(iss >> std::ws, textureFileName); // Get the texture filename
            break;
        }
    }

    std::string modelDir = modelPath.substr(0, modelPath.find_last_of("/") + 1);
    std::string fullTexturePath = modelDir + textureFileName;

    // Load the texture
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;

    unsigned char *data = stbi_load(fullTexturePath.c_str(), &width, &height, &channels, 0);
    if (data)
    {
        std::cout << "Loaded image: " << fullTexturePath << ", " << width << "x" << height << ", channels: " << channels << std::endl;
    }
    if (!data)
    {

        std::cerr << "Failed to load texture: " << fullTexturePath << std::endl;
        std::cerr << "Generating white default texture " << std::endl;
        GLuint defaultTexture;
        glGenTextures(1, &defaultTexture);
        glBindTexture(GL_TEXTURE_2D, defaultTexture);
        unsigned char whitePixel[4] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Generate and bind the OpenGL texture

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    stbi_image_free(data);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set uniform after linking and using program
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "diffuseMap"), 0);

        float angle = static_cast<float>(glfwGetTime()) * 45.0f;
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(angle), glm::vec3(0.1f, 0.1f, 0.1f));

        glm::vec3 lightPosView = glm::vec3(view * glm::vec4(lightPosWorld, -10.0f));
        glUniform3fv(glGetUniformLocation(shaderProgram, "uLightPos"), 1, glm::value_ptr(lightPosView));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "uViewPos"), 1, glm::value_ptr(viewPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "uLightDir"), 1, glm::value_ptr(lightDir));
        glUniform3f(glGetUniformLocation(shaderProgram, "uLightColor"), 2.0f, 2.0f, 2.0f);

        for (auto &mesh : model.meshes)
        {

            glUseProgram(shaderProgram);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "diffuseMap"), 0); // Use 0 to match GL_TEXTURE0

            mesh.draw(shaderProgram); // Pass shaderProgram so material uniforms are sent
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
