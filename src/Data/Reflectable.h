#pragma once
#ifndef QE_REFLECTABLE
#define QE_REFLECTABLE

#include <string>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <yaml-cpp/yaml.h>
#include "glm_yaml_conversions.h"
#include <memory>
#include <iostream>

struct SerializableComponent
{
    virtual ~SerializableComponent() = default;
    virtual const std::string& getTypeName() const = 0;
    virtual struct QEMetaType* meta() const = 0;
};

class QEGameComponent; // forward

struct QEMetaField
{
    std::string name;
    std::type_index type;
    size_t offset;
};

struct QEMetaType
{
    std::string typeName;
    std::vector<QEMetaField> fields;

    void addField(const std::string& name, std::type_index type, size_t offset)
    {
        fields.push_back({ name, type, offset });
    }
};

inline std::unordered_map<std::string, QEMetaType*>& getMetaRegistry()
{
    static std::unordered_map<std::string, QEMetaType*> registry;
    return registry;
}

inline void registerMetaType(const std::string& name, QEMetaType* meta)
{
    getMetaRegistry()[name] = meta;
}


inline QEMetaType* getMetaType(const std::string& name)
{
    auto it = getMetaRegistry().find(name);
    return it != getMetaRegistry().end() ? it->second : nullptr;
}

inline std::unordered_map<std::string, std::function<std::unique_ptr<class QEGameComponent>()>>& getFactoryRegistry()
{
    static std::unordered_map<std::string, std::function<std::unique_ptr<class QEGameComponent>()>> registry;
    return registry;
}

inline YAML::Node serializeComponent(const SerializableComponent* comp)
{
    YAML::Node node;
    node["type"] = comp->getTypeName();
    QEMetaType* meta = comp->meta();
    for (const auto& field : meta->fields) {
        void* fieldPtr = (void*)((char*)comp + field.offset);
        if (field.type == typeid(int)) {
            node[field.name] = *(int*)fieldPtr;
        }
        else if (field.type == typeid(float)) {
            node[field.name] = *(float*)fieldPtr;
        }
        else if (field.type == typeid(std::string)) {
            node[field.name] = *(std::string*)fieldPtr;
        }
        else if (field.type == typeid(glm::vec3)) {
            auto v = *reinterpret_cast<glm::vec3*>(fieldPtr);
            node[field.name] = v;  // usará convert<glm::vec3>::encode
        }
        else {
            node[field.name] = "<unsupported type>";
        }
    }
    return node;
}

inline void deserializeComponent(SerializableComponent* comp, const YAML::Node& node)
{
    QEMetaType* meta = comp->meta();
    for (const auto& field : meta->fields) {
        void* fieldPtr = (void*)((char*)comp + field.offset);
        if (!node[field.name]) continue;
        if (field.type == typeid(int)) {
            *(int*)fieldPtr = node[field.name].as<int>();
        }
        else if (field.type == typeid(float)) {
            *(float*)fieldPtr = node[field.name].as<float>();
        }
        else if (field.type == typeid(glm::vec3)) {
            *reinterpret_cast<glm::vec3*>(fieldPtr) = node[field.name].as<glm::vec3>();
        }
        else if (field.type == typeid(std::string)) {
            *(std::string*)fieldPtr = node[field.name].as<std::string>();
        }
    }
}

#define REFLECTABLE_COMPONENT(Type) \
    using Self = Type; \
    using Self = Type;                                                                 \
    static QEMetaType* staticMeta() {                                                  \
        static QEMetaType meta{#Type};                                                 \
        static bool initialized = false;                                               \
        if (!initialized) {                                                            \
            initialized = true;                                                        \
            meta.addField(                                                             \
              "id",                                                                    \
              typeid(std::string),                                                     \
              offsetof(Self, id)                                                       \
            );                                                                         \
        }                                                                              \
        return &meta;                                                                  \
    }                                                                                  \
    QEMetaType* meta() const override { return staticMeta(); } \
    const std::string& getTypeName() const override { \
        static const std::string name = #Type; \
        return name; \
    } \
    static std::unique_ptr<QEGameComponent> createInstance() { \
        return std::make_unique<Type>(); \
    } \
    static struct AutoRegister { \
        AutoRegister() { \
            registerMetaType(#Type, staticMeta()); \
            getFactoryRegistry()[#Type] = &Type::createInstance; \
        } \
    } _autoRegisterInstance;

#define REFLECT_PROPERTY(Type, Name) \
    Type Name; \
    struct AutoField_##Name { \
        AutoField_##Name() { \
            Self::staticMeta()->addField(#Name, typeid(Type), offsetof(Self, Name)); \
        } \
    } _autoField_##Name;


void exportToFile(const YAML::Node& node, const std::string& filename);

YAML::Node importFromFile(const std::string& filename);

#endif // !QE_REFLECTABLE
