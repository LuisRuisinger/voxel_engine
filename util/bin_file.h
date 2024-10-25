//
// Created by Luis Ruisinger on 16.09.24.
//

#ifndef OPENGL_3D_ENGINE_BIN_FILE_H
#define OPENGL_3D_ENGINE_BIN_FILE_H

#include <fstream>

#include "defines.h"
#include "log.h"

namespace util::bin_file {

    template <typename T>
    class File {
        using Byte = u8;

    public:
        File(std::string file_name, u32 width, u32 height, u32 components, bool y_flip) {
            std::ifstream file { file_name, std::ios::binary };
            if (!file) {
                LOG(util::log::LOG_LEVEL_ERROR, "Failed to load", file_name);
                std::exit(EXIT_FAILURE);
            }

            file.seekg(0 , std::ios::end);
            u32 file_len = file.tellg();

            file.seekg(0, std::ios::beg);
            auto buffer = new T[file_len / sizeof(T)];
            file.read(reinterpret_cast<char *>(buffer), file_len);
            file.close();

            // y-axis flip
            if (y_flip) {
                auto swap_buffer = new T[file_len / sizeof(T)];

                for (auto i = 0; i < width; ++i) {
                    for (auto j = 0; j < height; ++j) {
                        auto target_entry = (i + j * width) * components;
                        auto source_entry = (i + (height - 1 - j) * height) * components;

                        for (auto k = 0; k < components; ++k) {
                            swap_buffer[target_entry + k] = buffer[source_entry + k];
                        }
                    }
                }

                delete[] buffer;
                this->mem = reinterpret_cast<Byte *>(swap_buffer);
            }
            else {
                this->mem = reinterpret_cast<Byte *>(buffer);
            }
        }

        ~File() {
            delete[] this->mem;
        }

        auto buffer() -> const Byte * {
            return this->mem;
        }

    private:
        Byte *mem;
    };
};


#endif //OPENGL_3D_ENGINE_BIN_FILE_H
