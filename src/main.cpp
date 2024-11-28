#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Debugger.h>
#include <VertexBuffer.h>
#include <VertexBufferLayout.h>
#include <IndexBuffer.h>
#include <VertexArray.h>
#include <Shader.h>
#include <Texture.h>
#include <Camera.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <cmath>
using namespace std;


/* Window size */
const unsigned int Width = 512;
const unsigned int Height = 512;
// const float FOVdegree = 45.0f;  // Field Of View Angle
const float near = 0.1f;
const float far = 100.0f;

/* Shape vertices coordinates with positions, colors, and corrected texCoords */
float vertices[] = {
    // positions            // colors            // texCoords
    -0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,  // Bottom-left
     0.5f, -0.5f,  0.5f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f,  // Bottom-right
     0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f,  // Top-right
    -0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 0.0f,    0.0f, 1.0f,  // Top-left
};

/* Indices for vertices order */
unsigned int indices[] = {
    0, 1, 2, 
    2, 3, 0
};


// defines
#define PI 3.1415926
extern const float ker[3][3] = {
        {1.0 / 16, 2.0 / 16, 1.0 / 16},
        {2.0 / 16, 4.0 / 16, 2.0 / 16},
        {1.0 / 16, 2.0 / 16, 1.0 / 16}};

extern  const int GaussX[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}};

extern const int GaussY[3][3] = {
        {-1, -2, -1},
        {0, 0, 0},
        {1, 2, 1}};




void copy_image(unsigned char* image, vector<unsigned char> new_image, int length){
    for (int i = 0; i < length; i++)
    {
        image[i] = new_image[i];
    }
}


void noise(unsigned char *image, int width, int height, int length){

    vector<unsigned char> new_image(length);

    float pixel_value;
    for (int i = 1; i < height - 1; i++){
        for (int j = 1; j < width - 1; j++){

            pixel_value = (ker[0][0] * image[(i - 1) * width + (j - 1)]) +
                    (ker[0][1] * image[(i - 1) * width + j]) +
                    (ker[0][2] * image[(i - 1) * width + (j + 1)]) +
                    (ker[1][0] * image[i * width + (j - 1)]) +
                    (ker[1][1] * image[i * width + j]) +
                    (ker[1][2] * image[i * width + (j + 1)]) +
                    (ker[2][0] * image[(i + 1) * width + (j - 1)]) +
                    (ker[2][1] * image[(i + 1) * width + j]) +
                    (ker[2][2] * image[(i + 1) * width + (j + 1)]);

            new_image[i * width + j] = (unsigned char)(pixel_value); // update new image to noise (blurred value)
            pixel_value = 0; // reset
        }
    }
    for (int i = 0; i < height; i++){// right edge
        new_image[i * width + (width - 1)] = image[i * width + (width - 1)];
    }
    for (int i = 0; i < width; i++){ // beneath edge
        new_image[(height - 1) * width + i] = image[(height - 1) * width + i];
    }
    for (int i = 0; i < width; i++){ // Up edge
        new_image[i] = image[i];
    }

    for (int i = 0; i < height; i++) {// left edge
        new_image[i * width] = image[i * width];
    }


    // copy new image
    copy_image(image, new_image, length);
}


vector<float> gradientCalculation(unsigned char *image, int width, int height, int length)
{
    vector<unsigned char> new_image(length);
    vector<float> angles_vector(length);

    int gradX = 0, gradY = 0;

    for (int i = 1; i < height - 1; i++){
        for (int j = 1; j < width - 1; j++){
            for (int k = 0; k < 3; k++){
                for (int p = 0; p < 3; p++){
                    // look for the valued pixel in the given image
                    int pixel = image[(i + k - 1) * width + (j + p - 1)];
                    gradX += GaussX[k][p] * pixel;
                    gradY += GaussY[k][p] * pixel;
                }
            }

            int gradient = (sqrt(gradX * gradX + gradY * gradY));
            new_image[i * width + j] = min(255, gradient); // correct the value if needed (0-255)

            float angle = atan2((float)gradY, (float)gradX);
            angles_vector[i * width + j] = angle;

            //reset
            gradX = 0;
            gradY = 0;
        }
    }

    // copy new image
    copy_image(image, new_image, length);

    return angles_vector;
}

void Non_MaxSuppression(unsigned char *image, int width, int height, int length, vector<float> angles){
    vector<unsigned char> new_image(length);
    float angle;
    unsigned char pixel;
    unsigned char neighbor1, neighbor2;

    for (int i = 1; i < height - 1; i++){
        for (int j = 1; j < width - 1; j++){
            angle = angles[i * width + j];
            pixel = image[i * width + j];

            if (angle < 0){ // [-PI, PI]
                angle += PI;
            }

            if ((0 <= angle && angle <= (PI / 8)) || ((7 * PI / 8) < angle && angle <= PI)){ //(i,j+1),(i,j-1) 0 <= angle < 22.5 || 157.5 < angle <= 180      
                neighbor1 = image[i * width + j + 1];
                neighbor2 = image[i * width + j - 1];
            }

            else if ((3 * PI / 8) < angle && angle <= (5 * PI / 8)){ //(i+1,j),(i-1,j) 67.5 < angle <= 112.5
            
                neighbor1 = image[(i + 1) * width + j];
                neighbor2 = image[(i - 1) * width + j];
            }

            else if ((5 * PI / 8) < angle && angle <= (7 * PI / 8)){ //(i+1,j-1),(i-1,j+1) 112.5 < angle <= 157.5
                neighbor1 = image[(i + 1) * width + j - 1];
                neighbor2 = image[(i - 1) * width + j + -1];
            }

            else if ((PI / 8) < angle && angle <= (3 * PI / 8)){//(i+1,j+1),(i-1,j-1) 22.5 < angle <= 67.5
            
                neighbor1 = image[(i + 1) * width + j + 1];
                neighbor2 = image[(i - 1) * width + j - 1];
            }



            if (neighbor1 <= pixel && neighbor2 <= pixel){
                new_image[i * width + j] = pixel; // else temp =0
            }
        }
    }
    
    // copy new image
    copy_image(image, new_image, length);
}

unsigned char findArea(unsigned char pixel_value){
    int val= std::sqrt(255 * 255 *1.75);
    int low = val *0.5;
    int high =val *0.3;
    if (pixel_value <= low){ // non-relevant edge
        return 0;
    }
    if (pixel_value <= high && pixel_value > low){ // weak edge
        return 1;
    }
    return 255;// the rest (strong edge)
}

void Thresholding(unsigned char *img, int width, int height){
    unsigned char pixel_value;
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            pixel_value = img[i * width + j];
            img[i * width + j] = findArea(pixel_value);
        }
    }
}

void Hysteresis(unsigned char *image, int width, int height, int length){
    unsigned char pixel_value;
    bool check_weak = false;    //efficient
    vector<unsigned char> new_image(length);

    for (int i = 1; i < height - 1; i++){
        for (int j = 1; j < width - 1; j++){
            pixel_value = image[i * width + j];
            if (pixel_value == 1){ // weak edge
                for (int n = 0; (!check_weak) & (n < 3); n++){
                    for (int m = 0; (!check_weak) & (m < 3); m++){

                        if (image[(i + n - 1) * width + (j + m - 1)] == 255){
                            new_image[i * width + j] = 255;
                            check_weak = true; // break loop
                        }
                    }
                }
            }
            else if (pixel_value == 255) {  // strong edge
                new_image[i * width + j] = 255;
            }

            check_weak = false; //reset
        }
    }
    // copy new image
    copy_image(image, new_image, length);
}


void update_pixel(unsigned char* image,int width, int col,int row ,int a,int b,int c,int d){
            image[row * 2 * 2 * width + col * 2] = a;
            image[row * 2 * 2 * width + col * 2 + 1] = b;
            image[(row * 2 + 1) * 2 * width + col * 2] = c;
            image[(row * 2 + 1) * 2 * width + col * 2 + 1] = d;
}

//  Halftone
unsigned char * haftone(unsigned char * image, int width, int height) {
    int length = width * height;
    unsigned char * new_image = new unsigned char[length * 4];
    for (int i = 0; i < length; i++) {
        int row = i / width;
        int col = i % width;
        if (image[i] >= 255.0 / 5 * 4) 
            { update_pixel(new_image,width,col,row,255,255,255,255); }
        else if (image[i] >= 255.0 / 5 && image[i] < 255.0 / 5 * 2 ) 
            { update_pixel(new_image,width,col,row,0,0,255,0);    } 
        else if (image[i] >= 255.0 / 5 * 2 && image[i] < 255.0 / 5 * 3) 
            { update_pixel(new_image,width,col,row,255,0,255,0); } 
        else if (image[i] >= 255.0 / 5 * 3 && image[i] < 255.0 / 5 * 4) 
            { update_pixel(new_image,width,col,row,0,255,255,255); } 
        else if (image[i] < 255.0 / 5) 
            { update_pixel(new_image,width,col,row,0,0,0,0); }
    }
    return new_image;
}

//compressing image to smaller size
void compressImage(const unsigned char* old_Image, unsigned char* new_Image) {
    int old_Width=512,old_Height=512;
    int new_Width=256,new_Height=256;

    int X = old_Width / new_Width;
    int Y = old_Height / new_Height;

    for (int y = 0; y < new_Height; ++y) {
        for (int x = 0; x < new_Width; ++x) {
            int sum = 0;
            for (int ky = 0; ky < Y; ++ky) {
                for (int kx = 0; kx < X; ++kx) {
                    int originalX = x * X + kx;
                    int originalY = y * Y + ky;
                    sum += old_Image[originalY * old_Width + originalX];
                }
            }
            new_Image[y * new_Width + x] = static_cast<unsigned char>(sum / (X * Y));
        }
    }
}



unsigned char* floydSteinbergTo16Grayscale(const unsigned char* image, int width, int height) {
    const float right_pixel = 7 / 16.0f;
    const float left_bottom_pixel = 3 / 16.0f;
    const float bottom_pixel = 5 / 16.0f;
    const float right_bottom_pixel = 1 / 16.0f;
    const int num_levels = 16;
    const float level_size = 255.0f / (num_levels - 1); // Step size for quantization

    int length = width * height;
    float* floyd_diff = new float[length];
    unsigned char* output_image = new unsigned char[length]; 
    //init
    for (int i = 0; i < length; ++i) {
        floyd_diff[i] = static_cast<float>(image[i]);
    }
    // Process each pixel in the input image
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            unsigned char quantized = static_cast<unsigned char>(
            round(floyd_diff[idx] / level_size) * level_size);
            output_image[idx] = quantized;

            float error = floyd_diff[idx] - quantized;

            if (x + 1 < width) { // Right neighbor
                floyd_diff[idx + 1] += error * right_pixel;
            }
            if (y + 1 < height) {
                if (x - 1 >= 0) { // Bottom-left neighbor
                    floyd_diff[(y + 1) * width + (x - 1)] += error * left_bottom_pixel;
                }
                // Bottom neighbor
                floyd_diff[(y + 1) * width + x] += error * bottom_pixel;
                if (x + 1 < width) { // Bottom-right neighbor
                    floyd_diff[(y + 1) * width + (x + 1)] += error * right_bottom_pixel;
                }
            }
        }
    }
    delete[] floyd_diff;
    return output_image;
}




unsigned char * Grayscale(unsigned char * image, int length) {
    unsigned char * new_image = new unsigned char[length];
    for (int i = 0; i < length; i++) {
        new_image[i] = image[i * 4] * (0.2989) + image[i * 4 + 1] * (0.5870) + image[i * 4 + 2] * (0.1140);
    }
    return new_image;
}



int main(int argc, char* argv[]){
    //input image
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps, req_comps = 4;

    // Grayscale
    unsigned char *buffer_gray = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);
    unsigned char *result_buffer_gray = Grayscale(buffer_gray, width * height);
    int result = stbi_write_png("res/textures/Grayscale.png", width, height, 1, result_buffer_gray, width * 1); // changed comps
    std::cout << "grayscale is out:" << std::ends;
    std::cout <<  result << std::endl;


    // Canny
    std::string fp_gray = "res/textures/Grayscale.png";
    unsigned char *buffer_canny = stbi_load(fp_gray.c_str(), &width, &height, &comps, 1);
    noise(buffer_canny, width, height, width * height);
    vector<float> angles = gradientCalculation(buffer_canny, width, height, height * width);
    Non_MaxSuppression(buffer_canny, width, height, width * height, angles);
    Thresholding(buffer_canny, width, height);
    Hysteresis(buffer_canny, width, height, width * height);
    result = stbi_write_png("res/textures/Canny.png", width, height, 1, buffer_canny, width * comps);
    std::cout << "Canny is out:" << std::ends;
    std::cout <<  result << std::endl;

    // Haftone
    unsigned char* buffer_ld = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);
    unsigned char* result_buffer_haftone = haftone(result_buffer_gray, width, height);
    compressImage(result_buffer_haftone,buffer_ld);
    result = stbi_write_png("res/textures/Haftone.png", width , height , 1, buffer_ld, width );
    std::cout << "Haftone is out:" << std::ends;
    std::cout <<  result << std::endl;


    // Floyed
    unsigned char * buffer_floyed = floydSteinbergTo16Grayscale(result_buffer_gray, width, height);
    result = stbi_write_png("res/textures/FloyedSteinberg.png", width, height, 1, buffer_floyed, width);
    std::cout << "Floyed is out:" << std::ends;
    std::cout <<  result << std::endl;



    //display on screen

 GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
    {
        return -1;
    }
    
    /* Set OpenGL to Version 3.3.0 */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(Width, Height, "OpenGL", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Load GLAD so it configures OpenGL */
    gladLoadGL();

    /* Control frame rate */
    glfwSwapInterval(1);

    /* Print OpenGL version after completing initialization */
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    /* Set scope so that on widow close the destructors will be called automatically */
    {
        /* Blend to fix images with transperancy */
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        /* Generate VAO, VBO, EBO and bind them */
        VertexArray va;
        VertexBuffer vb(vertices, sizeof(vertices));
        IndexBuffer ib(indices, sizeof(indices));

        VertexBufferLayout layout;
        layout.Push<float>(3);  // positions
        layout.Push<float>(3);  // colors
        layout.Push<float>(2);  // texCoords
        va.AddBuffer(vb, layout);

        /* Create texture */
        Texture texture("res/textures/Grayscale.png");
        texture.Bind(0);
        Texture texture2("res/textures/Canny.png");
        texture2.Bind(1);
        Texture texture3("res/textures/Haftone.png");
        texture3.Bind(2);
        Texture texture4("res/textures/FloyedSteinberg.png");
        texture4.Bind(3);
        /* Create shaders */
        Shader shader("res/shaders/basic.shader");
        shader.Bind();

        /* Unbind all to prevent accidentally modifying them */
        va.Unbind();
        vb.Unbind();
        ib.Unbind();
        shader.Unbind();

        /* Enables the Depth Buffer */
    	GLCall(glEnable(GL_DEPTH_TEST));

        /* Create camera */
        Camera camera(Width, Height);
        camera.SetOrthographic(near, far);
        camera.EnableInputs(window);

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            /* Set white background color */
            GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));

            /* Render here */
            GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

            /* Initialize uniform color */
            glm::vec4 color = glm::vec4(1.0, 1.0f, 1.0f, 1.0f);


            //pic 1
             /* Initialize the model Translate, Rotate and Scale matrices */
            glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.5f, -1.0f));
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(1.0f));
            glm::mat4 scl = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));

            /* Initialize the MVP matrices */ 
            glm::mat4 model = trans * rot * scl;
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 proj = camera.GetProjectionMatrix();
            glm::mat4 mvp = proj * view * model;

            /* Update shaders paramters and draw to the screen */
            shader.Bind();
            shader.SetUniform4f("u_Color", color);
            shader.SetUniformMat4f("u_MVP", mvp);
            texture.Bind(0);  // Bind first texture to texture unit 0
            shader.SetUniform1i("u_Texture", 0);  // Tell the shader to use texture unit 0
            va.Bind();
            ib.Bind();
            GLCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));

            //pic 2

             /* Initialize the model Translate, Rotate and Scale matrices */
             trans = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, -1.0f));
             rot = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(1.0f));
             scl = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));

            /* Initialize the MVP matrices */ 
             model = trans * rot * scl;
             view = camera.GetViewMatrix();
             proj = camera.GetProjectionMatrix();
             mvp = proj * view * model;

            /* Update shaders paramters and draw to the screen */
            shader.Bind();
            shader.SetUniform4f("u_Color", color);
            shader.SetUniformMat4f("u_MVP", mvp);
            texture2.Bind(0);  // Bind first texture to texture unit 0
            shader.SetUniform1i("u_Texture", 0);
            va.Bind();
            ib.Bind();
            GLCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));


            //pic 3

            trans = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f,-0.5f, -1.0f));
             rot = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(1.0f));
             scl = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));

            /* Initialize the MVP matrices */ 
             model = trans * rot * scl;
             view = camera.GetViewMatrix();
             proj = camera.GetProjectionMatrix();
             mvp = proj * view * model;

            /* Update shaders paramters and draw to the screen */
            shader.Bind();
            shader.SetUniform4f("u_Color", color);
            shader.SetUniformMat4f("u_MVP", mvp);
            texture3.Bind(0);  // Bind first texture to texture unit 0
            shader.SetUniform1i("u_Texture", 0);
            va.Bind();
            ib.Bind();
            GLCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));


            //pic 4
            trans = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -0.5f, -1.0f));
             rot = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(1.0f));
             scl = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));

            /* Initialize the MVP matrices */ 
             model = trans * rot * scl;
             view = camera.GetViewMatrix();
             proj = camera.GetProjectionMatrix();
             mvp = proj * view * model;

            /* Update shaders paramters and draw to the screen */
            shader.Bind();
            shader.SetUniform4f("u_Color", color);
            shader.SetUniformMat4f("u_MVP", mvp);
            texture4.Bind(0);  // Bind first texture to texture unit 0
            shader.SetUniform1i("u_Texture", 0);
            va.Bind();
            ib.Bind();
            GLCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));
            // /* Swap front and back buffers */
            glfwSwapBuffers(window);

            /* Poll for and process events */
            glfwPollEvents();
        }
    }

    glfwTerminate();
    //free pointers
    delete [] buffer_gray;
    delete [] result_buffer_gray;
    delete [] buffer_canny;
    delete [] result_buffer_haftone;
    delete [] buffer_ld;
    return 0;
}