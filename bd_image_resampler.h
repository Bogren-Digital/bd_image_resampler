/** BEGIN_JUCE_MODULE_DECLARATION
ID:               bd_image_resampler
vendor:           Bogren Digital
version:          1.0.0
name:             Image Resampler
description:      A module for resampling images using various algorithms.
website:          https://bogrendigital.com
license:          MIT
dependencies:     gin_graphics, juce_gui_basics
END_JUCE_MODULE_DECLARATION
*/
#pragma once
#define BD_IMAGE_RESAMPLER_H_INCLUDED

#include <gin_graphics/gin_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "src/ResamplingHelpers.h"

#include "src/DeferredImageResampler.h"