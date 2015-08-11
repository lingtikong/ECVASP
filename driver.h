#ifndef DRIVER_H
#define DRIVER_H

#include "memory.h"

using namespace std;

class Driver {
public:
  Driver(int, char**);
  ~Driver();

private:
  Memory *memory;
  char *poscar, *fname;
  char *title, *element;

  double alat;
  int ntype, natom, *ntm;
  double **atpos;
  double **axis, **dispmat, **newaxis;
  double disp[7];

  int readpos();
  void matmul();
  void generate();
  void writepos(double **, FILE *);

  // help info
  void help();

  int count_words(const char *);
};
#endif
