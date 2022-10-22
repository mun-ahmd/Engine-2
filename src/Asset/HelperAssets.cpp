#include "Asset/TextureAsset.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//TextureAsset Body:

constexpr GLenum __image_data_formats__[4] = { GL_RED, GL_RG, GL_RGB, GL_RGBA };
std::shared_ptr<Texture_2D> __texture2D_loader__(std::string filedir) {
    struct {
        int w; int h; int channels;
        unsigned char* data = NULL;
        inline Texture_2D create_tex_2D(GLenum format, GLenum internal_format)
        {
            return Texture_2D(w, h,internal_format, format, GL_UNSIGNED_BYTE, data);
        }
    } texture_create_info;
    texture_create_info.data = stbi_load(filedir.c_str(), &texture_create_info.w, &texture_create_info.h, &texture_create_info.channels, 0);
    assert(texture_create_info.channels > 0 && texture_create_info.channels <= 4);
    return std::make_shared<Texture_2D>(
        texture_create_info.w,
        texture_create_info.h,
        __image_data_formats__[texture_create_info.channels],
        __image_data_formats__[texture_create_info.channels],
        GL_UNSIGNED_BYTE,
        texture_create_info.data
        );
}


void __texture2D_unloader__(std::shared_ptr<Texture_2D> ptr) {
    ptr->destroy();
}