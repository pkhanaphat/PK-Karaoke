#pragma once

#include "Audio/AudioGraphManager.h"
#include "Core/Routing/MixerController.h"
#include "UI/LookAndFeel_PK_Mixer.h"
#include <JuceHeader.h>

//==============================================================================
// InstrumentRow - Data model for one row
//==============================================================================
struct InstrumentRow {
  InstrumentGroup group;
  juce::String name;
};

//==============================================================================
// OutputComboCell - Custom component for each row's output selector
//==============================================================================
class OutputComboCell : public juce::Component,
                        public juce::ComboBox::Listener {
public:
  std::function<void(InstrumentGroup, OutputDestination, juce::String)>
      onChange;
  InstrumentGroup group;
  juce::StringArray sf2Names;

  OutputComboCell() {
    combo.setJustificationType(juce::Justification::centredLeft);
    combo.addListener(this);
    addAndMakeVisible(combo);
  }

  ~OutputComboCell() override { combo.removeListener(this); }

  void resized() override { combo.setBounds(getLocalBounds().reduced(2, 3)); }

  void populate(const juce::StringArray &availableSf2Names,
                AudioGraphManager &agm, const juce::String &defaultSF2Path) {
    combo.clear();
    sf2Names = availableSf2Names;
    int id = 1;

    // 1. Default SF2 — show actual filename if available
    juce::String defaultLabel = "SF2 (Default)";
    juce::String defaultName = "";
    if (defaultSF2Path.isNotEmpty()) {
      defaultName = juce::File(defaultSF2Path).getFileNameWithoutExtension();
      defaultLabel = defaultName + " (Default)";
    }
    combo.addItem(defaultLabel, id++);

    // 2. VSTi 1-8 - Only show LOADED VSTi slots
    for (int i = 0; i < 8; ++i) {
      juce::String vstiName = agm.getLoadedVstiName(i);
      if (vstiName.isNotEmpty()) {
        combo.addItem("VSTi " + juce::String(i + 1) + " - " + vstiName, i + 2);
      }
    }

    // Resume id for custom SF2 files after the 8 VSTi slots
    id = 10;

    // 3. Custom SF2 files — skip duplicates (e.g. if default SF2 is in the
    // list)
    for (const auto &name : sf2Names) {
      if (name != defaultName) // skip the default to avoid duplicates
        combo.addItem(name, id++);
      else
        id++; // keep IDs stable
    }
  }

  void setSelectedOutput(OutputDestination dest, const juce::String &sf2Path) {
    combo.removeListener(this);

    if (dest != OutputDestination::SF2) {
      combo.setSelectedId((int)dest + 1, juce::dontSendNotification);
    } else {
      if (sf2Path.isEmpty()) {
        combo.setSelectedId(1, juce::dontSendNotification);
      } else {
        int idx = sf2Names.indexOf(sf2Path);
        if (idx >= 0) {
          combo.setSelectedId(1 + 8 + 1 + idx, juce::dontSendNotification);
        } else {
          combo.setSelectedId(1, juce::dontSendNotification);
        }
      }
    }

    if (combo.getSelectedId() == 0) {
      combo.setText("Not Loaded", juce::dontSendNotification);
    }

    combo.addListener(this);
  }

  void comboBoxChanged(juce::ComboBox *) override {
    int id = combo.getSelectedId();
    if (id == 0)
      return;

    OutputDestination dest;
    juce::String path = "";

    if (id == 1) {
      dest = OutputDestination::SF2;
    } else if (id >= 2 && id <= 9) {
      dest = static_cast<OutputDestination>(id - 1);
    } else {
      dest = OutputDestination::SF2;
      path = sf2Names[id - 10];
    }

    if (onChange) {
      juce::Timer::callAfterDelay(50, [this, dest, path]() {
        if (onChange)
          onChange(group, dest, path);
      });
    }
  }

  juce::ComboBox combo;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputComboCell)
};

//==============================================================================
// InstrumentTableModel
//==============================================================================
class InstrumentTableModel : public juce::TableListBoxModel {
public:
  InstrumentTableModel(MixerController &mc, AudioGraphManager &agm,
                       juce::Array<InstrumentRow> &rows,
                       juce::TableListBox &tbl)
      : mixerController(mc), audioGraphManager(agm), rows(rows), table(tbl) {}

  std::function<void()> onSettingsChanged;

  int getNumRows() override { return rows.size(); }

  void paintRowBackground(juce::Graphics &g, int rowNumber, int /*w*/,
                          int /*h*/, bool rowIsSelected) override {
    g.fillAll(rowIsSelected ? juce::Colour(0xff007ACC)
                            : (rowNumber % 2 == 0 ? juce::Colour(0xff252526)
                                                  : juce::Colour(0xff2d2d30)));
  }

  void paintCell(juce::Graphics &g, int rowNumber, int columnId, int width,
                 int height, bool rowIsSelected) override {
    if (columnId == 1 && rowNumber < rows.size()) {
      g.setColour(rowIsSelected ? juce::Colours::white
                                : juce::Colour(0xffdddddd));
      g.setFont(juce::Font(13.0f));
      g.drawText(rows.getReference(rowNumber).name, 8, 0, width - 8, height,
                 juce::Justification::centredLeft, true);
    }
  }

  juce::Component *refreshComponentForCell(int rowNumber, int columnId,
                                           bool /*isRowSelected*/,
                                           juce::Component *existing) override {
    if (columnId == 2) {
      if (rowNumber >= rows.size())
        return nullptr;
      auto *cell = dynamic_cast<OutputComboCell *>(existing);
      if (cell == nullptr) {
        cell = new OutputComboCell();
        cell->populate(mixerController.getAvailableSF2Names(),
                       audioGraphManager, mixerController.getGlobalSF2Path());
      }

      auto &row = rows.getReference(rowNumber);
      cell->group = row.group;

      juce::String savedPath = mixerController.getTrackSF2Path(row.group);
      juce::String displayName = "";
      if (savedPath.isNotEmpty()) {
        displayName = juce::File(savedPath).getFileNameWithoutExtension();
      }

      cell->setSelectedOutput(
          mixerController.getTrackOutputDestination(row.group), displayName);

      cell->onChange = [this, rowNumber](InstrumentGroup grp,
                                         OutputDestination dest,
                                         juce::String name) {
        auto selectedRows = table.getSelectedRows();
        juce::String actualPath = "";
        if (dest == OutputDestination::SF2 && name.isNotEmpty()) {
          actualPath = mixerController.getSF2FileByName(name).getFullPathName();
        }

        if (selectedRows.contains(rowNumber)) {
          for (int i = 0; i < selectedRows.size(); ++i) {
            int r = selectedRows[i];
            if (r >= 0 && r < rows.size()) {
              auto g = rows.getReference(r).group;
              mixerController.setTrackOutputDestination(g, dest);
              mixerController.setTrackSF2Path(g,
                                              actualPath); // Save absolute path
            }
          }
        } else {
          mixerController.setTrackOutputDestination(grp, dest);
          mixerController.setTrackSF2Path(grp,
                                          actualPath); // Save absolute path
        }

        audioGraphManager.triggerRebuild();

        if (onSettingsChanged)
          onSettingsChanged();

        table.updateContent();
        table.repaint();
      };
      return cell;
    }
    delete existing;
    return nullptr;
  }

  juce::Array<InstrumentRow> &getRows() { return rows; }

private:
  MixerController &mixerController;
  AudioGraphManager &audioGraphManager;
  juce::Array<InstrumentRow> &rows;
  juce::TableListBox &table;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentTableModel)
};

//==============================================================================
// DragTableListBox
//==============================================================================
class DragTableListBox : public juce::TableListBox {
public:
  DragTableListBox(const juce::String &componentName,
                   juce::TableListBoxModel *model)
      : juce::TableListBox(componentName, model) {
    setMultipleSelectionEnabled(true);
  }

  void mouseDown(const juce::MouseEvent &e) override {
    if (!e.mods.isShiftDown() && !e.mods.isCtrlDown() &&
        !e.mods.isCommandDown()) {
      int row = getRowContainingPosition(e.x, e.y);
      if (row >= 0) {
        selectRow(row, false, true);
        lastDragRow = row;
        return;
      }
    }
    juce::TableListBox::mouseDown(e);
  }

  void mouseDrag(const juce::MouseEvent &e) override {
    int row = getRowContainingPosition(e.x, e.y);
    if (row >= 0 && row != lastDragRow) {
      if (!isRowSelected(row))
        selectRow(row, false, false);
      lastDragRow = row;
    }
    juce::TableListBox::mouseDrag(e);
  }

private:
  int lastDragRow = -1;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragTableListBox)
};

//==============================================================================
// InstrumentSettingsPanel
//==============================================================================
class InstrumentSettingsPanel : public juce::Component {
public:
  InstrumentSettingsPanel(MixerController &mc, AudioGraphManager &agm);
  ~InstrumentSettingsPanel() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  std::function<void()> onSettingsChanged;
  std::function<void()> onRightClickBackground;

  void mouseUp(const juce::MouseEvent &e) override {
    if (e.mods.isPopupMenu()) {
      juce::PopupMenu m;
      m.addItem(1, "Open Synth Mixer...");
      m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
        if (result == 1 && onRightClickBackground) {
          onRightClickBackground();
        }
      });
    }
  }

private:
  MixerController &mixerController;
  AudioGraphManager &audioGraphManager;
  LookAndFeel_PK_Mixer laf;

  juce::Label headerLabel;
  juce::TabbedComponent tabs;

  juce::Array<InstrumentRow> melodicRows;
  juce::Array<InstrumentRow> drumRows;

  std::unique_ptr<InstrumentTableModel> melodicModel;
  std::unique_ptr<InstrumentTableModel> drumModel;

  DragTableListBox melodicTable;
  DragTableListBox drumTable;

  juce::Label hintLabel;

  void buildRows();
  void setupTable(juce::TableListBox &table, juce::TableListBoxModel *model);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentSettingsPanel)
};

//==============================================================================
// InstrumentSettingsWindow - Document window wrapper
//==============================================================================
class InstrumentSettingsWindow : public juce::DocumentWindow {
public:
  InstrumentSettingsWindow(MixerController &mc, AudioGraphManager &agm,
                           juce::Component *parentForPosition = nullptr);
  ~InstrumentSettingsWindow() override;

  void closeButtonPressed() override;

  std::function<void()> onSettingsChanged;
  std::function<void()> onRightClickBackground;

private:
  std::unique_ptr<InstrumentSettingsPanel> panel;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentSettingsWindow)
};
