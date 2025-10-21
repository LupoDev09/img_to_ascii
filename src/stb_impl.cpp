// Single translation unit to provide STB implementations
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../include/stb_image.h"           // this line must be here because otherwise it won't work
#include "../include/stb_image_write.h"     // this line must be here because otherwise it won't work
