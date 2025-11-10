# bd_image_resampler

A JUCE module for high-quality deferred image resampling with optional masking support.

## Overview

`bd_image_resampler` provides efficient, background-threaded image resampling for JUCE applications. It automatically detects display scaling and applies high-quality resampling algorithms only when needed (on 1.0 scale displays), optimizing performance on retina/high-DPI displays.

## Features

- **Deferred Resampling**: Images are resampled asynchronously with a 500ms debounce timer to avoid unnecessary work during window resizing
- **Display-Aware**: Automatically detects display scale and only resamples on 1.0 scale (non-retina) displays
- **Background Processing**: Uses a dedicated thread pool to prevent UI blocking
- **Selective Masking**: Apply high-quality resampling only to masked regions of images
- **Multiple Algorithms**: Supports various resampling algorithms via the `gin_graphics` module (default: Lanczos)

## Dependencies

- `juce_gui_basics`
- `gin_graphics`

## Installation

Add this module to your JUCE project:

1. Copy the `bd_image_resampler` directory to your modules folder
2. Add the module in Projucer or your build system
3. Include the module header in your project:

```cpp
#include <bd_image_resampler/bd_image_resampler.h>
```

## Usage

### Basic Usage

```cpp
class MyComponent : public juce::Component
{
public:
    MyComponent()
        : resampler(*this)
    {
        // Add your images to the resampler
        resampler.images.add(new juce::Image(...));
        resampler.images.add(new juce::Image(...));
    }

    void paint(juce::Graphics& g) override
    {
        // Draw the image at index 0
        resampler.drawImage(g, 0);
    }

private:
    BogrenDigital::ImageResampler::DeferredImageResampler resampler;
};
```

### With Masking

Use masking to apply high-quality resampling only to specific regions:

```cpp
MyComponent()
    : resampler(*this, myMaskImage)
{
    // The mask defines which pixels get resampled
    // White/opaque pixels in the mask = high-quality resampling
    // Transparent pixels = excluded from resampling
}
```

### Direct Resampling Helpers

For immediate, synchronous resampling:

```cpp
using namespace BogrenDigital::ImageResampler;

// Basic resize
juce::Image resized = applyResize(originalImage, 800, 600);

// With masking
juce::Image maskedResize = applyResize(originalImage, maskImage,
                                       800, 600);

// Check if resampling is needed for a rectangle
if (isResamplingNeeded(component.getBounds()))
{
    // Resampling recommended
}
```

## How It Works

1. **Component Lifecycle**: `DeferredImageResampler` attaches as a `ComponentListener` to your component
2. **Resize Detection**: When the component resizes, a 500ms timer starts
3. **Display Check**: After the delay, it checks the display scale factor
4. **Background Resampling**: If scale is 1.0, images are resampled on a background thread
5. **Smart Drawing**: `drawImage()` automatically chooses between:
   - Original image with GPU scaling (retina/high-DPI displays)
   - Pre-resampled image (1.0 scale displays)

## API Reference

### DeferredImageResampler

```cpp
DeferredImageResampler(juce::Component& wrappedComponent,
                       juce::Image maskImage = {})
```

Creates a resampler for the specified component.

```cpp
void drawImage(juce::Graphics& g, int imageIndex)
```

Draws the image at the specified index, automatically choosing the appropriate quality.

```cpp
bool shouldDisplayResampledImages() const
```

Returns true if resampled images should be displayed (resampling is complete and display scale is 1.0).

**Public Members:**
- `juce::OwnedArray<juce::Image> images` - Source images to resample
- `juce::OwnedArray<juce::Image> resampledImages` - Resampled output images
- `juce::Image resamplingMask` - Optional mask for selective resampling
- `std::atomic<bool> isResamplingDone` - Thread-safe resampling status

### ResamplingHelpers

```cpp
juce::Image applyResize(const juce::Image& img,
                        int width, int height,
                        gin::ResizeAlgorirm algorithm = gin::ResizeAlgorirm::lanczos)
```

Synchronously resizes an image using the specified algorithm.

```cpp
juce::Image applyResize(const juce::Image& img,
                        const juce::Image& resamplingMask,
                        int width, int height,
                        gin::ResizeAlgorirm algorithm = gin::ResizeAlgorirm::lanczos)
```

Synchronously resizes an image with selective masking.

```cpp
bool isResamplingNeeded(juce::Rectangle<int> rect)
```

Checks if the display containing the rectangle has a scale of 1.0.

## Performance Considerations

- Resampling is **deferred by 500ms** after resize to avoid redundant processing
- Only **one resampling thread** runs at a time to prevent resource contention
- On high-DPI displays, GPU-accelerated scaling is used instead of CPU resampling
- The thread pool properly cancels in-flight jobs when new resize events occur

## License

MIT License - Copyright (c) 2025 Bogren Digital | FStreet Media

See [LICENSE](LICENSE) for full license text.

## Version

**1.0.0**

## Vendor

Bogren Digital - [https://bogrendigital.com](https://bogrendigital.com)
