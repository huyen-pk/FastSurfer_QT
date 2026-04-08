#pragma once

#include "fastsurfer/core/conform_step_request.h"
#include "fastsurfer/core/conform_step_result.h"
#include "fastsurfer/core/mgh_image.h"

namespace fastsurfer::core {

class ConformStepService {
public:
    ConformStepResult run(const ConformStepRequest &request) const;

private:
    static bool isAlreadyConformed(const MghImage &image, const ConformStepRequest &request);
};

} // namespace fastsurfer::core