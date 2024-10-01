#ifndef _TRIP_H
#define _TRIP_H

#include <vector>
class Move {
  public:
  char dir;
  char turn;
  unsigned short speed;
  unsigned long duration;
  
  Move(char dir, char turn, unsigned short speed, unsigned long duration);
  Move reverse(const Move& move);  
};

class Trip {
  std::vector<Move> moves;
  unsigned long lastMoveEnd;
  public:
  Trip();
  void addMove(const Move& move);
  void endLastMove();  
};
#endif