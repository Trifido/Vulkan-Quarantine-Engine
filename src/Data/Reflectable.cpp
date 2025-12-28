#include "Reflectable.h"
#include <fstream>

void exportToFile(const YAML::Node& node, const std::string& filename)
{
    std::ofstream fout(filename);
    if (!fout.is_open())
    {
        throw std::runtime_error("No se pudo abrir el archivo para escritura: " + filename);
    }

    fout << node;
    fout.close();
}

YAML::Node importFromFile(const std::string& filename)
{
    std::ifstream fin(filename);
    if (!fin.is_open())
    {
        throw std::runtime_error("No se pudo abrir el archivo para lectura: " + filename);
    }

    return YAML::Load(fin);
}
