#ifndef OPTION_PARSER_HPP
#define OPTION_PARSER_HPP

#include <string>

// Program options imports
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>

struct AppOptions {
    static constexpr unsigned int DEFAULT_SCR_WIDTH = 800;
    static constexpr unsigned int DEFAULT_SCR_HEIGHT = 600;
    static constexpr double DEFAULT_TICKRATE = 128.0;

    unsigned int scr_width, scr_height;
    double tickrate;
    std::string scene_file;
    bool help;
    bool skip_launcher;

    AppOptions(int argc, const char *argv[]);
    AppOptions(): scr_width(DEFAULT_SCR_WIDTH), scr_height(DEFAULT_SCR_HEIGHT), tickrate(DEFAULT_TICKRATE), scene_file(), help(false) {}

    boost::program_options::options_description getHelpMessage() const;

private:
    boost::program_options::options_description desc;
};

#endif
