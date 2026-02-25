#include "dambassador.hxx"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace amb::damb {
    int runDambassador(const std::string& command, const std::filesystem::path& input_path) {
        amb::Dambassador dambassador;

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
    }
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            amb::Dambassador::printUsage(std::cout);
            return 1;
        }

        return amb::damb::runDambassador(argv[1], argv[2]);
    } catch (const std::exception& ex) {
        std::cerr << "dambassador error: " << ex.what() << '\n';
        return 1;
    }
}
