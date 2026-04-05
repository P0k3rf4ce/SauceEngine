#ifndef OPTION_PARSER_HPP
#define OPTION_PARSER_HPP

#include <array>
#include <string>

// Program options imports
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>

struct AppOptions {
    static constexpr unsigned int DEFAULT_SCR_WIDTH = 1280;
    static constexpr unsigned int DEFAULT_SCR_HEIGHT = 720;
    static constexpr double DEFAULT_TICKRATE = 128.0;
    /// Default post-import rotation for standalone glTF/GLB: matches the fixed environment direction remap
    /// used when sampling IBL and drawing the skybox (see shader_pbr.slang / skybox.slang).
    static constexpr double DEFAULT_MODEL_ROTATE_X_DEGREES = 90.0;
    static constexpr double DEFAULT_MODEL_ROTATE_Y_DEGREES = 0.0;
    static constexpr double DEFAULT_MODEL_ROTATE_Z_DEGREES = 0.0;
    static constexpr const char* DEFAULT_MODEL_ROTATION_AXIS = "z";

    unsigned int scr_width, scr_height;
    double tickrate;
    double model_rotate_x_degrees;
    double model_rotate_y_degrees;
    double model_rotate_z_degrees;
    std::string scene_file;
    std::string ibl_file;
    std::string polyhaven_model_id;
    std::string polyhaven_model_resolution;
    std::string polyhaven_hdri_id;
    std::string polyhaven_hdri_resolution;
    bool model_rotation_provided;
    bool skip_launcher;
    bool help;

    std::array<double, 3> defaultModelRotationDegrees() const {
        return {DEFAULT_MODEL_ROTATE_X_DEGREES, DEFAULT_MODEL_ROTATE_Y_DEGREES, DEFAULT_MODEL_ROTATE_Z_DEGREES};
    }

    AppOptions(int argc, const char *argv[]);
    AppOptions()
        : scr_width(DEFAULT_SCR_WIDTH),
          scr_height(DEFAULT_SCR_HEIGHT),
          tickrate(DEFAULT_TICKRATE),
          model_rotate_x_degrees(DEFAULT_MODEL_ROTATE_X_DEGREES),
          model_rotate_y_degrees(DEFAULT_MODEL_ROTATE_Y_DEGREES),
          model_rotate_z_degrees(DEFAULT_MODEL_ROTATE_Z_DEGREES),
          scene_file(),
          ibl_file(),
          polyhaven_model_id(),
          polyhaven_model_resolution("2k"),
          polyhaven_hdri_id(),
          polyhaven_hdri_resolution("4k"),
          model_rotation_provided(false),
          skip_launcher(false),
          help(false),
          desc("Allowed options") {}

    boost::program_options::options_description getHelpMessage() const;

private:
    boost::program_options::options_description desc;
};

#endif
