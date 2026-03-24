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
    namespace style = boost::program_options::command_line_style;

    desc.add_options()
    ("help", "produce help message")
    ("skip-launcher", "start the engine immediately")
    ("width,w", po::value<unsigned int>(&(this->scr_width))->default_value(DEFAULT_SCR_WIDTH), "screen width")
    ("height,h", po::value<unsigned int>(&(this->scr_height))->default_value(DEFAULT_SCR_HEIGHT), "screen height")
    ("tickrate,t", po::value<double>(&(this->tickrate))->default_value(DEFAULT_TICKRATE), "animation tickrate")
    ("input-file,f", po::value<std::string>(&(this->scene_file))->default_value(""), "scene file to load")
    ("camera_collide", po::bool_switch(&(this->camera_collide)), "enable camera collision/pushing against dynamic objects");

    po::positional_options_description p;
    p.add("input-file", 1);

    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv)
            .options(desc)
            .positional(p)
            .style(style::default_style | style::allow_long_disguise)
            .run(),
        vm);
    po::notify(vm);

    this->help = vm.count("help") > 0;
}

boost::program_options::options_description AppOptions::getHelpMessage() const {
    return desc;
}
