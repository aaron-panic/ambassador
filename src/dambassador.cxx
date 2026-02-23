#include <string>
#include <list>
#include <iostream>
#include <zstd.h>

void print_usage() {
    std::cout << "\ndambassador (data ambassador)\n-----\nCreate or Extract damb objects for \"Ambassador\"\n\n";
    std::cout << "Usage:\n    CREATE  -> dambassador\n";
    std::cout << "    EXTRACT -> dambassador -x [FILE].damb\n\n";
    std::cout << "Options:\n-----\n";
    std::cout << "  -m, --manifest [FILE].json       Specify an alternate manifest file other than manifest.json\n";
    std::cout << "  -x, --extract [FILE].damb        Extract a damb file to its components.\n\n";
}

int extract(const std::string& file) {
    std::cout << "Extracting " << file << std::endl;
    return 0;
}

int create(const std::string& file) {
    std::cout << "Creating from " << file << std::endl;
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        print_usage();
        return 0;
    }

    std::list<std::string> args;
    for (size_t i = 1; i < argc; i++) {
        args.push_back(std::string(argv[i]));
    }

    if (args.front() == "-x" || args.front() == "--extract") {
        args.pop_front();
        if (args.size() != 1) {
            print_usage();
            return 1;
        } else {
            return extract(args.front());
        }
    }

    if (args.front() == "-m" || args.front() == "--manifest") {
        args.pop_front();
        if (args.size() == 0) {
            print_usage();
            return 1;
        }
    } else {
        args.push_front("manifest.json");
    }

    return create(args.front());
}