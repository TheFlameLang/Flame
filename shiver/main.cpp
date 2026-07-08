#include "toml.hpp"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <termcolor/termcolor.hpp>

int main(int argc, char* argv[]) {
    bool init = false;
    bool skip_init = false;
    bool build = false;
    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i], "init")==0) {
            init = true;
        } else if(strcmp(argv[i], "skip")==0&&init) {
            skip_init = true;
        } else if(strcmp(argv[i], "build")==0) {
            build = true;
        }
    }


    if(init) {
        if(std::filesystem::exists("./project.toml")) {
                std::cout << "project.toml is already exist in current directory. Do you want to overwrite it? (Y/N)\n";
                std::string input = "";
                std::cin >> input;
                if(input!="Y"&&input!="y") {
                    return 0;
                }
            }
        if (skip_init) {
            std::ofstream f("project.toml");
            f << "name = \"example\"\n";
            f << "version = \"1.0.0\"\n";
            f << "\n";
            f << "output = \"example\"\n";
            f << "license = \"MIT\"\n";
            f << "author = \"cool guy\"\n";
            f << "github = \"https://github.com/\"\n";

            f << "[build]\n";
            f << "cxx = \"g++\"\n";
            f << "cxxflags = \"-g -O1 -Wall\"\n";
            f << "ld = \"ld\"\n";
            f << "src = [] # order will matter";
        } else {
            std::ofstream f("project.toml");
            std::string temp = "";
            std::cout << termcolor::blue << "Project name: " << termcolor::reset; std::getline(std::cin >> std::ws, temp);
            f << "name = \"" << temp << "\"\n";
            std::cout << termcolor::blue << "Project version: " << termcolor::reset; std::getline(std::cin >> std::ws, temp);
            f << "version = \"" << temp << "\"\n";
            std::cout << termcolor::blue << "Project output file name: " << termcolor::reset; std::getline(std::cin >> std::ws, temp);
            f << "\n";
            f << "output = \"" << temp << "\"\n";
            std::cout << termcolor::blue << "Project license[MIT, Apache2, GPL2, GPL3]: " << termcolor::reset; std::getline(std::cin >> std::ws, temp);
            f << "license = \"" << temp << "\"\n";
            std::cout << termcolor::blue << "Project author: " << termcolor::reset; std::getline(std::cin >> std::ws, temp);
            f << "author = \"" << temp << "\"\n";
            std::cout << termcolor::blue << "Project github: " << termcolor::reset; std::getline(std::cin >> std::ws, temp);
            f << "github = \"" << temp << "\"\n";

            f << "[build]\n";
            std::cout << termcolor::blue << "C++ compiler: " << termcolor::reset; std::getline(std::cin >> std::ws, temp);
            f << "cxx = \"" << temp << "\"\n";
            std::cout << termcolor::blue << "C++ flags: " << termcolor::reset; std::getline(std::cin >> std::ws, temp);
            f << "cxxflags = \"" << temp << "\"\n";
            std::cout << termcolor::blue << "Linker: " << termcolor::reset; std::getline(std::cin >> std::ws, temp);
            f << "ld = \"" << temp << "\"\n";
            std::cout << termcolor::green << "Done.\n" << termcolor::reset;
        }
        return 0;
    }

    if(build) {
        if(!std::filesystem::exists("./project.toml")) {
            std::cerr << termcolor::red << "Error: " << termcolor::reset <<
            "cannot find project.toml in current directory. Run `shiver init` to create it.\n";
            return 1;
        }
        toml::table t = toml::parse_file("./project.toml");
        std::string name = t["name"].value_or("");
        std::string ver = t["version"].value_or("");
        std::cout << "Running basic check...";
        std::vector<std::string> src;
        if (toml::array* arr = t["build"]["src"].as_array()) {
            arr->for_each([&src](auto&& e){
                src.emplace_back(e.value_or(""));
            });
        }

        for(auto &x : src) {
            if(!std::filesystem::exists(x)) {
                std::cerr << termcolor::red << "Error \n" << termcolor::reset <<
                "\n> Cannot find '" << x << "' in current directory. Check if path is correct.\n";
                return 1;
            }
        }
        if(name=="") {
            std::cerr << termcolor::yellow << "Warning \n" << termcolor::reset <<
            "\n> Name of project is not set.\n";
            name = "project";
        }
        std::cout << termcolor::green << "OK\n\n" << termcolor::reset;

        std::cout << "Starting building " << name << '@' << ver << "...\n";

        std::string cxx = t["cxx"].value_or("g++");
        std::string cxxflags = t["cxxflags"].value_or("-g -O1");
    }
    return 0;
}