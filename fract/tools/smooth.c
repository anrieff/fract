#include <stdio.h>

#define MAXN 5000
#define X 6

double a[MAXN][X];

int ss, n;

void DoWork(void)
{
    int i,j,k;
    double f[X], fi[X];
    
    for (j=0;j+ss<n;j+=ss) {
	for (i=0;i<X;i++) {
	    fi[i] = (a[j+ss][i] - a[j][i]) / (double ) ss;
	    f [i] = a[j][i];
	}
	for (k=0;k<ss;k++) {
	    for (i=0;i<X;i++) {
		a[j+k][i] = f[i];
		if (i) f[i]+=fi[i];
	    }
	}
    }
}

int main(int argc, char **argv)
{
    FILE *f;
    int i;
    if (argc != 3) {
	printf("Usage: smooth <in-file> <smooth-sampling>\nWhere the `in-file' can be any file and `smooth-sampling' is an integer\n\n");
	return 1;
    }
    if ((f = fopen(argv[1], "rt"))==NULL) {
	printf("Error: `%s' does not exist!\n\n");
	return 2;
    }
    if (!sscanf(argv[2], "%d", &ss)) {
	printf("Error: please, integer smooth-sampling only!\n\n");
	return 3;
    }
    while (fscanf(f, "%lF%lf%lf%lf%lf%lf", a[n], a[n]+1, a[n]+2, a[n]+3, a[n]+4, a[n]+5)==X) n++;
    fclose(f);
    DoWork();
//    f = fopen(argv[1], "wt");
    for (i=0;i<n;i++)
	printf( "%.3lf \t%.6lf \t%.6lf \t%.6lf \t%.6lf \t%.6lf\n", a[i][0], a[i][1], a[i][2], a[i][3], a[i][4], a[i][5]);
//    fclose(f);
    return 0;
}
