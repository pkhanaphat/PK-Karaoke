#pragma once

#include "Database/DatabaseManager.h"
#include <JuceHeader.h>


struct QueueEntry {
  SongRecord song;
  juce::uint32 uniqueId; // Useful for UI identification
};

class QueueManager : public juce::ChangeBroadcaster {
public:
  QueueManager();
  ~QueueManager() = default;

  void addSong(const SongRecord &song);
  void removeSong(int index);
  void moveSong(int oldIndex, int newIndex);
  void clear();

  const juce::Array<QueueEntry> &getQueue() const { return queue; }

  // Pops the next song from the queue. Returns true if a song was available.
  bool popNext(SongRecord &nextSong);

  int getNumItems() const { return queue.size(); }
  bool isEmpty() const { return queue.isEmpty(); }

private:
  juce::Array<QueueEntry> queue;
  juce::uint32 nextUniqueId = 1;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QueueManager)
};

