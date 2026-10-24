#include "ToyTuneLookAndFeel.h"

namespace toytune
{

const juce::Colour ToyTuneLookAndFeel::backgroundDark   = juce::Colour(0xff0d0d0f);
const juce::Colour ToyTuneLookAndFeel::panelBackground  = juce::Colour(0xff141417);
const juce::Colour ToyTuneLookAndFeel::panelBorder      = juce::Colour(0xff242429);
const juce::Colour ToyTuneLookAndFeel::knobDark         = juce::Colour(0xff18181c);
const juce::Colour ToyTuneLookAndFeel::knobBezel        = juce::Colour(0xff2c2c34);
const juce::Colour ToyTuneLookAndFeel::knobPointer      = juce::Colour(0xfff0f0f4);
const juce::Colour ToyTuneLookAndFeel::accentWhite      = juce::Colour(0xffffffff);
const juce::Colour ToyTuneLookAndFeel::accentSilver     = juce::Colour(0xffc5c5cb);
const juce::Colour ToyTuneLookAndFeel::textMuted        = juce::Colour(0xff7a7a85);
const juce::Colour ToyTuneLookAndFeel::activeIndicator  = juce::Colour(0xffe2e2e8);

ToyTuneLookAndFeel::ToyTuneLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, backgroundDark);
    setColour(juce::ComboBox::backgroundColourId, panelBackground);
    setColour(juce::ComboBox::textColourId, accentWhite);
    setColour(juce::ComboBox::outlineColourId, panelBorder);
    setColour(juce::ComboBox::arrowColourId, accentSilver);
    setColour(juce::PopupMenu::backgroundColourId, panelBackground);
    setColour(juce::PopupMenu::textColourId, accentWhite);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff222228));
    setColour(juce::PopupMenu::highlightedTextColourId, accentWhite);
    setColour(juce::TextButton::buttonColourId, panelBackground);
    setColour(juce::TextButton::textColourOffId, accentSilver);
    setColour(juce::TextButton::textColourOnId, accentWhite);
}

void ToyTuneLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle, float rotaryEndAngle,
                                          juce::Slider& slider)
{
    const bool isSliderEnabled = slider.isEnabled();
    const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centreX = bounds.getCentreX();
    const auto centreY = bounds.getCentreY();

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // 1. Outer Track Ring Background
    juce::Path trackBg;
    trackBg.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                          0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(isSliderEnabled ? juce::Colour(0xff1f1f24) : juce::Colour(0xff141418));
    g.strokePath(trackBg, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 2. Active Track Value Arc (illuminated only when slider is active)
    if (isSliderEnabled)
    {
        juce::Path trackActive;
        trackActive.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                                  0.0f, rotaryStartAngle, angle, true);
        g.setColour(accentSilver);
        g.strokePath(trackActive, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 3. Drop Shadow for 3D depth
    const auto knobRadius = radius - 8.0f;
    g.setColour(isSliderEnabled ? juce::Colour(0x60000000) : juce::Colour(0x25000000));
    g.fillEllipse(centreX - knobRadius + 1.0f, centreY - knobRadius + 2.0f, knobRadius * 2.0f, knobRadius * 2.0f);

    // 4. Outer Bezel
    if (isSliderEnabled)
    {
        juce::ColourGradient bezelGrad(juce::Colour(0xff363640), centreX - knobRadius, centreY - knobRadius,
                                       juce::Colour(0xff16161a), centreX + knobRadius, centreY + knobRadius, false);
        g.setGradientFill(bezelGrad);
    }
    else
    {
        juce::ColourGradient bezelGrad(juce::Colour(0xff202026), centreX - knobRadius, centreY - knobRadius,
                                       juce::Colour(0xff111114), centreX + knobRadius, centreY + knobRadius, false);
        g.setGradientFill(bezelGrad);
    }
    g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

    // 5. Knob Face (Metallic brushed finish)
    const auto innerRadius = knobRadius - 2.5f;
    if (isSliderEnabled)
    {
        juce::ColourGradient faceGrad(juce::Colour(0xff222228), centreX, centreY - innerRadius,
                                      juce::Colour(0xff121215), centreX, centreY + innerRadius, false);
        g.setGradientFill(faceGrad);
    }
    else
    {
        juce::ColourGradient faceGrad(juce::Colour(0xff151518), centreX, centreY - innerRadius,
                                      juce::Colour(0xff0c0c0e), centreX, centreY + innerRadius, false);
        g.setGradientFill(faceGrad);
    }
    g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

    // 6. Subtle inner rim outline
    g.setColour(isSliderEnabled ? juce::Colour(0x30ffffff) : juce::Colour(0x10ffffff));
    g.drawEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.0f);

    // 7. Metallic Indicator Needle / Pointer
    juce::Path p;
    const float pointerLength = innerRadius * 0.75f;
    const float pointerThickness = 2.2f;
    p.addRoundedRectangle(-pointerThickness * 0.5f, -innerRadius + 3.0f, pointerThickness, pointerLength, 1.0f);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(isSliderEnabled ? knobPointer : juce::Colour(0xff454854));
    g.fillPath(p);

    // Center jewel pin
    g.setColour(isSliderEnabled ? juce::Colour(0xff0a0a0c) : juce::Colour(0xff070709));
    g.fillEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
    g.setColour(isSliderEnabled ? juce::Colour(0x60ffffff) : juce::Colour(0x20ffffff));
    g.drawEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f, 1.0f);
}

void ToyTuneLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                      int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                      juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);
    const float radius = bounds.getHeight() * 0.5f;

    // Dark capsule background
    g.setColour(juce::Colour(0xff141518));
    g.fillRoundedRectangle(bounds, radius);

    g.setColour(juce::Colour(0xff272932));
    g.drawRoundedRectangle(bounds, radius, 1.2f);

    // Downward chevron arrow
    const float arrowX = static_cast<float>(width) - 18.0f;
    const float arrowY = static_cast<float>(height) * 0.5f - 1.5f;

    juce::Path chevron;
    chevron.startNewSubPath(arrowX, arrowY);
    chevron.lineTo(arrowX + 4.5f, arrowY + 4.5f);
    chevron.lineTo(arrowX + 9.0f, arrowY);
    g.setColour(box.isEnabled() ? juce::Colour(0xffc5c8d4) : juce::Colour(0xff555865));
    g.strokePath(chevron, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ToyTuneLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted, bool /*shouldDrawButtonAsDown*/)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const bool isTicked = button.getToggleState();

    const auto boxBounds = juce::Rectangle<float>(bounds.getX() + 2.0f, bounds.getCentreY() - 7.0f, 14.0f, 14.0f);

    g.setColour(isTicked ? juce::Colour(0xff2b2b34) : juce::Colour(0xff121215));
    g.fillRoundedRectangle(boxBounds, 3.0f);

    g.setColour(isTicked ? accentWhite : panelBorder);
    g.drawRoundedRectangle(boxBounds, 3.0f, 1.0f);

    if (isTicked)
    {
        g.setColour(accentWhite);
        const auto inner = boxBounds.reduced(3.5f);
        g.fillRoundedRectangle(inner, 1.5f);
    }

    g.setColour(button.isEnabled() ? accentWhite : textMuted);
    g.setFont(juce::Font(12.0f, juce::Font::plain));
    g.drawFittedText(button.getButtonText(),
                     static_cast<int>(boxBounds.getRight() + 6.0f),
                     0,
                     static_cast<int>(bounds.getWidth() - boxBounds.getRight() - 6.0f),
                     static_cast<int>(bounds.getHeight()),
                     juce::Justification::centredLeft,
                     1);
}

void ToyTuneLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& /*backgroundColour*/,
                                             bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

    juce::Colour base = panelBackground;
    if (button.getToggleState())
        base = juce::Colour(0xff2a2a32);
    else if (shouldDrawButtonAsDown)
        base = juce::Colour(0xff0e0e11);
    else if (shouldDrawButtonAsHighlighted)
        base = juce::Colour(0xff1c1c22);

    g.setColour(base);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(button.getToggleState() ? accentWhite : panelBorder);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void ToyTuneLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(panelBackground);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(panelBorder);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void ToyTuneLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                                          bool /*hasSubMenu*/, const juce::String& text,
                                          const juce::String& /*shortcutKeyText*/,
                                          const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isSeparator)
    {
        g.setColour(panelBorder);
        g.fillRect(area.getX() + 6, area.getCentreY(), area.getWidth() - 12, 1);
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour(juce::Colour(0xff22222a));
        g.fillRect(area);
    }

    g.setColour(isActive ? (isTicked ? accentWhite : accentSilver) : textMuted);
    g.setFont(juce::Font(12.5f, isTicked ? juce::Font::bold : juce::Font::plain));

    auto textArea = area.reduced(10, 0);
    g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);
}

} // namespace toytune
