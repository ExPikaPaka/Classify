#include "Text.h"

namespace ent {
	namespace ui {
        Text::Text() {
        }

        Text::~Text() {
            clear();
        }

        Text::Text(std::string fontPath) {
            init(fontPath);
        }

        void Text::init(std::string fontPath, ui32 charWidth, ui32 charHeight) {
            // Clear
            clear();

            // Logger initialization
            logger = util::Logger::getInstance();
            logger->addLog("Initializing text", util::level::DEBUG);

            // Value initialization
            bool error = false;
            m_AlwaysOnTop = false;
            m_CharWidth = charWidth;
            m_CharHeight = charHeight;
            m_FontPath = fontPath;

            // All functions return a value different than 0 whenever an error occurred
            if (FT_Init_FreeType(&ft)) {
                logger->addLog("Could not init FreeType Library", ent::util::level::FATAL);
                error = true;
            }

            // find path to font
            std::string font_name = fontPath;
            if (font_name.empty()) {
                logger->addLog("Failed to load font_name", ent::util::level::FATAL);
                error = true;
            }

            // load font as face
            FT_Face face;
            if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
                logger->addLog("Failed to load font <" + font_name, ent::util::level::FATAL);
                error = true;
            } else {
                // set size to load glyphs as
                FT_Set_Pixel_Sizes(face, m_CharWidth, m_CharHeight);

                // disable byte-alignment restriction
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                // load first 127 characters of ASCII set
                for (ui8 c = 0; c < 128; c++) {
                    // Load character glyph 
                    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                        logger->addLog("Failed to load Glyph", ent::util::level::FATAL);
                        error = true;
                        continue;
                    }
                    // generate texture
                    unsigned int texture;
                    glGenTextures(1, &texture);
                    glBindTexture(GL_TEXTURE_2D, texture);
                    glTexImage2D(
                        GL_TEXTURE_2D,
                        0,
                        GL_RED,
                        face->glyph->bitmap.width,
                        face->glyph->bitmap.rows,
                        0,
                        GL_RED,
                        GL_UNSIGNED_BYTE,
                        face->glyph->bitmap.buffer
                    );
                    // set texture options
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    // now store character for later use
                    Character character = {
                        texture,
                        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                        static_cast<unsigned int>(face->glyph->advance.x)
                    };
                    Characters.insert(std::pair<char, Character>(c, character));
                }
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            // destroy FreeType once we're finished
            FT_Done_Face(face);
            FT_Done_FreeType(ft);


            // configure VAO/VBO for texture quads
            // -----------------------------------
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            if (!error) {
                logger->addLog("Text renderer initialized successfully", ent::util::level::INFO);
            }
        }

        void Text::RenderText(render::Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color) {
            // activate corresponding render state	
            shader.use();
            shader.setVec3("textColor", color);
            
            // Clearing Z-buffer if needed
            if (m_AlwaysOnTop) {
                glClear(GL_DEPTH_BUFFER_BIT);
            }

            glActiveTexture(GL_TEXTURE0);
            glBindVertexArray(VAO);

            // iterate through all characters
            std::string::const_iterator c;
            for (c = text.begin(); c != text.end(); c++) {
                Character ch = Characters[*c];
                
                if (ch.TextureID == -1) {
                    ch = Characters['#'];
                }

                float xpos = x + ch.Bearing.x * scale;
                float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

                float w = ch.Size.x * scale;
                float h = ch.Size.y * scale;
                // update VBO for each character
                float vertices[6][4] = {
                    { xpos,     ypos + h,   0.0f, 0.0f },
                    { xpos,     ypos,       0.0f, 1.0f },
                    { xpos + w, ypos,       1.0f, 1.0f },

                    { xpos,     ypos + h,   0.0f, 0.0f },
                    { xpos + w, ypos,       1.0f, 1.0f },
                    { xpos + w, ypos + h,   1.0f, 0.0f }
                };
                // render glyph texture over quad
                glBindTexture(GL_TEXTURE_2D, ch.TextureID);
                // update content of VBO memory
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                // render quad
                glDrawArrays(GL_TRIANGLES, 0, 6);
                // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
                x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
            }
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        void Text::always_on_top(bool value) {
            m_AlwaysOnTop = value;
        }
        void Text::setSize(ui32 charWidth, ui32 charHeight) {
            init(m_FontPath, charWidth, charHeight);
        }
        void Text::applyOrthoMatrix(ent::render::Shader shader, ui32 windowWidth, ui32 windowHeight) {
            glm::mat4 projection = glm::ortho(0.0f, (f32)windowWidth, 0.0f, (f32)windowHeight);

            shader.use();
            shader.setMat4("projection", projection);
        }
        void Text::clear() {
            // Free resources
            for (const auto& pair : Characters) {
                if (pair.second.TextureID != 0) {
                    glDeleteTextures(1, &pair.second.TextureID);
                }
            }
            Characters.clear();

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




