#pragma once

namespace juce::Gui
{
    class GainReductionMeter : public juce::Component
    {
    public:
        GainReductionMeter(){
            grMeterImage = juce::ImageCache::getFromMemory(BinaryData::grMeter_png, BinaryData::grMeter_pngSize);
        }
        
        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(2.0f);
            
            // Background colour
            g.setColour(juce::Colours::black);
            g.fillRoundedRectangle(bounds, 15.0f);
            
            // FIXME: Does it represent GR accuretly?
            // Level colour
            g.setGradientFill(gradient);
            // Map level from {0, 20} to {0, width}
            const auto scaledX = juce::jmap(level, 0.0f, 20.0f, 0.0f, static_cast<float>(getWidth()));
            g.fillRoundedRectangle(bounds.removeFromLeft(scaledX), 15.0f);
        }
        
        void paintOverChildren(juce::Graphics& g) override
        {
            g.drawImage(grMeterImage, getLocalBounds().toFloat());
        }
        
        void resized() override
        {
            const auto bounds = getLocalBounds().toFloat();
            
            gradient = juce::ColourGradient{
                juce::Colours::azure,
                bounds.getBottomLeft(),
                juce::Colours::red,
                bounds.getTopRight(),
                false
            };
            gradient.addColour(0.7, juce::Colours::yellow);
        }
        
        void setLevel(const float value) { level = value; }
        
    private:
        float level = 0.0f;
        juce::ColourGradient gradient{};
        juce::Image grMeterImage;
    };
}

