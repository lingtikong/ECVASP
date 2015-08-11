#include "driver.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <ctype.h>

#define ZERO 1.e-10
#define MAXLINE 1024
#define STRAIN 0.008

/*------------------------------------------------------------------------------
 * Constructor of driver, main menu
 *------------------------------------------------------------------------------ */
Driver::Driver(int narg, char** arg)
{
  ntm = NULL;
  memory = NULL;
  axis = dispmat = newaxis = atpos = NULL;
  for (int i = 0; i < 7; ++i) disp[i] = 0.;
  poscar = fname = title = element = NULL;

  // analyse command line options
  int iarg = 1;
  while (narg > iarg){
    if (strcmp(arg[iarg],"-h") == 0){
      help();

    } else if (strcmp(arg[iarg], "-o") == 0){ // global displacement
      if (++iarg >= narg) help();
      int n = strlen(arg[iarg]);
      if (fname) delete []fname;
      fname = new char [n+1];
      strcmp(fname, arg[iarg]);

    } else if (strcmp(arg[iarg], "-e") == 0){ // global displacement
      if (++iarg >= narg) help();
      disp[0] = fabs(atof(arg[iarg]));

    } else if (strcmp(arg[iarg], "-x") == 0){ // epsilon_{xx}
      if (++iarg >= narg) help();
      disp[1] = fabs(atof(arg[iarg]));

    } else if (strcmp(arg[iarg], "-xx") == 0){ // epsilon_{xx}
      if (++iarg >= narg) help();
      disp[1] = fabs(atof(arg[iarg]));

    } else if (strcmp(arg[iarg], "-y") == 0){ // epsilon_{yy}
      if (++iarg >= narg) help();
      disp[2] = fabs(atof(arg[iarg]));

    } else if (strcmp(arg[iarg], "-yy") == 0){ // epsilon_{yy}
      if (++iarg >= narg) help();
      disp[2] = fabs(atof(arg[iarg]));

    } else if (strcmp(arg[iarg], "-z") == 0){ // epsilon_{zz}
      if (++iarg >= narg) help();
      disp[3] = fabs(atof(arg[iarg]));

    } else if (strcmp(arg[iarg], "-zz") == 0){ // epsilon_{zz}
      if (++iarg >= narg) help();
      disp[3] = fabs(atof(arg[iarg]));

    } else if (strcmp(arg[iarg], "-xy") == 0){ // epsilon_{xy}
      if (++iarg >= narg) help();
      disp[6] = fabs(atof(arg[iarg]));

    } else if (strcmp(arg[iarg], "-xz") == 0){ // epsilon_{xz}
      if (++iarg >= narg) help();
      disp[5] = fabs(atof(arg[iarg]));

    } else if (strcmp(arg[iarg], "-yz") == 0){ // epsilon_{yz}
      if (++iarg >= narg) help();
      disp[4] = fabs(atof(arg[iarg]));

    } else {
      if (poscar) delete []poscar;
      poscar = new char [strlen(arg[iarg])+1];
      strcpy(poscar, arg[iarg]);

    }

    ++iarg;
  }

  if (poscar == NULL){
    poscar = new char [7];
    strcpy(poscar, "POSCAR");
  }

  // set default values
  if (disp[0] < ZERO) disp[0] = STRAIN;
  for (int i = 1; i <= 3; ++i) if (disp[i] < ZERO) disp[i] = disp[0];
  for (int i = 4; i <= 6; ++i) if (disp[i] < ZERO) disp[i] = disp[0]*1.6;

  if (fname == NULL){
    fname = new char[6];
    strcpy(fname, "ecrun");
  }

  memory = new Memory();
  // read the POSCAR
  if ( readpos() ) return;

  // write the script
  generate();

  // write out related info
  printf("\n"); for (int i = 0; i < 20; ++i) printf("====");
  printf("\nEquilibrium config read from : %s\n", poscar);
  printf("Script info written to file  : %s\n", fname);
  printf("Displacement info            : ");
  for (int i = 1; i <= 6; ++i) printf(" %g", disp[i]);
  printf("\n"); for (int i = 0; i < 20; ++i) printf("====");
  printf("\n");
  
return;
}

/*------------------------------------------------------------------------------
 * Method to count # of words in a string, without destroying the string
 *------------------------------------------------------------------------------ */
Driver::~Driver()
{
  if (ntm) memory->destroy(ntm);
  if (axis) memory->destroy(axis);
  if (atpos) memory->destroy(atpos);
  if (newaxis) memory->destroy(newaxis);
  if (dispmat) memory->destroy(dispmat);

  if (fname)  delete []fname;
  if (title)  delete []title;
  if (poscar) delete []poscar;
  if (element) delete []element;

  if (memory) delete memory;
return;
}

/*------------------------------------------------------------------------------
 * Method to read the VASP POSCAR file
 *------------------------------------------------------------------------------ */
int Driver::readpos()
{
  char str[MAXLINE];
  // open the file
  FILE *fp = fopen(poscar, "r");
  if (fp == NULL){
    printf("\nFile %s not found!\n", poscar);
    return 1;
  }
  
  fgets(str, MAXLINE, fp);
  int n = strlen(str);
  title = new char [n+1];
  strcpy(title, str);
  
  fgets(str, MAXLINE, fp);
  alat = atof(strtok(str, " \n\t\r\f"));

  memory->create(axis, 3, 3, "axis");
  memory->create(dispmat, 3, 3, "dispmat");
  memory->create(newaxis, 3, 3, "newaxis");
  for (int i = 0; i < 3; ++i){
    fgets(str, MAXLINE, fp);
    axis[i][0] = atof(strtok(str,  " \n\t\r\f"));
    axis[i][1] = atof(strtok(NULL, " \n\t\r\f"));
    axis[i][2] = atof(strtok(NULL, " \n\t\r\f"));
  }

  fgets(str, MAXLINE, fp);
  n = strlen(str);
  element = new char [n+1];
  strcpy(element, str);
  n = count_words(str);

  char *ptr = strtok(str, " \n\t\r\f");
  if (isalpha(ptr[0])){
    fgets(str, MAXLINE, fp);
    n = count_words(str);
    ptr = strtok(str, " \n\t\r\f");

  } else {
    delete []element; element = NULL;
  }
  ntm = new int[n];
  ntype = n;
  natom = 0;
  for (int i = 0; i < n; ++i){
    ntm[i] = atoi(ptr);
    natom += ntm[i];

    ptr = strtok(NULL, " \n\t\r\f");
  }
  if (natom < 1){
    printf("\nERROR: no atom is identified in file: %s\n!", poscar);
    return 2;
  }
  memory->create(atpos, natom, 3, "atpos");

  fgets(str, MAXLINE, fp);
  ptr = strtok(str, " \n\t\r\f");
  if (ptr[0] == 'S' || ptr[0] == 's'){
    fgets(str, MAXLINE, fp);
    ptr = strtok(str, " \n\t\r\f");
  }
  if (ptr[0] == 'C' || ptr[0] == 'c' || ptr[0] == 'K' || ptr[0] == 'k'){
    printf("\nERROR: the positions read from %s are in cartesian coordinate,\n", poscar);
    printf("while I expect fractional/direct!\n");

    return 3;
  }

  for (int i = 0; i < natom; ++i){
    fgets(str, MAXLINE, fp);
    ptr = strtok(str, " \n\t\r\f");
    for (int ii = 0; ii < 3; ++ii){
      atpos[i][ii] = atof(ptr);
      ptr = strtok(NULL, " \n\t\r\f");
    }
  }
  fclose(fp);
    
return 0;
}

/*------------------------------------------------------------------------------
 * Method to generate the script to compute elastic constants from VASP
 *------------------------------------------------------------------------------ */
void Driver::generate()
{
  FILE *fp = fopen(fname, "w");
  if (fp == NULL){
    printf("\nERROR: cannot open file %s for writting!\n", fname);
    return;
  }

  fprintf(fp,"#!/bin/bash\n#\n# Script to compute the elastic constants based on VASP.\n");
  fprintf(fp,"#===========================================================================\n");
  fprintf(fp,"# Extremely accurate stress calculations are needed to get reliable results.\n");
  fprintf(fp,"# Suggested settings for INCAR:\n");
  fprintf(fp,"#     PREC   = high\n");
  fprintf(fp,"#     ENCUT  = 1.5*ENMAX\n");
  fprintf(fp,"#     ISMEAR = 2   # for metals; \n");
  fprintf(fp,"#     SIGMA  = 0.2 # for metals;\n#\n");
  fprintf(fp,"#     ISMEAR = -5  # for insulators.\n");
  fprintf(fp,"#\n# The strain should be large enough to avoid noise, but small enough to\n");
  fprintf(fp,"#  keep elasticity.\n");
  fprintf(fp,"#===========================================================================\n");
  fprintf(fp,"if [ %c$#%c -gt %c0%c ]; then\n", char(34), char(34), char(34), char(34));
  fprintf(fp,"   np=$1\nelse\n   np=2\nfi\n#\n");
  fprintf(fp,"if [ -f %cPOSCAR%c ]; then\n", char(34), char(34));
  fprintf(fp,"   cp POSCAR POSCAR_ini\nfi\n#\n");
  fprintf(fp,"VASP=%cmpirun -np ${np} v533%c\n", char(34), char(34));
  fprintf(fp,"#\necho %cThe as-provided configuration (equilibrium state expected)%c\n", char(34), char(34));

  writepos(axis, fp);

  fprintf(fp,"press=`grep -B1 'external pressure' OUTCAR|head -1`\n");
  fprintf(fp,"pxx0=`echo ${press}|awk '{print $3}'`\n");
  fprintf(fp,"pyy0=`echo ${press}|awk '{print $4}'`\n");
  fprintf(fp,"pzz0=`echo ${press}|awk '{print $5}'`\n");
  fprintf(fp,"pxy0=`echo ${press}|awk '{print $6}'`\n");
  fprintf(fp,"pyz0=`echo ${press}|awk '{print $7}'`\n");
  fprintf(fp,"pxz0=`echo ${press}|awk '{print $8}'`\n");
  fprintf(fp,"eng0=`grep 'energy  without' OUTCAR|tail -1|awk '{print $4}'`\n");
  fprintf(fp,"echo %c0   0  ${pxx0} ${pyy0} ${pzz0} ${pxy0} ${pxz0} ${pyz0} ${eng0}%c\n", char(34), char(34));
  fprintf(fp,"echo %c# Information on elastic constants calculations, since: `date`%c >> info.dat\n", char(34), char(34));
  fprintf(fp,"echo %c0   0  ${pxx0} ${pyy0} ${pzz0} ${pxy0} ${pxz0} ${pyz0} ${eng0}%c >> info.dat\n", char(34), char(34));
  
  double eps[7];
  for (int idim = 1; idim <= 6; ++idim){
    for (int i = 1; i < 7; ++i) eps[i] = 0.; eps[idim] = disp[idim];
    fprintf(fp,"# Now to compute that for eps = [%g %g %g %g %g %g]\necho\n",  eps[1], eps[2], eps[3], eps[4], eps[5], eps[6]);
    fprintf(fp,"echo %cNow to compute that for eps = [%g %g %g %g %g %g]%c\n", char(34), eps[1], eps[2], eps[3], eps[4], eps[5], eps[6], char(34));
    fprintf(fp,"eps=%c%lg%c\n", char(34), disp[idim], char(34));
    for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) dispmat[i][j] = 0.;

    for (int i = 0; i < 3; ++i) dispmat[i][i] = 1.;

    if (idim <= 3) dispmat[idim-1][idim-1] += disp[idim];
    if (idim == 4) dispmat[2][1] = disp[idim];
    if (idim == 5) dispmat[2][0] = disp[idim];
    if (idim == 6) dispmat[1][0] = disp[idim];

    matmul();
    writepos(newaxis, fp);

    fprintf(fp,"press=`grep -B1 'external pressure' OUTCAR|head -1`\n");
    fprintf(fp,"pxx=`echo ${press}|awk '{print $3}'`\n");
    fprintf(fp,"pyy=`echo ${press}|awk '{print $4}'`\n");
    fprintf(fp,"pzz=`echo ${press}|awk '{print $5}'`\n");
    fprintf(fp,"pxy=`echo ${press}|awk '{print $6}'`\n");
    fprintf(fp,"pyz=`echo ${press}|awk '{print $7}'`\n");
    fprintf(fp,"pxz=`echo ${press}|awk '{print $8}'`\n");
    fprintf(fp,"C1%dpos=`echo ${pxx} ${pxx0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"C2%dpos=`echo ${pyy} ${pyy0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"C3%dpos=`echo ${pzz} ${pzz0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"C4%dpos=`echo ${pyz} ${pyz0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"C5%dpos=`echo ${pxz} ${pxz0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"C6%dpos=`echo ${pxy} ${pxy0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"eng%dp=`grep 'energy  without' OUTCAR|tail -1|awk '{print $4}'`\n", idim);
    fprintf(fp,"echo %c%d %g  ${pxx} ${pyy} ${pzz} ${pxy} ${pxz} ${pyz} ${eng%dp}%c\n", char(34), idim, eps[idim], idim, char(34));
    fprintf(fp,"echo %c%d %g  ${pxx} ${pyy} ${pzz} ${pxy} ${pxz} ${pyz} ${eng%dp}%c >> info.dat\n", char(34), idim, eps[idim], idim, char(34));

    eps[idim] = -disp[idim];
    fprintf(fp,"# Now to compute that for eps = [%g %g %g %g %g %g]\necho\n",  eps[1], eps[2], eps[3], eps[4], eps[5], eps[6]);
    fprintf(fp,"echo %cNow to compute that for eps = [%g %g %g %g %g %g]%c\n", char(34), eps[1], eps[2], eps[3], eps[4], eps[5], eps[6], char(34));
    fprintf(fp,"eps=%c%lg%c\n", char(34), -disp[idim], char(34));
    if (idim <= 3) dispmat[idim-1][idim-1] -= disp[idim] + disp[idim];
    if (idim == 4) dispmat[2][1] = -disp[idim];
    if (idim == 5) dispmat[2][0] = -disp[idim];
    if (idim == 6) dispmat[1][0] = -disp[idim];

    matmul();
    writepos(newaxis, fp);

    fprintf(fp,"press=`grep -B1 'external pressure' OUTCAR|head -1`\n");
    fprintf(fp,"pxx=`echo ${press}|awk '{print $3}'`\n");
    fprintf(fp,"pyy=`echo ${press}|awk '{print $4}'`\n");
    fprintf(fp,"pzz=`echo ${press}|awk '{print $5}'`\n");
    fprintf(fp,"pxy=`echo ${press}|awk '{print $6}'`\n");
    fprintf(fp,"pyz=`echo ${press}|awk '{print $7}'`\n");
    fprintf(fp,"pxz=`echo ${press}|awk '{print $8}'`\n");
    fprintf(fp,"C1%dneg=`echo ${pxx} ${pxx0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"C2%dneg=`echo ${pyy} ${pyy0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"C3%dneg=`echo ${pzz} ${pzz0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"C4%dneg=`echo ${pyz} ${pyz0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"C5%dneg=`echo ${pxz} ${pxz0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"C6%dneg=`echo ${pxy} ${pxy0} ${eps} | awk '{print ($2 - $1)/$3}'`\n", idim);
    fprintf(fp,"eng%dn=`grep 'energy  without' OUTCAR|tail -1|awk '{print $4}'`\n", idim);
    fprintf(fp,"echo %c%d %g  ${pxx} ${pyy} ${pzz} ${pxy} ${pxz} ${pyz} ${eng%dp}%c\n", char(34), idim, eps[idim], idim, char(34));
    fprintf(fp,"echo %c%d %g  ${pxx} ${pyy} ${pzz} ${pxy} ${pxz} ${pyz} ${eng%dn}%c >> info.dat\n", char(34), idim, eps[idim], idim, char(34));

    fprintf(fp,"C1%d=`echo ${C1%dpos} ${C1%dneg}|awk '{print ($1+$2)/2}'`\n", idim, idim, idim);
    fprintf(fp,"C2%d=`echo ${C2%dpos} ${C2%dneg}|awk '{print ($1+$2)/2}'`\n", idim, idim, idim);
    fprintf(fp,"C3%d=`echo ${C3%dpos} ${C3%dneg}|awk '{print ($1+$2)/2}'`\n", idim, idim, idim);
    fprintf(fp,"C4%d=`echo ${C4%dpos} ${C4%dneg}|awk '{print ($1+$2)/2}'`\n", idim, idim, idim);
    fprintf(fp,"C5%d=`echo ${C5%dpos} ${C5%dneg}|awk '{print ($1+$2)/2}'`\n", idim, idim, idim);
    fprintf(fp,"C6%d=`echo ${C6%dpos} ${C6%dneg}|awk '{print ($1+$2)/2}'`\n", idim, idim, idim);
  }

  fprintf(fp,"C11all=`echo ${C11}|awk '{print $1/10}'`\n");
  fprintf(fp,"C22all=`echo ${C22}|awk '{print $1/10}'`\n");
  fprintf(fp,"C33all=`echo ${C33}|awk '{print $1/10}'`\n");
  fprintf(fp,"C12all=`echo ${C12} ${C21}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C13all=`echo ${C13} ${C31}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C23all=`echo ${C23} ${C32}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C44all=`echo ${C44}|awk '{print $1/10}'`\n");
  fprintf(fp,"C55all=`echo ${C55}|awk '{print $1/10}'`\n");
  fprintf(fp,"C66all=`echo ${C66}|awk '{print $1/10}'`\n");

  fprintf(fp,"C14all=`echo ${C14} ${C41}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C15all=`echo ${C15} ${C51}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C16all=`echo ${C16} ${C61}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C24all=`echo ${C24} ${C42}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C25all=`echo ${C25} ${C52}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C26all=`echo ${C26} ${C62}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C34all=`echo ${C34} ${C43}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C35all=`echo ${C35} ${C53}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C36all=`echo ${C36} ${C63}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C45all=`echo ${C45} ${C54}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C46all=`echo ${C46} ${C64}|awk '{print ($1+$2)/20}'`\n");
  fprintf(fp,"C56all=`echo ${C56} ${C65}|awk '{print ($1+$2)/20}'`\n");

  fprintf(fp, "echo Elastic Constant C11 = ${C11all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C22 = ${C22all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C33 = ${C33all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C12 = ${C12all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C13 = ${C13all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C23 = ${C23all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C44 = ${C44all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C55 = ${C55all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C66 = ${C66all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C14 = ${C14all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C15 = ${C15all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C16 = ${C16all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C24 = ${C24all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C25 = ${C25all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C26 = ${C26all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C34 = ${C34all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C35 = ${C35all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C36 = ${C36all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C45 = ${C45all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C46 = ${C46all} GPa >> info.dat\n");
  fprintf(fp, "echo Elastic Constant C56 = ${C56all} GPa >> info.dat\n");
  fprintf(fp, "echo # The elastic constant matrix:       >> info.dat\n");
  fprintf(fp, "echo ${C11all}|awk '{printf %c%%9.4f\\n%c, $1}' >> info.dat\n", char(34),char(34));
  fprintf(fp, "echo ${C12all} ${C22all}|awk '{printf %c%%9.4f %%9.4f\\n%c, $1,$2}' >> info.dat\n", char(34),char(34));
  fprintf(fp, "echo ${C13all} ${C23all} ${C33all}|awk '{printf %c%%9.4f %%9.4f %%9.4f\\n%c, $1,$2,$3}' >> info.dat\n", char(34),char(34));
  fprintf(fp, "echo ${C14all} ${C24all} ${C34all} ${C44all}|awk '{printf %c%%9.4f %%9.4f %%9.4f %%9.4f\\n%c, $1,$2,$3,$4}' >> info.dat\n", char(34),char(34));
  fprintf(fp, "echo ${C15all} ${C25all} ${C35all} ${C45all} ${C55all}|awk '{printf %c%%9.4f %%9.4f %%9.4f %%9.4f %%9.4f\\n%c, $1,$2,$3,$4,$5}' >> info.dat\n", char(34),char(34));
  fprintf(fp, "echo ${C16all} ${C26all} ${C36all} ${C46all} ${C56all} ${C66all}|awk '{printf %c%%9.4f %%9.4f %%9.4f %%9.4f %%9.4f %%9.4f\\n%c, $1,$2,$3,$4,$5,$6}' >> info.dat\n", char(34),char(34));
  fprintf(fp, "\ncat info.dat\n\n");
  fprintf(fp, "rm -rf CHG* CONTCAR EIGENVAL IBZKPT OSZICAR OUTCAR PCDAT vasprun.xml WAVECAR XDATCAR\n");
  fprintf(fp, "#\nexit 0\n");

  char str[MAXLINE];
  sprintf(str, "chmod +x ./%s", fname);
  system(str);

return;
}

/*------------------------------------------------------------------------------
 * Method to write one frame of the dump file to a new file
 *------------------------------------------------------------------------------ */
void Driver::matmul()
{
  for (int i = 0; i < 3; ++i)
  for (int j = 0; j < 3; ++j) newaxis[i][j] = 0.;

  for (int i = 0; i < 3; ++i)
  for (int j = 0; j < 3; ++j){
    for (int m = 0; m <3; ++m) newaxis[i][j] += axis[i][m] * dispmat[m][j];
  }
return;
}

/*------------------------------------------------------------------------------
 * Method to write one frame of the dump file to a new file
 *------------------------------------------------------------------------------ */
void Driver::writepos(double **ax, FILE *fp)
{
  fprintf(fp,"cat > POSCAR << EOF\n");
  fprintf(fp,"%s%20.14f\n", title, alat);
  for (int i = 0; i < 3; ++i) fprintf(fp,"%20.14f %20.14f %20.14f\n", ax[i][0], ax[i][1], ax[i][2]);
  if (element) fprintf(fp,"%s", element);
  for (int i = 0; i < ntype; ++i) fprintf(fp, "%d ", ntm[i]);
  fprintf(fp,"\nDirect\n");
  for (int i = 0; i < natom; ++i) fprintf(fp, "%20.14f %20.14f %20.14f\n", atpos[i][0], atpos[i][1], atpos[i][2]);
  fprintf(fp,"EOF\ncat POSCAR\n# Now to do the calculations\nrm -rf WAVECAR\n${VASP}\n");

return;
}

/*------------------------------------------------------------------------------
 * To display help info
 *------------------------------------------------------------------------------ */
void Driver::help()
{
  printf("\nCode to generate the script for elastic constants calculations based on vasp.\n");
  printf("\nUsage:\n    ecvasp [options] [poscar]\n\nAvailable options:\n");
  printf("    -h       To display this help info;\n");
  printf("    -e       To define the default strain;\n");
  printf("    -xx      To define the default strain of eps_{xx};\n");
  printf("    -yy      To define the default strain of eps_{yy};\n");
  printf("    -zz      To define the default strain of eps_{zz};\n");
  printf("    -xy      To define the default strain of eps_{xy};\n");
  printf("    -xz      To define the default strain of eps_{xz};\n");
  printf("    -yz      To define the default strain of eps_{yz};\n");
  printf("    poscar   POSCAR or CONTCAR of vasp; by default: POSCAR\n");
  printf("\n\n");

  exit(0);
return;
}

/*------------------------------------------------------------------------------
 * Method to count # of words in a string, without destroying the string
 *------------------------------------------------------------------------------ */
int Driver::count_words(const char *line)
{
  int n = strlen(line) + 1;
  char *copy;
  memory->create(copy, n, "copy");
  strcpy(copy,line);

  char *ptr;
  if (ptr = strchr(copy,'#')) *ptr = '\0';

  if (strtok(copy," \t\n\r\f") == NULL) {
    memory->sfree(copy);
    return 0;
  }
  n = 1;
  while (strtok(NULL," \t\n\r\f")) ++n;

  memory->sfree(copy);
  return n;
}

/*----------------------------------------------------------------------------*/
