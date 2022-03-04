#pragma once

#include "Graphics/Graphics_2.h"

#include "Asset/Asset.h"



//Body in HelperAssets.cpp
std::shared_ptr<Texture_2D> __texture2D_loader__(std::string filedir);
void __texture2D_unloader__(std::shared_ptr<Texture_Bindless_2D> ptr);


typedef Asset<Texture_2D, __texture2D_loader__> Texture2DAsset;