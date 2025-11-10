namespace BogrenDigital::ImageResampler
{
    class DeferredImageResampler : public juce::ComponentListener, private juce::Timer
    {
    public:
        DeferredImageResampler(juce::Component& wrappedComponent, juce::Image maskImage = {})
            : resamplingMask(std::move(maskImage))
            , component(wrappedComponent)
        {
            component.addComponentListener(this);

            auto weakThis = juce::WeakReference<DeferredImageResampler>(this);
            juce::MessageManager::getInstance()->callAsync([weakThis]()
            {
                if (auto* strongThis = weakThis.get())
                {
                    strongThis->handleResampling();
                }
            });
        }

        ~DeferredImageResampler() override
        {
            component.removeComponentListener(this);
        }

        bool shouldDisplayResampledImages() const
        {
            const auto* display = getComponentDisplay();
            const bool resamplingNeeded = display != nullptr ? juce::approximatelyEqual(display->scale, 1.0) : false;
            bool shouldDisplay = isResamplingDone && resamplingNeeded;
            
            // juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName()
            //     + "' shouldDisplayResampledImages: " + (shouldDisplay ? "YES" : "NO")
            //     + " (resamplingDone: " + (isResamplingDone ? "YES" : "NO")
            //     + ", resamplingNeeded: " + (resamplingNeeded ? "YES" : "NO")
            //     + ", display is main: " + (display != nullptr ? (display->isMain ? "YES" : "NO") : "nullptr")
            //     + ", display scale: " + (display != nullptr ? juce::String(display->scale) : "nullptr")
            //     + ")");
            return shouldDisplay;
        }

        void drawImage(juce::Graphics& g, int imageIndex)
        {
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

            if (imageIndex < 0 || imageIndex >= images.size())
            {
                juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName()
                    + "' drawImage - Invalid image index: " + juce::String(imageIndex)
                    + " (total images: " + juce::String(images.size()) + ")");
                g.fillAll(juce::Colours::darkgrey.withAlpha(0.3f));
                g.setColour(juce::Colours::white);
                g.drawText("Invalid Image Index", component.getLocalBounds(),
                        juce::Justification::centred, true);
                return;
            }

            if(shouldDisplayResampledImages())
            {
                // juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName()
                //     + "' drawing RESAMPLED image #" + juce::String(imageIndex));

                const auto resampledImage = resampledImages[imageIndex];
                if (resampledImage == nullptr || !resampledImage->isValid())
                {
                    juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName()
                        + "' resampled image #" + juce::String(imageIndex) + " is invalid!");
                    g.fillAll(juce::Colours::darkgrey.withAlpha(0.3f));
                    g.setColour(juce::Colours::white);
                    g.drawText("Resampling Failed", component.getLocalBounds(),
                                juce::Justification::centred, true);
                    return;
                }
                g.drawImageAt(*resampledImage, 0, 0);
            }
            else
            {
                // juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName()
                //     + "' drawing ORIGINAL image #" + juce::String(imageIndex) + " with scaling");
                const auto floatRect = getFloatRect();
                g.drawImage(*images[imageIndex], floatRect, juce::RectanglePlacement::stretchToFit);
            }
        }

    protected:
        juce::OwnedArray<juce::Image> images;
        juce::OwnedArray<juce::Image> resampledImages;
        juce::Image resamplingMask = {};
        std::atomic<bool> isResamplingDone = false;
        constexpr static auto delayTimeMs = 500;

    private:
        juce::Component& component;

        juce::Rectangle<float> getFloatRect() const
        {
            const auto& props = component.getProperties();
            if (props.contains("floatX") && props.contains("floatY") &&
                props.contains("floatW") && props.contains("floatH"))
            {
                return juce::Rectangle<float>(
                    static_cast<float>(static_cast<double>(props["floatX"])),
                    static_cast<float>(static_cast<double>(props["floatY"])),
                    static_cast<float>(static_cast<double>(props["floatW"])),
                    static_cast<float>(static_cast<double>(props["floatH"])));
            }
            return component.getLocalBounds().toFloat();
        }

        const juce::Displays::Display* getComponentDisplay() const
        {
            auto& displays = juce::Desktop::getInstance().getDisplays();
            
            // Strategy 1: Try component's screen bounds if showing
            if (component.isShowing() && component.getScreenBounds().getWidth() > 0)
            {
                // First try the center point
                auto center = component.getScreenBounds().getCentre();
                if (const auto* display = displays.getDisplayForPoint(center))
                {
                    // juce::Logger::writeToLog("DeferredImageResampler: Display detected using center point method");
                    return display;
                }
                
                // Then try the full bounds
                if (const auto* display = displays.getDisplayForRect(component.getScreenBounds()))
                {
                    // juce::Logger::writeToLog("DeferredImageResampler: Display detected using screen bounds method");
                    return display;
                }
            }
            
            // Strategy 2: Try using the parent monitor area
            auto monitorArea = component.getParentMonitorArea();
            if (!monitorArea.isEmpty())
            {
                if (const auto* display = displays.getDisplayForRect(monitorArea))
                {
                    // juce::Logger::writeToLog("DeferredImageResampler: Display detected using parent monitor area method");
                    return display;
                }
            }
            
            // Strategy 3: Try using the component's top-level window
            if (auto* topLevel = component.getTopLevelComponent())
            {
                if (topLevel->isOnDesktop() && topLevel->getScreenBounds().getWidth() > 0)
                {
                    auto topLevelCenter = topLevel->getScreenBounds().getCentre();
                    if (const auto* display = displays.getDisplayForPoint(topLevelCenter))
                    {
                        // juce::Logger::writeToLog("DeferredImageResampler: Display detected using top-level window method");
                        return display;
                    }
                }
            }
            
            juce::Logger::writeToLog("DeferredImageResampler: Failed to detect display!");
            return nullptr;
        }

        struct ImageResamplingThreadPool
        {
        public:
            ImageResamplingThreadPool() = default;
            ~ImageResamplingThreadPool()
            {
                threadPool.removeAllJobs(true, delayTimeMs);
            }
            juce::ThreadPool threadPool { juce::ThreadPoolOptions{}
                .withThreadName ("Image Resampling Thread")
                .withNumberOfThreads (1) };
        };

        juce::SharedResourcePointer<ImageResamplingThreadPool> threadPool;

        class ResamplingJob : public juce::ThreadPoolJob
        {
        public:
            ResamplingJob(DeferredImageResampler& r)
                : juce::ThreadPoolJob("Image Resampling job"),
                weakResampler(&r)
            {
            }

            ~ResamplingJob() override = default;

        private:
            juce::ThreadPoolJob::JobStatus runJob() override
            {
                if (shouldExit())
                    return juce::ThreadPoolJob::jobHasFinished;

                if (auto* strongThis = weakResampler.get())
                {
                    const auto timeNowMs = juce::Time::getCurrentTime().toMilliseconds();
                    juce::Logger::writeToLog("DeferredImageResampler: '" + strongThis->component.getName() 
                        + "' starting resampling job for " + juce::String(strongThis->images.size()) + " images to size " 
                        + juce::String(strongThis->component.getWidth()) + "x" + juce::String(strongThis->component.getHeight()));
                        
                    strongThis->isResamplingDone = false;
                    strongThis->resampledImages.clear();

                    for (int i = 0; i < strongThis->images.size(); ++i)
                    {
                        auto* image = strongThis->images[i];
                        if (shouldExit())
                            return juce::ThreadPoolJob::jobHasFinished;

                        juce::Logger::writeToLog("DeferredImageResampler: '" + strongThis->component.getName() 
                            + "' resampling image #" + juce::String(i) + " from " 
                            + juce::String(image->getWidth()) + "x" + juce::String(image->getHeight()));

                        auto resampledImage = BogrenDigital::ImageResampler::applyResize(
                            *image,
                            strongThis->resamplingMask,
                            strongThis->component.getWidth(),
                            strongThis->component.getHeight());

                        strongThis->resampledImages.add(new juce::Image(resampledImage));
                    }

                    strongThis->isResamplingDone = true;

                    auto repaintWeakRef = juce::WeakReference<DeferredImageResampler>(strongThis);
                    juce::MessageManager::getInstance()->callAsync([repaintWeakRef]()
                    {
                        if (auto* repaintStrongRef = repaintWeakRef.get())
                        {
                            juce::Logger::writeToLog("DeferredImageResampler: '" + repaintStrongRef->component.getName() 
                                + "' resampling complete, triggering repaint");
                            repaintStrongRef->component.repaint();
                        }
                    });

                    auto timeTakenMs = juce::Time::getCurrentTime().toMilliseconds() - timeNowMs;
                    juce::Logger::writeToLog("DeferredImageResampler: '" + strongThis->component.getName() 
                        + "' resampling job completed in " + juce::String(timeTakenMs) + "ms");
                }

                return juce::ThreadPoolJob::jobHasFinished;
            }

            juce::WeakReference<DeferredImageResampler> weakResampler;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResamplingJob)
        };

        void handleResampling()
        {
            if (component.getWidth() <= 0 || component.getHeight() <= 0)
            {
                juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName() 
                    + "' handleResampling skipped - size: " + juce::String(component.getWidth()) + "x" + juce::String(component.getHeight()) );
                return;
            }
            
            // More robust display detection
            const auto* display = getComponentDisplay();
            const bool resamplingNeeded = display != nullptr ? juce::approximatelyEqual(display->scale, 1.0) : false;
            const auto displayScale = display != nullptr ? display->scale : 1.0f;
            
            juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName() 
                + "' handleResampling - display scale: " + juce::String(displayScale, 2)
                + ", resamplingNeeded: " + (resamplingNeeded ? "YES" : "NO"));
            
            if (resamplingNeeded)
            {
                juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName() 
                    + "' scheduling resampling job");
                threadPool->threadPool.addJob(new ResamplingJob(*this), true);
            }
        }

        void componentMovedOrResized(juce::Component& c, bool wasMoved, bool wasResized) override
        {
            juce::ignoreUnused(c, wasMoved);

            if (!wasResized)
            {
                // juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName() 
                //     + "' component moved but not resized, ignoring");
                return;
            }
            
            juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName() 
                + "' component resized to " + juce::String(component.getWidth()) + "x" + juce::String(component.getHeight())
                + ", cancelling existing jobs and starting " + juce::String(delayTimeMs) + "ms timer");
            
            isResamplingDone = false;
            threadPool->threadPool.removeAllJobs(true, delayTimeMs);
            startTimer(delayTimeMs);
        }

        void timerCallback() override
        {
            juce::Logger::writeToLog("DeferredImageResampler: '" + component.getName() 
                + "' timer fired after " + juce::String(delayTimeMs) + "ms delay, triggering resampling");
            stopTimer();
            handleResampling();
        }

        friend class ResamplingJob;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeferredImageResampler)
        JUCE_DECLARE_WEAK_REFERENCEABLE(DeferredImageResampler)
    };
}