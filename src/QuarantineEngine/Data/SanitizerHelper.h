#pragma once

#ifndef SANITIZER_HELPER_H
#define SANITIZER_HELPER_H

#include <algorithm>
#include <unordered_map>
#include <string>
#include <cctype>

namespace SanitizerHelper
{
    static inline std::string SanitizeName(std::string s)
    {
        // 1) trim
        auto isspace_l = [](unsigned char c) { return std::isspace(c) != 0; };
        while (!s.empty() && isspace_l(s.front())) s.erase(s.begin());
        while (!s.empty() && isspace_l(s.back()))  s.pop_back();

        // 2) opcional: tolower para evitar variantes de mayúsculas
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });

        // 3) normaliza sufijos L/R comunes a ":l"/":r"
        auto normalizeLR = [](std::string& t, const std::string& pat, const std::string& rep) {
            if (t.size() >= pat.size() && t.compare(t.size() - pat.size(), pat.size(), pat) == 0)
                t.replace(t.size() - pat.size(), pat.size(), rep);
            };
        normalizeLR(s, ".l", ":l");  normalizeLR(s, "_l", ":l");
        normalizeLR(s, "-l", ":l");  normalizeLR(s, ".r", ":r");
        normalizeLR(s, "_r", ":r");  normalizeLR(s, "-r", ":r");
        normalizeLR(s, " left", ":l"); normalizeLR(s, " right", ":r");

        // 4) cambia separadores raros por ':'
        auto is_sep = [](unsigned char c)
            {
                switch (c) {
                    case ' ': case '_': case '-': case '.': case '/': case '\\': case '|':
                        return true;
                    default: return false;
                }
            };
        for (auto& c : s) if (is_sep((unsigned char)c)) c = ':';

        // 5) colapsa ':::' -> ':'
        std::string out; out.reserve(s.size());
        bool prevColon = false;
        for (char c : s)
        {
            if (c == ':') {
                if (!prevColon) out.push_back(c);
                prevColon = true;
            }
            else {
                out.push_back(c);
                prevColon = false;
            }
        }

        // 6) quita ':' al principio/fin
        if (!out.empty() && out.front() == ':') out.erase(out.begin());
        if (!out.empty() && out.back() == ':')  out.pop_back();

        return out;
    }

    static void ResolveCollisions(std::unordered_map<std::string, std::string>& old2new)
    {
        std::unordered_map<std::string, int> counts;
        for (auto& kv : old2new) counts[kv.second]++;
        for (auto& kv : old2new)
        {
            if (counts[kv.second] > 1)
            {
                int idx = --counts[kv.second];
                kv.second = kv.second + ":" + std::to_string(idx);
            }
        }
    }

    struct NameRemap
    {
        std::unordered_map<std::string, std::string> map;
        const std::string& Canonical(const std::string& s)
        {
            auto it = map.find(s);
            if (it != map.end()) return it->second;
            auto canon = SanitizeName(s);
            auto [it2, _] = map.emplace(s, std::move(canon));
            return it2->second;
        }
    };

    static void SanitizeSceneNames(aiScene* sc) {
        if (!sc || !sc->mRootNode) return;

        // 1) Recolecta todos los nombres actuales
        std::vector<aiNode*> nodes;
        std::function<void(aiNode*)> dfs = [&](aiNode* n) {
            nodes.push_back(n);
            for (unsigned i = 0; i < n->mNumChildren; ++i) dfs(n->mChildren[i]);
            };
        dfs(sc->mRootNode);

        std::unordered_map<std::string, std::string> old2new;

        // 2) Proponer nombres canónicos para nodos
        for (auto* n : nodes) {
            std::string old = n->mName.C_Str();
            old2new[old] = SanitizeName(old);
        }
        // 3) También huesos y canales
        for (unsigned m = 0; m < sc->mNumMeshes; ++m) {
            aiMesh* mesh = sc->mMeshes[m];
            for (unsigned b = 0; b < mesh->mNumBones; ++b) {
                std::string old = mesh->mBones[b]->mName.C_Str();
                old2new[old] = SanitizeName(old);
            }
        }
        for (unsigned a = 0; a < sc->mNumAnimations; ++a) {
            aiAnimation* anim = sc->mAnimations[a];
            for (unsigned c = 0; c < anim->mNumChannels; ++c) {
                std::string old = anim->mChannels[c]->mNodeName.C_Str();
                old2new[old] = SanitizeName(old);
            }
        }

        // 4) Resolver colisiones (importantísimo)
        ResolveCollisions(old2new);

        // 5) Aplicar nombres nuevos
        auto applyName = [&](aiString& s) {
            auto it = old2new.find(s.C_Str());
            if (it != old2new.end()) s = aiString(it->second);
            };

        for (auto* n : nodes) applyName(n->mName);
        for (unsigned m = 0; m < sc->mNumMeshes; ++m) {
            aiMesh* mesh = sc->mMeshes[m];
            for (unsigned b = 0; b < mesh->mNumBones; ++b) applyName(mesh->mBones[b]->mName);
        }
        for (unsigned a = 0; a < sc->mNumAnimations; ++a) {
            aiAnimation* anim = sc->mAnimations[a];
            for (unsigned c = 0; c < anim->mNumChannels; ++c) applyName(anim->mChannels[c]->mNodeName);
        }
    }
};

#endif // !SANITIZER_HELPER_H

