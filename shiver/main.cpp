#include "toml.hpp"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
    bool init = false;
    bool skip_init = false;
    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i], "init")==0) {
            init = true;
        } else if(strcmp(argv[i], "skip")==0&&init) {
            skip_init = true;
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
        } else {
            std::ofstream f("project.toml");
            std::string temp = "";
            std::cout << "Project name: "; std::cin >> temp;
            f << "name = \"" + temp + "\"\n";
            std::cout << "Project version: "; std::cin >> temp;
            f << "version = \"" + temp + "\"\n";
            std::cout << "Project output file name: "; std::cin >> temp;
            f << "\n";
            f << "output = \"" + temp + "\"\n";
            std::cout << "Project license[MIT, Apache2, GPL2, GPL3]: "; std::cin >> temp;
            f << "license = \"" + temp + "\"\n";
            std::cout << "Project author: "; std::cin >> temp;
            f << "author = \"" + temp + "\"\n";
            std::cout << "Project github: "; std::cin >> temp;
            f << "github = \"" + temp + "\"\n";
        }
        return 0;
    }
    return 0;
}