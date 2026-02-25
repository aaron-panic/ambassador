#include "dambassador.hxx"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    try {
        amb::Dambassador dambassador;

        if (argc != 3) {
            amb::Dambassador::printUsage(std::cout);
            return 1;
        }

        const std::string command = argv[1];
        const std::filesystem::path input_path = argv[2];

        if (command == "-c") {
            dambassador.create(input_path);
            return 0;
        }

        if (command == "-x") {
            dambassador.extract(input_path);
            return 0;
        }

        if (command == "-i") {
            dambassador.inspect(input_path);
            return 0;
        }

        amb::Dambassador::printUsage(std::cout);
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "dambassador error: " << ex.what() << '\n';
        return 1;
    }
}
