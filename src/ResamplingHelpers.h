#pragma once

namespace BogrenDigital::ImageResampler
{
    static inline juce::Image applyResize (const juce::Image& img, const juce::Image& resamplingMask, int width, int height, gin::ResizeAlgorirm resizeAlgorirm = gin::ResizeAlgorirm::lanczos)
    {
        if (width <= 0 || height <= 0 || img.isNull())
        {
            return juce::Image();
        }

        if (resamplingMask.isNull())
        {
            return gin::applyResize (img, width, height, resizeAlgorirm);
        }
        
        juce::Image highQualResampledImage = gin::applyResize (img, width, height, resizeAlgorirm);
        // juce::Image lowQualResampledImage = img.rescaled (width, height, juce::Graphics::ResamplingQuality::highResamplingQuality);

        // Get scaled down version of mask without resampling
        juce::Image scaledDownMask = resamplingMask.rescaled (width, height, juce::Graphics::ResamplingQuality::lowResamplingQuality);
        juce::Image maskedImage = juce::Image (juce::Image::ARGB,  highQualResampledImage.getWidth(),  highQualResampledImage.getHeight(), true);
        maskedImage.clear(maskedImage.getBounds(), juce::Colours::transparentBlack);

        // Loop over the pixels of the resampled image and apply the mask
        for (int y = 0; y <  highQualResampledImage.getHeight(); ++y)
        {
            for (int x = 0; x <  highQualResampledImage.getWidth(); ++x)
            {
                // If the mask pixel is not transparent, copy the pixel from the resampled image
                if (scaledDownMask.getPixelAt (x, y).getAlpha() > 0)
                {
                    maskedImage.setPixelAt (x, y,  highQualResampledImage.getPixelAt (x, y));
                }
                // else
                // {
                //     // If the mask pixel is transparent, set the pixel in the masked image to the low-quality resampled pixel
                //     maskedImage.setPixelAt (x, y, lowQualResampledImage.getPixelAt (x, y));
                // }
            }
        }
        return maskedImage;
    }

    static inline juce::Image applyResize (const juce::Image& img, int width, int height, gin::ResizeAlgorirm resizeAlgorirm = gin::ResizeAlgorirm::lanczos)
    {
        if (width <= 0 || height <= 0 || img.isNull())
        {
            return juce::Image();
        }

        return gin::applyResize (img, width, height, resizeAlgorirm);
    }

    static inline bool isResamplingNeeded (juce::Rectangle<int> rect)
    {
        return juce::Desktop::getInstance().getDisplays().getDisplayForRect(rect)->scale == 1.0f;
    }
}