#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char **args)
{
    if (argc < 2)
    {
        std::cerr << "Too few arguments!" << std::endl;
        return 1;
    }

    if (std::filesystem::exists(args[1]))
    {
        std::filesystem::recursive_directory_iterator iterator(args[1]);
        std::ofstream output("embedded.hpp");
        output << "#pragma once" << std::endl
               << "#include <map>" << std::endl
               << "#include <string>" << std::endl
               << "#include <vector>" << std::endl
               << "inline const std::map<const std::string, const std::vector<unsigned char>> embedded_files = {";

        for (const auto &file : iterator)
        {
            if (file.path().filename().string().at(0) == '.' || !file.is_regular_file())
                continue;

            output << "{\"" << file.path().filename().string() << "\", {";

            std::ifstream fileStream(file.path(), std::ios::binary);
            std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(fileStream), {});

            for (auto it = buffer.begin(); it != buffer.end(); it++)
            {
                output << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(*it);
                if (std::distance(it, buffer.end()) > 1)
                {
                    output << ",";
                }
            }

            output << "}}," << std::endl;
        }

        output << "};";
    }
    else
    {
        std::cerr << "Invalid path!" << std::endl;
    }

    return 0;
}