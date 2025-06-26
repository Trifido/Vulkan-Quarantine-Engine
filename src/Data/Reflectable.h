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
#include <PhysicsTypes.h>
#include <LightType.h>
#include <AtmosphereType.h>

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
    QEMetaType* base = nullptr;

    void addField(const std::string& name, std::type_index type, size_t offset)
    {
        fields.push_back({ name, type, offset });
    }

    std::vector<QEMetaField> allFields() const {
        if (!base) return fields;
        auto v = base->allFields();
        v.insert(v.end(), fields.begin(), fields.end());
        return v;
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
    for (const auto& field : meta->allFields()) {
        void* fieldPtr = (void*)((char*)comp + field.offset);
        if (field.type == typeid(int)) {
            node[field.name] = *(int*)fieldPtr;
        }
        else if (field.type == typeid(uint32_t)) {
            node[field.name] = *(uint32_t*)fieldPtr;
        }
        else if (field.type == typeid(bool)) {
            node[field.name] = *(bool*)fieldPtr;
        }
        else if (field.type == typeid(float)) {
            node[field.name] = *(float*)fieldPtr;
        }
        else if (field.type == typeid(std::string)) {
            node[field.name] = *(std::string*)fieldPtr;
        }
        else if (field.type == typeid(glm::vec2)) {
            auto v = *reinterpret_cast<glm::vec2*>(fieldPtr);
            node[field.name] = v;
        }
        else if (field.type == typeid(glm::vec3)) {
            auto v = *reinterpret_cast<glm::vec3*>(fieldPtr);
            node[field.name] = v;
        }
        else if (field.type == typeid(btVector3)) {
            auto v = *reinterpret_cast<btVector3*>(fieldPtr);
            node[field.name] = v;
        }
        else if (field.type == typeid(glm::vec4)) {
            auto v = *reinterpret_cast<glm::vec4*>(fieldPtr);
            node[field.name] = v;
        }
        else if (field.type == typeid(glm::quat)) {
            auto v = *reinterpret_cast<glm::quat*>(fieldPtr);
            node[field.name] = v;
        }
        else if (field.type == typeid(glm::mat4)) {
            auto v = *reinterpret_cast<glm::mat4*>(fieldPtr);
            node[field.name] = v;
        }
        else if (field.type == typeid(std::vector<std::string>)) {
            auto& vec = *reinterpret_cast<const std::vector<std::string>*>(fieldPtr);
            node[field.name] = vec;  // usa convert<vector<string>>::encode
        }
        else if (field.type == typeid(PhysicBodyType)) {
            auto v = *reinterpret_cast<PhysicBodyType*>(fieldPtr);
            node[field.name] = static_cast<int>(v);
        }
        else if (field.type == typeid(CollisionFlag)) {
            auto v = *reinterpret_cast<CollisionFlag*>(fieldPtr);
            node[field.name] = static_cast<int>(v);
        }
        else if (field.type == typeid(LightType)) {
            auto v = *reinterpret_cast<LightType*>(fieldPtr);
            node[field.name] = static_cast<uint32_t>(v);
        }
        else if (field.type == typeid(AtmosphereType)) {
            auto v = *reinterpret_cast<AtmosphereType*>(fieldPtr);
            node[field.name] = static_cast<uint32_t>(v);
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
    for (const auto& field : meta->allFields()) {
        void* fieldPtr = (void*)((char*)comp + field.offset);
        if (!node[field.name]) continue;
        if (field.type == typeid(int)) {
            *(int*)fieldPtr = node[field.name].as<int>();
        }
        else if (field.type == typeid(uint32_t)) {
            *(uint32_t*)fieldPtr = node[field.name].as<uint32_t>();
        }
        else if (field.type == typeid(bool)) {
            *(bool*)fieldPtr = node[field.name].as<bool>();
        }
        else if (field.type == typeid(float)) {
            *(float*)fieldPtr = node[field.name].as<float>();
        }
        else if (field.type == typeid(std::string)) {
            *(std::string*)fieldPtr = node[field.name].as<std::string>();
        }
        else if (field.type == typeid(glm::vec2)) {
            *reinterpret_cast<glm::vec2*>(fieldPtr) = node[field.name].as<glm::vec2>();
        }
        else if (field.type == typeid(glm::vec3)) {
            *reinterpret_cast<glm::vec3*>(fieldPtr) = node[field.name].as<glm::vec3>();
        }
        else if (field.type == typeid(btVector3)) {
            *reinterpret_cast<btVector3*>(fieldPtr) = node[field.name].as<btVector3>();
        }
        else if (field.type == typeid(glm::vec4)) {
            *reinterpret_cast<glm::vec4*>(fieldPtr) = node[field.name].as<glm::vec4>();
        }
        else if (field.type == typeid(glm::quat)) {
            *reinterpret_cast<glm::quat*>(fieldPtr) = node[field.name].as<glm::quat>();
        }
        else if (field.type == typeid(glm::mat4)) {
            *reinterpret_cast<glm::mat4*>(fieldPtr) = node[field.name].as<glm::mat4>();
        }
        else if (field.type == typeid(std::vector<std::string>)) {
            *reinterpret_cast<std::vector<std::string>*>(fieldPtr) = node[field.name].as<std::vector<std::string>>();
        }
        else if (field.type == typeid(PhysicBodyType))
        {
            int ival = node[field.name].as<int>();
            auto v = static_cast<PhysicBodyType>(ival);
            *reinterpret_cast<PhysicBodyType*>(fieldPtr) = v;
        }
        else if (field.type == typeid(CollisionFlag))
        {
            int ival = node[field.name].as<int>();
            auto v = static_cast<CollisionFlag>(ival);
            *reinterpret_cast<CollisionFlag*>(fieldPtr) = v;
        }
        else if (field.type == typeid(LightType))
        {
            uint32_t ival = node[field.name].as<uint32_t>();
            auto v = static_cast<LightType>(ival);
            *reinterpret_cast<LightType*>(fieldPtr) = v;
        }
        else if (field.type == typeid(AtmosphereType))
        {
            uint32_t ival = node[field.name].as<uint32_t>();
            auto v = static_cast<AtmosphereType>(ival);
            *reinterpret_cast<AtmosphereType*>(fieldPtr) = v;
        }
    }
}

#define REFLECTABLE_COMPONENT(Type)                                \
    using Self = Type;                                             \
    static QEMetaType* staticMeta() {                              \
        static QEMetaType meta{#Type};                             \
        static bool initialized = false;                           \
        if (!initialized) {                                        \
            initialized = true;                                    \
            meta.base = nullptr;   /* no hay clase base */        \
            /* añade aquí los campos propios: */                   \
            meta.addField("id", typeid(std::string), offsetof(Self, id)); \
            registerMetaType(#Type, &meta);                        \
            getFactoryRegistry()[#Type] = &Type::createInstance;   \
        }                                                          \
        return &meta;                                              \
    }                                                              \
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

#define REFLECTABLE_DERIVED_COMPONENT(Type, BaseType)                        \
    using Self = Type;                                                       \
    static QEMetaType* staticMeta() {                                        \
        static QEMetaType meta{#Type};                                       \
        static bool initialized = false;                                     \
        if (!initialized) {                                                  \
            initialized = true;                                              \
            /* 1) Apuntamos al meta de la base */                            \
            meta.base = BaseType::staticMeta();                              \
            /* 2) Ya registrarCamposPropios se hará por los REFLECT_PROPERTY */\
            /* 3) Registramos este tipo en el registry */                    \
            registerMetaType(#Type, &meta);                                  \
            getFactoryRegistry()[#Type] = &Type::createInstance;             \
        }                                                                    \
        return &meta;                                                        \
    }                                                                        \
    QEMetaType* meta()   const override { return staticMeta(); }             \
    const std::string& getTypeName() const override {                        \
        static const std::string name = #Type;                               \
        return name;                                                         \
    }                                                                        \
    static std::unique_ptr<QEGameComponent> createInstance() {               \
        return std::make_unique<Type>();                                     \
    }                                                                        \
    struct AutoRegister_##Type {                                             \
        AutoRegister_##Type() {                                              \
            /* El registro global ya está hecho en staticMeta() */           \
        }                                                                    \
    } _autoRegister_##Type;

void exportToFile(const YAML::Node& node, const std::string& filename);

YAML::Node importFromFile(const std::string& filename);

#endif // !QE_REFLECTABLE
