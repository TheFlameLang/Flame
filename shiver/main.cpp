#include "toml.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include "../include/third-party/termcolor.hpp""


int main(int argc, char* argv[]) {
    bool init = false;
    bool skip_init = false;
    bool build = false;
    bool verbose = false;
    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i], "init")==0) {
            init = true;
        } else if(strcmp(argv[i], "skip")==0&&init) {
            skip_init = true;
        } else if(strcmp(argv[i], "build")==0) {
            build = true;
        } else if(strcmp(argv[i], "verbose")==0) {
            verbose = true;
        } else if(strcmp(argv[i], "version")==0) {
            std::cout << "Shiver 1.0 by Naharashu\n"
            << "MIT License\n"
            << "Made with love\n";
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
            f << "license = \"MIT\"\n";
            f << "author = \"cool guy\"\n";
            f << "github = \"https://github.com/\"\n";

            f << "[build]\n";
            f << "output = \"example\"\n";
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
            f << "\n";
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
            std::cout << termcolor::blue << "Output file name: " << termcolor::reset; std::getline(std::cin >> std::ws, temp);
            f << "output = \"" << temp << "\"\n";
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
        std::cout << "Running basic check...\n";
        std::vector<std::string> src;
        if (toml::array* arr = t["build"]["src"].as_array()) {
            arr->for_each([&src](auto&& e){
                src.emplace_back(e.value_or(""));
            });
        }
        bool error = false;
		std::vector<std::string> files;
        for(auto &x : src) {
            for(auto &y : std::filesystem::recursive_directory_iterator(x)) {
                if(y.path().extension() == ".flame") {
                    const std::string n = y.path().stem();
                    files.emplace_back(n+"_flame");
                }
            }
        }
        if(files.empty()) {
            std::cerr << termcolor::red << "Error: " << termcolor::reset <<
            "No files to build detected, stopping now.\n";
            return 1;
        }
        if(name=="") {
            std::cerr << termcolor::yellow << "Warning \n" << termcolor::reset <<
            "\n> Name of project is not set.\n";
            name = "project";
        }
        if(ver=="") {
            std::cerr << termcolor::yellow << "Warning \n" << termcolor::reset <<
            "\n> Version of project is not set.\n";
            ver = "1.0.0";
        }
        std::cout << "Starting building " << name << '@' << ver << "...\n";
        auto start = std::chrono::high_resolution_clock::now();
        std::filesystem::create_directory("./build");
        for(auto &x : src) {
            for(auto &y : std::filesystem::recursive_directory_iterator(x)) {
                if(y.path().extension() == ".flame") {
                    const std::string n = y.path().stem();
                    const std::string name = y.path().filename();
                    std::string cmd_ = "flame -C "; cmd_ += x + '/' + name; cmd_ += " -o ./build/" + n;
                    std::cout << "Compiling " << termcolor::bright_yellow << x << '/' <<  name << termcolor::reset
                    << " into C++...\n";
                    if(verbose) std::cout << termcolor::blue << "[VERBOSE]: " << termcolor::reset << cmd_ << '\n';
                    int ret = std::system(cmd_.c_str());
                    if(ret!=0) {
                        error = true;
                    }
                }
            }
        }


        std::string cxx = t["build"]["cxx"].value_or("g++");
        std::string cxxflags = t["build"]["cxxflags"].value_or("-g -O1");
        std::string output = t["build"]["output"].value_or("test");

  		
        std::string final = "";
        for(auto &x : files) {
            if(error) break;
            final += "./build/" + x + ".o";
            std::ostringstream cmd;
            cmd << cxx << " -c " << cxxflags << " ./build/" << x << ".cpp -o ./build/" << x << ".o";
            std::cout << "Compiling " << termcolor::bright_yellow << x << termcolor::reset
            << " into object file...\n";
            if(verbose) std::cout << termcolor::blue << "[VERBOSE]: " << termcolor::reset << cmd.str() << '\n';
            std::system(cmd.str().c_str());
        }
        std::ostringstream cmd;
        cmd << cxx << ' ' << final << " -o " << output;
        if(verbose) std::cout << termcolor::blue << "[VERBOSE]: " << termcolor::reset << cmd.str() << '\n';
        if(!error) std::cout << "Linking files together...";
        else {
            std::cout << "Build is unsuccessful...\n";
            return 1;
        }
        std::system(cmd.str().c_str());
        // very long but its just (end-start)->to millisec-> to double
        double time_end = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count());
        std::cout << termcolor::green << "Done\n" << termcolor::reset;
        std::cout << "Buid successful (" << time_end/1000 << "s)\n";
    }
    return 0;
}
