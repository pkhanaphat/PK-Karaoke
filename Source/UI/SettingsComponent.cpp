#include "UI/SettingsComponent.h"

// ============================================================
// Shared design tokens
// ============================================================
namespace SettingsStyle {
static const juce::Colour bg{0xff1e2128};
static const juce::Colour cardBg{0xff282c35};
static const juce::Colour accent{0xff5865f2};
static const juce::Colour accentHover{0xff6e7af4};
static const juce::Colour textPrimary{0xfff0f0f5};
static const juce::Colour textMuted{0xff9a9ab0};
static const juce::Colour border{0xff3a3f50};
static const juce::Colour inputBg{0xff14171f};
static const juce::Colour btnDanger{0xffed4245};

static const int ROW_H = 44;
static const int BTN_H = 38;
static const int LABEL_W = 130;
static const int BROWSE_W = 80;
static const int SECTION_H = 32;
static const int PAD = 20;
static const float FONT_SECTION = 15.0f;
static const float FONT_LABEL = 14.0f;
static const float FONT_BODY = 13.5f;

// Draw a subtle section header bar
static void drawSectionHeader(juce::Graphics &g, juce::Rectangle<int> area,
                              const juce::String &title) {
  g.setColour(border);
  g.drawRect(area, 1);
  g.setColour(accent.withAlpha(0.15f));
  g.fillRect(area);
  g.setColour(accent);
  g.fillRect(area.removeFromLeft(3));
  g.setColour(textPrimary);
  g.setFont(juce::Font(FONT_SECTION, juce::Font::bold));
  g.drawText(title, area.withTrimmedLeft(10), juce::Justification::centredLeft,
             true);
}

// Style a TextButton as primary action
static void stylePrimaryButton(juce::TextButton &btn) {
  btn.setColour(juce::TextButton::buttonColourId, accent);
  btn.setColour(juce::TextButton::buttonOnColourId, accentHover);
  btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
  btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

// Style a TextButton as secondary (browse / small)
static void styleSecondaryButton(juce::TextButton &btn) {
  btn.setColour(juce::TextButton::buttonColourId, border);
  btn.setColour(juce::TextButton::buttonOnColourId, accent);
  btn.setColour(juce::TextButton::textColourOffId, textPrimary);
  btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

// Style a TextButton as danger (remove)
static void styleDangerButton(juce::TextButton &btn) {
  btn.setColour(juce::TextButton::buttonColourId, btnDanger.withAlpha(0.7f));
  btn.setColour(juce::TextButton::buttonOnColourId, btnDanger);
  btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
  btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

// Style a read-only TextEditor
static void styleTextEditor(juce::TextEditor &ed) {
  ed.setColour(juce::TextEditor::backgroundColourId, inputBg);
  ed.setColour(juce::TextEditor::textColourId, textPrimary);
  ed.setColour(juce::TextEditor::outlineColourId, border);
  ed.setColour(juce::TextEditor::focusedOutlineColourId, accent);
  ed.setFont(juce::Font(FONT_BODY));
}

// Style a Label
static void styleLabel(juce::Label &lbl, float fontSize = FONT_LABEL) {
  lbl.setColour(juce::Label::textColourId, textPrimary);
  lbl.setFont(juce::Font(fontSize));
}

static void styleLabelMuted(juce::Label &lbl) {
  lbl.setColour(juce::Label::textColourId, textMuted);
  lbl.setFont(juce::Font(FONT_BODY));
}
} // namespace SettingsStyle

using namespace SettingsStyle;

// ============================================================
// DatabaseSettingsPanel
// ============================================================
DatabaseSettingsPanel::DatabaseSettingsPanel() : progressBar(progress) {
  addAndMakeVisible(ncnLabel);
  addAndMakeVisible(ncnPathEditor);
  addAndMakeVisible(ncnBrowseButton);

  addAndMakeVisible(pkmLabel);
  addAndMakeVisible(pkmPathEditor);
  addAndMakeVisible(pkmBrowseButton);

  addAndMakeVisible(progressLabel);
  addAndMakeVisible(progressBar);

  addAndMakeVisible(totalSongsLabel);
  addAndMakeVisible(buildDbButton);

  ncnPathEditor.setReadOnly(true);
  pkmPathEditor.setReadOnly(true);

  ncnLabel.setText(u8"NCN :", juce::dontSendNotification);
  pkmLabel.setText(u8"PKM :", juce::dontSendNotification);
  progressLabel.setText(u8"ความคืบหน้า :", juce::dontSendNotification);
  buildDbButton.setButtonText(u8"ปรับปรุงฐานข้อมูล");
  totalSongsLabel.setText(u8"จำนวนเพลงทั้งหมด : 0 เพลง",
                          juce::dontSendNotification);

  // Apply styles
  styleLabel(ncnLabel);
  styleLabel(pkmLabel);
  styleLabel(progressLabel);
  styleLabelMuted(totalSongsLabel);
  styleTextEditor(ncnPathEditor);
  styleTextEditor(pkmPathEditor);
  stylePrimaryButton(buildDbButton);
  styleSecondaryButton(ncnBrowseButton);
  styleSecondaryButton(pkmBrowseButton);

  ncnBrowseButton.setButtonText(u8"เลือก...");
  pkmBrowseButton.setButtonText(u8"เลือก...");

  progressBar.setColour(juce::ProgressBar::backgroundColourId, inputBg);
  progressBar.setColour(juce::ProgressBar::foregroundColourId, accent);
}

void DatabaseSettingsPanel::paint(juce::Graphics &g) {
  g.fillAll(bg);

  // Section 1 header
  auto b = getLocalBounds().reduced(PAD);
  drawSectionHeader(g, b.removeFromTop(SECTION_H), u8"ที่เก็บเพลง (Song Library)");
  b.removeFromTop(ROW_H); // NCN row
  b.removeFromTop(8);
  b.removeFromTop(ROW_H); // PKM row
  b.removeFromTop(PAD * 2);

  // Section 2 header
  drawSectionHeader(g, b.removeFromTop(SECTION_H),
                    u8"ปรับปรุงฐานข้อมูลเพลง (Database)");
}

void DatabaseSettingsPanel::resized() {
  auto area = getLocalBounds().reduced(PAD);

  // --- Section 1: Song Library ---
  area.removeFromTop(SECTION_H); // header painted in paint()
  area.removeFromTop(8);

  // NCN row
  auto ncnRow = area.removeFromTop(ROW_H);
  ncnLabel.setBounds(ncnRow.removeFromLeft(LABEL_W)
                         .withTrimmedBottom((ROW_H - 22) / 2)
                         .withTrimmedTop((ROW_H - 22) / 2));
  ncnRow.removeFromLeft(8);
  ncnBrowseButton.setBounds(ncnRow.removeFromRight(BROWSE_W).reduced(0, 4));
  ncnRow.removeFromRight(6);
  ncnPathEditor.setBounds(ncnRow.reduced(0, 4));

  area.removeFromTop(8);

  // PKM row
  auto pkmRow = area.removeFromTop(ROW_H);
  pkmLabel.setBounds(pkmRow.removeFromLeft(LABEL_W)
                         .withTrimmedBottom((ROW_H - 22) / 2)
                         .withTrimmedTop((ROW_H - 22) / 2));
  pkmRow.removeFromLeft(8);
  pkmBrowseButton.setBounds(pkmRow.removeFromRight(BROWSE_W).reduced(0, 4));
  pkmRow.removeFromRight(6);
  pkmPathEditor.setBounds(pkmRow.reduced(0, 4));

  area.removeFromTop(PAD * 2);

  // --- Section 2: Database ---
  area.removeFromTop(SECTION_H); // header painted in paint()
  area.removeFromTop(14);

  // Progress row
  auto progressRow = area.removeFromTop(ROW_H);
  progressLabel.setBounds(progressRow.removeFromLeft(LABEL_W)
                              .withTrimmedBottom((ROW_H - 22) / 2)
                              .withTrimmedTop((ROW_H - 22) / 2));
  progressRow.removeFromLeft(8);
  progressBar.setBounds(progressRow.reduced(0, 10));

  area.removeFromTop(10);

  // Song count
  totalSongsLabel.setBounds(
      area.removeFromTop(28).withTrimmedLeft(LABEL_W + 8));

  area.removeFromTop(14);

  // Build button — centered, larger
  buildDbButton.setBounds(
      area.removeFromTop(BTN_H).withSizeKeepingCentre(240, BTN_H));
}

void DatabaseSettingsPanel::updateProgress(double newProgress) {
  progress = newProgress;
  progressBar.setPercentageDisplay(true);
}

void DatabaseSettingsPanel::setTotalSongs(int count) {
  juce::String text = juce::String(u8"จำนวนเพลงทั้งหมด : ") +
                      juce::String(count) + juce::String(u8" เพลง");
  totalSongsLabel.setText(text, juce::dontSendNotification);
}

// ============================================================
// VstiSettingsPanel
// ============================================================
VstiSettingsPanel::VstiSettingsPanel() {
  for (int i = 0; i < 8; ++i) {
    nameLabels[i].setText("Slot " + juce::String(i + 1) + ": Not Loaded",
                          juce::dontSendNotification);
    styleLabel(nameLabels[i], FONT_BODY);
    addAndMakeVisible(nameLabels[i]);

    // ปุ่มเลือก VSTi (พร้อมใช้งานเสมอ)
    loadButtons[i].setButtonText(
        u8"\u0e40\u0e25\u0e37\u0e2d\u0e01 VSTi  \u25be");
    stylePrimaryButton(loadButtons[i]);
    addAndMakeVisible(loadButtons[i]);
    loadButtons[i].onClick = [this, i]() {
      if (availablePlugins.isEmpty()) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            u8"\u0e44\u0e21\u0e48\u0e1e\u0e1a Plugin",
            u8"\u0e22\u0e31\u0e07\u0e44\u0e21\u0e48\u0e21\u0e35 VST Plugin "
            u8"\u0e43\u0e19\u0e23\u0e32\u0e22\u0e01\u0e32\u0e23 "
            u8"\u0e01\u0e23\u0e38\u0e13\u0e32\u0e44\u0e1b\u0e41\u0e2a\u0e01"
            u8"\u0e19 Plugin \u0e43\u0e19\u0e2b\u0e19\u0e49\u0e32 Settings > "
            u8"VST Plugins \u0e01\u0e48\u0e2d\u0e19\u0e04\u0e23\u0e31\u0e1a",
            u8"\u0e15\u0e01\u0e25\u0e07");
        return;
      }
      juce::PopupMenu menu;
      for (int j = 0; j < availablePlugins.size(); ++j) {
        const auto &desc = availablePlugins.getReference(j);
        menu.addItem(j + 1, desc.name + "  (" + desc.version + ")");
      }
      auto options = juce::PopupMenu::Options()
                         .withTargetComponent(&loadButtons[i])
                         .withMinimumWidth(220);
      menu.showMenuAsync(options, [this, slotIdx = i + 1](int result) {
        if (result > 0 && result <= availablePlugins.size()) {
          const auto &desc = availablePlugins.getReference(result - 1);
          if (onPluginSelected)
            onPluginSelected(slotIdx, desc);
        }
      });
    };

    // ปุ่มเปิด UI ของ VSTi (เปิดใช้งานเปิดขึ้นเมื่อโหลด Plugin แล้ว)
    openButtons[i].setButtonText(u8"\u0e40\u0e1b\u0e34\u0e14 UI");
    stylePrimaryButton(openButtons[i]);
    openButtons[i].setEnabled(false); // Disabled เริ่มต้น
    addAndMakeVisible(openButtons[i]);
    openButtons[i].onClick = [this, i]() {
      if (onOpenVstiClicked)
        onOpenVstiClicked(i + 1);
    };

    // ปุ่มลบ Plugin (เปิดใช้งานเมื่อโหลด Plugin แล้ว)
    removeButtons[i].setButtonText(u8"\u0e25บ");
    styleDangerButton(removeButtons[i]);
    removeButtons[i].setEnabled(false); // Disabled เริ่มต้น
    addAndMakeVisible(removeButtons[i]);
    removeButtons[i].onClick = [this, i]() {
      if (onRemoveVstiClicked)
        onRemoveVstiClicked(i + 1);
    };
  }
}

void VstiSettingsPanel::updateSlotState(int slotIndex, bool pluginLoaded) {
  int i = slotIndex - 1;
  if (i < 0 || i >= 8)
    return;
  openButtons[i].setEnabled(pluginLoaded);
  removeButtons[i].setEnabled(pluginLoaded);
}

void VstiSettingsPanel::setAvailablePlugins(
    const juce::OwnedArray<juce::PluginDescription> &plugins) {
  availablePlugins.clear();
  for (const auto *p : plugins)
    availablePlugins.add(*p);
}

void VstiSettingsPanel::paint(juce::Graphics &g) {
  g.fillAll(bg);
  auto b = getLocalBounds().reduced(PAD);
  drawSectionHeader(
      g, b.removeFromTop(SECTION_H),
      u8"\u0e08\u0e31\u0e14\u0e01\u0e32\u0e23 VSTi (Virtual Instruments)");
}

void VstiSettingsPanel::resized() {
  auto area = getLocalBounds().reduced(PAD);
  area.removeFromTop(SECTION_H + 10);

  const int rowH = 42;
  const int nameW = 180;
  const int loadW = 150;
  const int openW = 80;
  const int removeW = 55;
  const int gap = 6;

  for (int i = 0; i < 8; ++i) {
    auto row = area.removeFromTop(rowH);
    nameLabels[i].setBounds(row.removeFromLeft(nameW));
    row.removeFromLeft(gap);
    loadButtons[i].setBounds(row.removeFromLeft(loadW).reduced(0, 4));
    row.removeFromLeft(gap);
    openButtons[i].setBounds(row.removeFromLeft(openW).reduced(0, 4));
    row.removeFromLeft(gap);
    removeButtons[i].setBounds(row.removeFromLeft(removeW).reduced(0, 4));
    area.removeFromTop(6);
  }
}

// ============================================================
// SoundFontSettingsPanel
// ============================================================
SoundFontSettingsPanel::SoundFontSettingsPanel() {
  addAndMakeVisible(folderLabel);
  addAndMakeVisible(folderPathEditor);
  addAndMakeVisible(browseButton);
  addAndMakeVisible(globalSf2Label);
  addAndMakeVisible(globalSf2ComboBox);

  folderLabel.setText(u8"โฟลเดอร์ SoundFont :", juce::dontSendNotification);
  folderPathEditor.setReadOnly(true);

  globalSf2Label.setText(u8"Global SF2 (ค่าเริ่มต้น) :",
                         juce::dontSendNotification);
  globalSf2ComboBox.setTextWhenNothingSelected(
      u8"-- ผือเลือก SF2 สำหรับทุก Track --");
  globalSf2ComboBox.onChange = [this]() {
    if (onGlobalSf2Changed) {
      if (globalSf2ComboBox.getSelectedId() > 0)
        onGlobalSf2Changed(globalSf2ComboBox.getText());
      else
        onGlobalSf2Changed("");
    }
  };

  styleLabel(folderLabel);
  styleLabel(globalSf2Label);
  styleTextEditor(folderPathEditor);
  styleSecondaryButton(browseButton);
  browseButton.setButtonText(u8"เลือก...");

  browseButton.onClick = [this]() {
    if (onBrowseClicked)
      onBrowseClicked();
  };
}

void SoundFontSettingsPanel::paint(juce::Graphics &g) {
  g.fillAll(bg);
  auto b = getLocalBounds().reduced(PAD);
  drawSectionHeader(g, b.removeFromTop(SECTION_H),
                    u8"จัดการ SoundFont (.sf2 / .sf3)");
}

void SoundFontSettingsPanel::resized() {
  auto area = getLocalBounds().reduced(PAD);
  area.removeFromTop(SECTION_H + 14);

  // Row 1: SF2 Folder
  auto row = area.removeFromTop(ROW_H);
  folderLabel.setBounds(row.removeFromLeft(180)
                            .withTrimmedBottom((ROW_H - 22) / 2)
                            .withTrimmedTop((ROW_H - 22) / 2));
  row.removeFromLeft(8);
  browseButton.setBounds(row.removeFromRight(BROWSE_W).reduced(0, 4));
  row.removeFromRight(6);
  folderPathEditor.setBounds(row.reduced(0, 4));

  area.removeFromTop(12);

  // Row 2: Global SF2 ComboBox
  auto row2 = area.removeFromTop(ROW_H);
  globalSf2Label.setBounds(row2.removeFromLeft(180)
                               .withTrimmedBottom((ROW_H - 22) / 2)
                               .withTrimmedTop((ROW_H - 22) / 2));
  row2.removeFromLeft(8);
  globalSf2ComboBox.setBounds(row2.reduced(0, 4));
}

void SoundFontSettingsPanel::refreshGlobalSf2List(
    const juce::StringArray &sf2Names) {
  globalSf2ComboBox.clear(juce::dontSendNotification);
  globalSf2ComboBox.addItemList(sf2Names, 1);
}

void SoundFontSettingsPanel::setSelectedGlobalSf2(const juce::String &sf2Name) {
  if (sf2Name.isEmpty()) {
    globalSf2ComboBox.setSelectedId(0, juce::dontSendNotification);
  } else {
    globalSf2ComboBox.setText(sf2Name, juce::dontSendNotification);
  }
}
// ============================================================
SettingsComponent::SettingsComponent(PluginHost &host) : vstScannerPanel(host) {
  addAndMakeVisible(tabs);

  // Tab bar colours
  tabs.setColour(juce::TabbedComponent::backgroundColourId, bg);
  tabs.setColour(juce::TabbedButtonBar::tabTextColourId, textMuted);
  tabs.setColour(juce::TabbedButtonBar::frontTextColourId,
                 juce::Colours::white);

  tabs.addTab(u8"ทั่วไป", bg, &generalPanel, false);
  tabs.addTab(u8"ฐานข้อมูลเพลง", bg, &dbPanel, false);
  tabs.addTab(u8"อุปกรณ์เสียง", bg, &soundDevicePanel, false);
  tabs.addTab(u8"ซาวด์ฟอนท์", bg, &sf2Panel, false);
  tabs.addTab(u8"VST Plugins", bg, &vstScannerPanel, false);

  // Set tab bar to a taller height for readability
  tabs.setTabBarDepth(40);

  // Colour each tab
  for (int i = 0; i < tabs.getNumTabs(); ++i)
    tabs.setTabBackgroundColour(i, cardBg);

  // ---------- General panel ----------
  generalPanel.addAndMakeVisible(loadBgButton);
  loadBgButton.setButtonText(u8"เลือกภาพพื้นหลัง (Background Image)");
  stylePrimaryButton(loadBgButton);

  // -------Backward compat / hidden ------
  sf2Button.setButtonText(u8"เลือก SoundFont");
  sf2Panel.addAndMakeVisible(sf2Button);
  sf2Button.setVisible(false);

  // ---------- Button callbacks ----------
  dbPanel.ncnBrowseButton.onClick = [this]() {
    if (onLoadNcnClicked)
      onLoadNcnClicked();
  };
  dbPanel.pkmBrowseButton.onClick = [this]() {
    if (onLoadPkmClicked)
      onLoadPkmClicked();
  };
  dbPanel.buildDbButton.onClick = [this]() {
    if (onBuildDbClicked)
      onBuildDbClicked();
  };
  sf2Button.onClick = [this]() {
    if (onLoadSf2Clicked)
      onLoadSf2Clicked();
  };
  loadBgButton.onClick = [this]() {
    if (onLoadBgClicked)
      onLoadBgClicked();
  };
}

SettingsComponent::~SettingsComponent() {}

void SettingsComponent::paint(juce::Graphics &g) {
  g.fillAll(bg);

  // Title bar
  auto titleArea = getLocalBounds().removeFromTop(50);
  g.setColour(cardBg);
  g.fillRect(titleArea);
  g.setColour(border);
  g.drawLine(0, 50, (float)getWidth(), 50, 1.0f);
  g.setColour(accent);
  g.fillRect(0, 48, getWidth(), 2);

  g.setColour(textPrimary);
  g.setFont(juce::Font(17.0f, juce::Font::bold));
  g.drawText(u8"\u2699\ufe0f  การตั้งค่า  (Settings)", titleArea.reduced(PAD, 0),
             juce::Justification::centredLeft, true);
}

void SettingsComponent::resized() {
  auto area = getLocalBounds();
  area.removeFromTop(50); // title bar
  tabs.setBounds(area);

  // General panel: centre the background button
  loadBgButton.setBounds(generalPanel.getLocalBounds()
                             .reduced(PAD)
                             .removeFromTop(100 + SECTION_H)
                             .removeFromBottom(BTN_H + 20)
                             .withSizeKeepingCentre(280, BTN_H));

  sf2Button.setBounds(
      sf2Panel.getLocalBounds().withSizeKeepingCentre(200, BTN_H));
}

// ============================================================
// Setters / callbacks
// ============================================================
void SettingsComponent::setOnLoadNcnClicked(std::function<void()> callback) {
  onLoadNcnClicked = callback;
}
void SettingsComponent::setOnLoadPkmClicked(std::function<void()> callback) {
  onLoadPkmClicked = callback;
}
void SettingsComponent::setOnBuildDbClicked(std::function<void()> callback) {
  onBuildDbClicked = callback;
}
void SettingsComponent::setOnLoadSf2Clicked(std::function<void()> callback) {
  onLoadSf2Clicked = callback;
}
void SettingsComponent::setOnLoadBgClicked(std::function<void()> callback) {
  onLoadBgClicked = callback;
}
void SettingsComponent::setNcnPath(const juce::String &path) {
  dbPanel.ncnPathEditor.setText(path);
}
void SettingsComponent::setPkmPath(const juce::String &path) {
  dbPanel.pkmPathEditor.setText(path);
}
void SettingsComponent::setDbProgress(double progress) {
  dbPanel.updateProgress(progress);
}
void SettingsComponent::setTotalSongs(int count) {
  dbPanel.setTotalSongs(count);
}
void SettingsComponent::setSf2ButtonText(const juce::String &text) {
  sf2Button.setButtonText(text);
}
