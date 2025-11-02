#include "launcher/optionParser.hpp"

/**
 * Options:
 * --help               Help
 * -w --width           Screen width
 * -h --height          Screen height
 * -t --tickrate        Tickrate
 * -f --input-file      Scene file
 */
AppOptions::AppOptions(int argc, char const **argv): desc("Allowed options") {
    namespace po = boost::program_options;

    desc.add_options()
    ("help", "produce help message")
    ("width,w", po::value<unsigned int>(&(this->scr_width))->default_value(DEFAULT_SCR_WIDTH), "screen width")
    ("height,h", po::value<unsigned int>(&(this->scr_height))->default_value(DEFAULT_SCR_HEIGHT), "screen height")
    ("tickrate,t", po::value<double>(&(this->tickrate))->default_value(DEFAULT_TICKRATE), "animation tickrate")
    ("input-file,f", po::value<std::string>(&(this->scene_file))->default_value(""), "scene file to load");

    po::positional_options_description p;
    p.add("input-file", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);

    this->help = vm.count("help") > 0;
}

boost::program_options::options_description AppOptions::getHelpMessage() const {
    return desc;
}