#pragma once

#include "imaging/mri/fastsurfer/step_conform_request.h"
#include "imaging/mri/fastsurfer/step_conform_result.h"
#include "imaging/mri/fastsurfer/mgh_image.h"

namespace OpenHC::imaging::mri::fastsurfer {

class ConformStepService {
public:
    ConformStepResult run(const ConformStepRequest &request) const;

private:
    static bool isAlreadyConformed(const MghImage &image, const ConformStepRequest &request);
};

} // namespace OpenHC::imaging::mri::fastsurfer