#ifndef RUNTIME_IMAGE_HXX_INCLUDED
#define RUNTIME_IMAGE_HXX_INCLUDED

#include "amb_types.hxx"
#include "runtime_object.hxx"

class ImageRuntime final : public RuntimeObject {
public:
    TexturePtr texture;

    const char* typeName() const noexcept override { return "ImageRuntime"; }
};

#endif
