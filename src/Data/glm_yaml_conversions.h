#pragma once
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

#include "AtmosphereDto.h"

#include <Jolt/Jolt.h>
#include <Jolt/Math/Vec3.h>

namespace YAML {

    template<>
    struct convert<glm::vec2> {
        static Node encode(const glm::vec2& v) {
            Node node;
            node.SetStyle(YAML::EmitterStyle::Flow);
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
            node.SetStyle(YAML::EmitterStyle::Flow);
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
    struct convert<JPH::Vec3> {
        static Node encode(const JPH::Vec3& v)
        {
            Node node;
            node.SetStyle(YAML::EmitterStyle::Flow);
            node.push_back(v[0]);
            node.push_back(v[1]);
            node.push_back(v[2]);
            return node;
        }

        static bool decode(const Node& node, JPH::Vec3& v)
        {
            if (!node.IsSequence() || node.size() != 3) return false;
            float x = node[0].as<float>();
            float y = node[1].as<float>();
            float z = node[2].as<float>();
            v = JPH::Vec3(x, y, z);
            return true;
        }
    };

    template<>
    struct convert<glm::vec4> {
        static Node encode(const glm::vec4& v) {
            Node node;
            node.SetStyle(YAML::EmitterStyle::Flow);
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
            node.SetStyle(YAML::EmitterStyle::Flow);
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
    struct convert<glm::mat4>
    {
        static Node encode(const glm::mat4& m)
        {
            YAML::Node n;
            // n.SetStyle(YAML::EmitterStyle::Block); // (opcional) ya es el predeterminado
            for (int r = 0; r < 4; ++r)
            {
                YAML::Node row;
                row.SetStyle(YAML::EmitterStyle::Flow); // [a, b, c, d]
                for (int c = 0; c < 4; ++c)
                {
                    // glm es column-major: m[col][row]
                    row.push_back(m[c][r]);
                }
                n.push_back(row);
            }
            return n;
        }

        static bool decode(const Node& n, glm::mat4& m)
        {
            if (!n.IsSequence() || n.size() != 4) return false;
            for (int r = 0; r < 4; ++r)
            {
                const YAML::Node& row = n[r];
                if (!row.IsSequence() || row.size() != 4) return false;
                for (int c = 0; c < 4; ++c)
                {
                    m[c][r] = row[c].as<float>(); // column-major
                }
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
