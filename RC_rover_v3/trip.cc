#include <string>
#include <sstream>
#include <algorithm>
#include <Arduino.h>
#include "trip.h"

Move::Move(char dir, char moveType, unsigned short speed, unsigned long duration)
  : dir{ dir }, moveType{ moveType }, speed{ speed }, duration{ duration } {}

Move Move::reverse() {  
    char revDir = (dir == 'F') ? 'R' : 'F';
    char revMoveType = moveType;
    // Left turn in forward motion becomes turn in reverse
    if (moveType == 'L') {      
      revMoveType = 'R';
    } else if (moveType == 'R'){
      revMoveType = 'L';
    } else {
      // reverse moveType remains the same as forward moveType
    }

    return Move(revDir, revMoveType, speed, duration);
}

std::string Move::toString() {
  std::ostringstream os;
  os << dir << ":" << moveType << ":" << speed << ":" << duration;
  return os.str();
}

Trip::Trip()
  : lastMoveEnd{ millis() } {}

void Trip::endLastMove() {  
  // Terminates the last move by updating its duration
  // and updating the timestamp
  if (!moves.empty()) { 
    unsigned long now = millis();
    moves.back().duration = now - lastMoveEnd;
    lastMoveEnd = now;
  }
}

void Trip::addMove(const Move& move) {
  moves.push_back(move);
}

std::vector<Move> Trip::getMoves() const {
  return moves;
}

std::string Trip::toString() {
  std::ostringstream os;
  for (auto m : moves) {
    os << m.toString() << '\n';
  }  
  return os.str();
}

void Trip::clear() {
  moves.clear();
}

Trip Trip::reverse() {
  Trip revTrip;  
  for (auto m: moves) {
    revTrip.addMove(m.reverse());
  }
  return revTrip;
}