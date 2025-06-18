#pragma once
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <btBulletDynamicsCommon.h>
#include <vector>

namespace YAML {

    template<>
    struct convert<glm::vec2> {
        static Node encode(const glm::vec2& v) {
            Node node;
            node.push_back(v.x);
            node.push_back(v.y);
            return node;
        }

        static bool decode(const Node& node, glm::vec2& v) {
            if (!node.IsSequence() || node.size() != 2) return false;
            v.x = node[0].as<float>();
            v.y = node[1].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::vec3> {
        static Node encode(const glm::vec3& v) {
            Node node;
            node.push_back(v.x);
            node.push_back(v.y);
            node.push_back(v.z);
            return node;
        }

        static bool decode(const Node& node, glm::vec3& v) {
            if (!node.IsSequence() || node.size() != 3) return false;
            v.x = node[0].as<float>();
            v.y = node[1].as<float>();
            v.z = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<btVector3> {
        static Node encode(const btVector3& v) {
            Node node;
            node.push_back(v[0]);
            node.push_back(v[1]);
            node.push_back(v[2]);
            return node;
        }

        static bool decode(const Node& node, btVector3& v) {
            if (!node.IsSequence() || node.size() != 3) return false;
            v[0] = node[0].as<float>();
            v[1] = node[1].as<float>();
            v[2] = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::vec4> {
        static Node encode(const glm::vec4& v) {
            Node node;
            node.push_back(v.x);
            node.push_back(v.y);
            node.push_back(v.z);
            node.push_back(v.w);
            return node;
        }

        static bool decode(const Node& node, glm::vec4& v) {
            if (!node.IsSequence() || node.size() != 4) return false;
            v.x = node[0].as<float>();
            v.y = node[1].as<float>();
            v.z = node[2].as<float>();
            v.w = node[3].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::quat> {
        static Node encode(const glm::quat& q) {
            Node node;
            // guardamos en orden (x, y, z, w)
            node.push_back(q.x);
            node.push_back(q.y);
            node.push_back(q.z);
            node.push_back(q.w);
            return node;
        }

        static bool decode(const Node& node, glm::quat& q) {
            if (!node.IsSequence() || node.size() != 4) return false;
            // reconstruimos a través de su constructor (w, x, y, z)
            float x = node[0].as<float>();
            float y = node[1].as<float>();
            float z = node[2].as<float>();
            float w = node[3].as<float>();
            q = glm::quat{ w, x, y, z };
            return true;
        }
    };

    template<>
    struct convert<glm::mat4> {
        static Node encode(const glm::mat4& m) {
            Node node;
            node.push_back(m[0][0]);
            node.push_back(m[0][1]);
            node.push_back(m[0][2]);
            node.push_back(m[0][3]);
            node.push_back(m[1][0]);
            node.push_back(m[1][1]);
            node.push_back(m[1][2]);
            node.push_back(m[1][3]);
            node.push_back(m[2][0]);
            node.push_back(m[2][1]);
            node.push_back(m[2][2]);
            node.push_back(m[2][3]);
            node.push_back(m[3][0]);
            node.push_back(m[3][1]);
            node.push_back(m[3][2]);
            node.push_back(m[3][3]);
            return node;
        }

        static bool decode(const Node& node, glm::mat4& m) {
            if (!node.IsSequence() || node.size() != 16) return false;
            for (int i = 0; i < 16; ++i) {
                // glm::mat4 se accede como m[col][row], es una matriz 4x4 en columnas
                m[i / 4][i % 4] = node[i].as<float>();
            }
            return true;
        }
    };

    template<typename T>
    struct convert<std::vector<T>> {
        static Node encode(const std::vector<T>& in) {
            Node node(NodeType::Sequence);
            for (auto& el : in)
                node.push_back(el);       // usa convert<T>::encode o Node::operator=(T)
            return node;
        }
        static bool decode(const Node& node, std::vector<T>& out) {
            if (!node.IsSequence()) return false;
            out.clear();
            out.reserve(node.size());
            for (auto& el : node)
                out.push_back(el.as<T>());// usa convert<T>::decode o as<T>()
            return true;
        }
    };
}
