#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

// namespace bi = boost::interprocess;
// namespace bf = boost::filesystem;
// using bf::file_size;
// using bi::copy_on_write;
// using bi::file_mapping;
// using bi::mapped_region;
// using bi::read_only;

// auto file_name = argv[1];
// auto size = file_size(file_name);
// file_mapping file(file_name, read_only);
// mapped_region region(file, read_only, 0, size);
// JsonDoc d;
// auto text = static_cast<const char*>(region.get_address());
// rapidjson::ParseResult status = d.Parse(text, size);
// if (!status) {
//    errstr(std::cout, status, text);
// }

// boost::filesystem::path basepath(argv[2]);
// std::initializer_list<const char*> dirs{
//    "buy", "issued", "price", "volume", "duration", "id", "minVolume", "volumeEntered", "range", "stationID",
//    //"type"
//};
// create_directories(basepath, dirs);
