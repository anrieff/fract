#include <stdio.h>
#include <math.h>
#include <string.h>
#include <algorithm>
#include "../src/vector3.h"

FILE *infile;
Vector p[4];
Vector center;

static void usage(void)
{
	printf("find_circumscribed_sphere - Finds the sphere, passing through four 3D-points\n");
	printf("Usage:\n\n");
	printf("\tfind_circumscribed_sphere [coords_file] [-help]\n\n");
	printf("\t-help        - display this help\n");
	printf("\tcoords_file  - use this file as source for coordinates. If not given, stdin is used.\n");
}

static bool parse_cmdline(int argc, char **argv)
{
	infile = stdin;
	bool filegiven = false;
	for (int i = 1; i < argc; i++) {
		bool recognized = false;
		if ( !strcmp(argv[i], "-h") || !strcmp(argv[i], "-h") || !strcmp(argv[i], "-h")) {
			usage();
			return false;
		}
		
		if (!recognized) {
			if (filegiven) {
				printf("Stray arg: `%s' - you can process one file only!\n", argv[i]);
				printf("Use `find_circubscribed_sphere -help' to get help\n");
				return false;
			}
			infile = fopen(argv[i], "rt");
			if (!infile) {
				printf("Couldn't open `%s'!\n", argv[i]);
				return false;
			}
			filegiven = true;
		}
	}
	return true;
}

static bool read_file(void)
{
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++) {
			if (1 != fscanf(infile, "%lf", &p[i][j])) {
				char s[1000];
				fscanf(infile, "%s", s);
				printf("Error reading coords: I don't understand this `%s'\n", s);
				return false;
			}
		}
	return true;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

double get_mult(int a[] )
{
	int c = 0;
	for (int i = 0; i < 4; i++)
		for (int j = i+1; j < 4; j++)
			c += (a[i] > a[j]);
	if (c % 2) return -1;
	else return +1;
}

class Matrix4x4 {
	double a[4][4];
public:
	Matrix4x4() { memset(a, 0, sizeof(a)); }
	void set(int i, int j, double x) { a[i][j] = x; }
	double det(void)
	{
		double sum = 0.0;
		int ord[4] = {0, 1, 2, 3};
		do {
			sum += get_mult(ord) * a[0][ord[0]] * a[1][ord[1]] * a[2][ord[2]] * a[3][ord[3]];
		} while (std::next_permutation(ord, ord+4));
		return sum;
	}
};

static bool find_sphere(void)
{
	Vector a(p[0]), b(p[1]), c(p[2]), d(p[3]);
	Vector u = (b-a), v = (c-a);
	Vector normal = u ^ v;

	double Q = - (a * normal);

	if (fabs(Q + (d * normal)) < 1e-9) return false;

	Vector all[4] = {a,b,c,d};

	Matrix4x4 A;

	for (int i = 0; i < 4; i++) {
		A.set(i, 3, 1);
		for (int j = 0; j < 3; j++) A.set(i, j, all[i][j]);
	}
	double alpha = A.det();

	for (int i = 0; i < 4; i++) {
		A.set(i, 0, all[i].lengthSqr());
		A.set(i, 3, 1);
	}

	int indices[3][3] = { {1, 2}, {0, 2}, {0, 1} };
	double D[3];

	for (int i = 0; i < 3; i++) {
		for (int j = 0;  j < 4; j++) {
			A.set(j, 1, all[j][indices[i][0]]);
			A.set(j, 2, all[j][indices[i][1]]);
		}
		D[i] = A.det();
	}
	D[1] = - D[1];
	alpha = 1 / (2.0 *alpha);
	center = Vector(D[0] * alpha, D[1] * alpha, D[2] * alpha);
	
	return true;

}

int main(int argc, char ** argv)
{
	if (!parse_cmdline(argc, argv)) return -1;
	if (!read_file()) return -1;
	if (find_sphere()) {
		printf("Circumsphere center is in (%.4lf, %.4lf, %.4lf)\n", center[0], center[1], center[2]);
		printf("Circumradius is %.5lf\n", center.distto(p[0]));
	} else {
		printf("error: points are coplanar!\n");
	}
	fclose(infile);
	return 0;
}
