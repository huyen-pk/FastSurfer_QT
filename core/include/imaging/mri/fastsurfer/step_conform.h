#pragma once

#include "imaging/mri/fastsurfer/step_conform_request.h"
#include "imaging/mri/fastsurfer/step_conform_result.h"
#include "imaging/common/mgh_image.h"

namespace OpenHC::imaging::mri::fastsurfer {

// Executes the native FastSurfer-style conform step on one input volume.
class ConformStepService {
public:
    // Runs the conform step, optionally copies the original image, and writes
    // the resulting conformed MGZ output.
    // Parameters:
    // - request: Input, output, and policy settings for one conform execution.
    // Returns metadata describing the produced outputs.
    ConformStepResult run(const ConformStepRequest &request) const;

private:
    // Returns true when the source image already satisfies the requested native
    // conform contract.
    // Parameters:
    // - image: Candidate image to validate.
    // - request: Conform policy that defines the target contract.
    static bool isAlreadyConformed(const MghImage &image, const ConformStepRequest &request);
};

} // namespace OpenHC::imaging::mri::fastsurfer