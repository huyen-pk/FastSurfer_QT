#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "fastsurfer/core/conform_step_request.h"
#include "fastsurfer/core/conform_step_service.h"
#include "fastsurfer/core/mgh_image.h"

namespace {

std::filesystem::path makeUniqueDirectory(const std::string &name)
{
    const auto root = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    return root;
}

void require(const bool condition, const std::string &message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main()
{
    try {
        const std::filesystem::path repoRoot = FASTSURFER_REPO_ROOT;
        const std::filesystem::path fixturePath = repoRoot / "test/data/Subject140/140_orig.mgz";
        const std::filesystem::path outputDir = makeUniqueDirectory("fastsurfer_core_native_conform_step_test");

        const auto copyPath = outputDir / "copy_orig.mgz";
        const auto conformedPath = outputDir / "conformed_orig.mgz";

        const auto inputImage = fastsurfer::core::MghImage::load(fixturePath);
        require(inputImage.orientationCode().size() == 3 && inputImage.orientationCode().find('?') == std::string::npos,
            "The fixture orientation code could not be derived from the MGZ header: '" + inputImage.orientationCode() + "'.");

        fastsurfer::core::ConformStepRequest request;
        request.inputPath = fixturePath;
        request.copyOrigPath = copyPath;
        request.conformedPath = conformedPath;

        fastsurfer::core::ConformStepService service;
        const auto result = service.run(request);

        require(result.success, "The native conform step did not succeed.");
        require(result.alreadyConformed, "The Subject140 fixture should be treated as already conformed.");
        require(std::filesystem::exists(copyPath), "The native conform step did not write the copy_orig output.");
        require(std::filesystem::exists(conformedPath), "The native conform step did not write the conformed output.");

        const auto outputImage = fastsurfer::core::MghImage::load(conformedPath);

        require(inputImage.header().dimensions == outputImage.header().dimensions,
                "The conformed output dimensions differ from the input fixture.");
        require(inputImage.rawData() == outputImage.rawData(),
                "The conformed output voxel payload differs from the input fixture.");

        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}