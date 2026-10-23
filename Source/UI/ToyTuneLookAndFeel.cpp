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
                                          juce::Slider& /*slider*/)
{
    const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centreX = bounds.getCentreX();
    const auto centreY = bounds.getCentreY();

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // 1. Outer Track Ring Background
    juce::Path trackBg;
    trackBg.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                          0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff1f2128));
    g.strokePath(trackBg, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 2. Active Track Value Arc with Optical Neon Bloom
    juce::Path trackActive;
    trackActive.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                              0.0f, rotaryStartAngle, angle, true);
    
    // Outer bloom
    g.setColour(juce::Colour(0x35ffffff));
    g.strokePath(trackActive, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    // Core arc
    g.setColour(accentWhite);
    g.strokePath(trackActive, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 3. Drop Shadow for 3D depth
    const auto knobRadius = radius - 8.0f;
    g.setColour(juce::Colour(0x80000000));
    g.fillEllipse(centreX - knobRadius + 1.0f, centreY - knobRadius + 2.5f, knobRadius * 2.0f, knobRadius * 2.0f);

    // 4. Outer CNC Knurled Bezel
    juce::ColourGradient bezelGrad(juce::Colour(0xff363842), centreX - knobRadius, centreY - knobRadius,
                                   juce::Colour(0xff121316), centreX + knobRadius, centreY + knobRadius, false);
    g.setGradientFill(bezelGrad);
    g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

    // CNC Machined Gripping Notches around Dial Rim (32 micro-teeth)
    const int numNotches = 32;
    for (int i = 0; i < numNotches; ++i)
    {
        const float notchAngle = static_cast<float>(i) * (juce::MathConstants<float>::twoPi / static_cast<float>(numNotches));
        const float cosA = std::cos(notchAngle);
        const float sinA = std::sin(notchAngle);
        g.setColour(juce::Colour(0x35000000));
        g.drawLine(centreX + cosA * (knobRadius - 3.5f), centreY + sinA * (knobRadius - 3.5f),
                   centreX + cosA * knobRadius, centreY + sinA * knobRadius, 1.2f);
    }

    // 5. Knob Face (Anodized Lathe Machined Texture)
    const auto innerRadius = knobRadius - 3.0f;
    juce::ColourGradient faceGrad(juce::Colour(0xff22242c), centreX, centreY - innerRadius,
                                  juce::Colour(0xff0f1013), centreX, centreY + innerRadius, false);
    g.setGradientFill(faceGrad);
    g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

    // Lathe micro-grooves (concentric machined aerospace rings)
    g.setColour(juce::Colour(0x15ffffff));
    g.drawEllipse(centreX - innerRadius * 0.75f, centreY - innerRadius * 0.75f, innerRadius * 1.5f, innerRadius * 1.5f, 0.7f);
    g.drawEllipse(centreX - innerRadius * 0.45f, centreY - innerRadius * 0.45f, innerRadius * 0.9f, innerRadius * 0.9f, 0.7f);

    // Subtle inner rim bevel outline
    g.setColour(juce::Colour(0x40ffffff));
    g.drawEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.0f);

    // 6. Futuristic Laser Indicator Needle
    juce::Path p;
    const float pointerLength = innerRadius * 0.75f;
    const float pointerThickness = 2.4f;
    p.addRoundedRectangle(-pointerThickness * 0.5f, -innerRadius + 2.5f, pointerThickness, pointerLength, 1.0f);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    // Pointer Bloom
    g.setColour(juce::Colour(0x50ffffff));
    g.strokePath(p, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(accentWhite);
    g.fillPath(p);

    // Glowing tip pip
    const float tipX = centreX + std::sin(angle) * (innerRadius - 4.0f);
    const float tipY = centreY - std::cos(angle) * (innerRadius - 4.0f);
    g.setColour(juce::Colour(0xffffffff));
    g.fillEllipse(tipX - 1.8f, tipY - 1.8f, 3.6f, 3.6f);

    // Center machined spindle
    g.setColour(juce::Colour(0xff09090b));
    g.fillEllipse(centreX - 3.2f, centreY - 3.2f, 6.4f, 6.4f);
    g.setColour(juce::Colour(0x70ffffff));
    g.drawEllipse(centreX - 3.2f, centreY - 3.2f, 6.4f, 6.4f, 1.0f);
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

    juce::ignoreUnused(shouldDrawButtonAsHighlighted);
    g.setColour(button.isEnabled() ? accentWhite : textMuted);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
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
    g.setFont(juce::Font(juce::FontOptions(12.5f).withStyle(isTicked ? "Bold" : "Regular")));

    auto textArea = area.reduced(10, 0);
    g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);
}

} // namespace toytune
