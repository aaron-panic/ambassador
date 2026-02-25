#ifndef RUNTIME_OBJECT_HXX_INCLUDED
#define RUNTIME_OBJECT_HXX_INCLUDED

class RuntimeObject {
public:
    virtual ~RuntimeObject() = default;
    virtual const char* typeName() const noexcept = 0;
};

#endif
