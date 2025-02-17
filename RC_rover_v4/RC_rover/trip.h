#ifndef _TRIP_H
#define _TRIP_H

#include <vector>
#include <sstream>

class Move {
public:
  char dir;
  char moveType;
  unsigned short speed;
  unsigned long duration;

  Move(char dir, char moveType, unsigned short speed, unsigned long duration);
  Move reverse();
  std::string toString();  
};

class Trip {
  std::vector<Move> moves;
  unsigned long lastMoveEnd;
public:
  Trip();
  void addMove(const Move& move);
  void endLastMove();
  std::string toString();
  void clear();
  Trip reverse();
  std::vector<Move> getMoves() const;
};
#endif