#include <Arduino.h>
#include "trip.h"

Move::Move(char dir, char turn, unsigned short speed, unsigned long duration)
  : dir{dir}, turn{turn}, speed{speed}, duration{duration} {}

Move Move::reverse(const Move &move) {
  return Move('F', ' ', 0, 0);
}

Trip::Trip(): lastMoveEnd{millis()} {}

void Trip::endLastMove() {
  // Terminates the last move by updating its duration
  // and updating the timestamp
  unsigned long now = millis();
  moves.back().duration = now - lastMoveEnd;  
  lastMoveEnd = now;
}

void Trip::addMove(const Move& move) {
  moves.push_back(move);
}
