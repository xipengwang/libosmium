/*

  The code in this file is released into the Public Domain.

*/

#include <cstdint>
#include <vector>

#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " INPUT-FILE OUTPUT-FILE\n";
        exit(1);
    }

    std::string input_filename = argv[1];
    std::string output_filename = argv[2];

    std::vector<osmium::memory::Buffer> buffers;

    osmium::io::Reader reader(input_filename);
    while (osmium::memory::Buffer buffer = reader.read()) {
        buffers.push_back(std::move(buffer));
    }
    reader.close();

    osmium::io::File output_file(output_filename, "pbf");
    osmium::io::Header header;
    osmium::io::Writer writer(output_file, header, osmium::io::overwrite::allow);
    for (auto&& buffer : buffers) {
        writer(std::move(buffer));
    }
    writer.close();

}
