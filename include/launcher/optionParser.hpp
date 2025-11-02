#ifndef OPTION_PARSER_HPP
#define OPTION_PARSER_HPP

#include <string>

// Program options imports
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>

struct AppOptions {
    const unsigned int DEFAULT_SCR_WIDTH = 800;
    const unsigned int DEFAULT_SCR_HEIGHT = 600;
    const double DEFAULT_TICKRATE = 128.0;

    unsigned int scr_width, scr_height;
    double tickrate;
    std::string scene_file;
    bool help;

    AppOptions(int argc, const char *argv[]);
    boost::program_options::options_description getHelpMessage() const;

private:
    boost::program_options::options_description desc;
};

#endif