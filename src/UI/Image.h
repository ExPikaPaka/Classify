#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <string>

#include "..\Logger\Logger.h"
#include "..\Render\Shader.h"

namespace ent {
    namespace ui {
        class Image {
        public:
            Image();
            ~Image();
            void init(unsigned char* data, ui32 width, ui32 height);
            void RenderImage(render::Shader& shader, float x, float y, float width, float height);
            void always_on_top(bool value);
            void setPosition(float x, float y);
            void applyOrthoMatrix(ent::render::Shader& shader, ui32 windowWidth, ui32 windowHeight);
        private:
            unsigned int TextureID;
            unsigned int VAO = 0, VBO = 0;
            bool m_AlwaysOnTop;
            float m_X, m_Y;
            float m_Width, m_Height;

            ent::util::Logger* logger;

            void clear();
            void loadTexture(unsigned char* data, ui32 width, ui32 height);
        };
    }
}
