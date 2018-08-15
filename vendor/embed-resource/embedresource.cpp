// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>

using namespace std;
using namespace boost::filesystem;

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "USAGE: %s {sym} {rsrc}\n\n"
                        "  Creates {sym}.cpp from the contents of {rsrc}\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    path dst{argv[1]};
    path src{argv[2]};

    string sym = src.filename().string();
    replace(sym.begin(), sym.end(), '.', '_');
    replace(sym.begin(), sym.end(), '-', '_');

    create_directories(dst.parent_path());

    boost::filesystem::ofstream ofs{dst};

    boost::filesystem::ifstream ifs{src, ios::binary};

    ofs << "#include <cstdlib>" << endl;
    ofs << "extern const unsigned char _resource_" << sym << "[] = {" << endl;

    size_t lineCount = 0;
    while (true) {
        char c;
        ifs.get(c);

        if (ifs.eof()) {
            break;
        }

        ofs << "0x" << hex << (c & 0xff) << ", ";
        if (++lineCount == 10) {
            ofs << endl;
            lineCount = 0;
        }
    }


    ofs << "};" << endl;
    ofs << "extern const std::size_t _resource_" << sym << "_len = sizeof(_resource_" << sym << ");";

    return EXIT_SUCCESS;
}
