#include "Image.h"

namespace ent {
    namespace ui {
        Image::Image() : TextureID(0), m_X(0.0f), m_Y(0.0f), m_Width(0.0f), m_Height(0.0f), m_AlwaysOnTop(false) {
            logger = util::Logger::getInstance();
        }

        Image::~Image() {
            clear();
        }

        void Image::init(unsigned char* data, ui32 width, ui32 height) {
            clear();
            logger->addLog("Initializing image rendering", ent::util::level::DEBUG);

            // Load the image texture
            loadTexture(data, width, height);

            // Configure VAO/VBO for quad rendering
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            logger->addLog("Image renderer initialized successfully", ent::util::level::INFO);
        }

        void Image::loadTexture(unsigned char* data, ui32 width, ui32 height) {
            if (data) {
                glGenTextures(1, &TextureID);
                glBindTexture(GL_TEXTURE_2D, TextureID);

                // Set the texture wrapping/filtering options
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                GLenum format = GL_RGB;
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);

                m_Width = static_cast<float>(width);
                m_Height = static_cast<float>(height);
            }
        }

        void Image::RenderImage(render::Shader& shader, float x, float y, float width, float height) {
            shader.use();

            if (m_AlwaysOnTop) {
                glClear(GL_DEPTH_BUFFER_BIT);
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, TextureID);

            glBindVertexArray(VAO);

            // Set up vertices for the image quad
            float vertices[6][4] = {
                { x, y + height, 0.0f, 0.0f },
                { x, y, 0.0f, 1.0f },
                { x + width, y, 1.0f, 1.0f },

                { x, y + height, 0.0f, 0.0f },
                { x + width, y, 1.0f, 1.0f },
                { x + width, y + height, 1.0f, 0.0f }
            };

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        void Image::always_on_top(bool value) {
            m_AlwaysOnTop = value;
        }

        void Image::setPosition(float x, float y) {
            m_X = x;
            m_Y = y;
        }

        void Image::applyOrthoMatrix(ent::render::Shader& shader, ui32 windowWidth, ui32 windowHeight) {
            glm::mat4 projection = glm::ortho(0.0f, (f32)windowWidth, 0.0f, (f32)windowHeight);
            shader.use();
            shader.setMat4("projection", projection);
        }

        void Image::clear() {
            if (TextureID != 0) {
                glDeleteTextures(1, &TextureID);
                TextureID = 0;
            }

            if (VBO != 0) {
                glDeleteBuffers(1, &VBO);
                VBO = 0;
            }

            if (VAO != 0) {
                glDeleteVertexArrays(1, &VAO);
                VAO = 0;
            }
        }
    }
}
