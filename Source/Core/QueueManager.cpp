#include "Core/QueueManager.h"

QueueManager::QueueManager() {}

void QueueManager::addSong(const SongRecord &song) {
  queue.add({song, nextUniqueId++});
  sendChangeMessage();
}

void QueueManager::removeSong(int index) {
  if (juce::isPositiveAndBelow(index, queue.size())) {
    queue.remove(index);
    sendChangeMessage();
  }
}

void QueueManager::moveSong(int oldIndex, int newIndex) {
  if (juce::isPositiveAndBelow(oldIndex, queue.size())) {
    queue.move(oldIndex, juce::jlimit(0, queue.size() - 1, newIndex));
    sendChangeMessage();
  }
}

void QueueManager::clear() {
  queue.clear();
  sendChangeMessage();
}

bool QueueManager::popNext(SongRecord &nextSong) {
  if (queue.isEmpty())
    return false;

  nextSong = queue[0].song;
  queue.remove(0);
  sendChangeMessage();
  return true;
}

