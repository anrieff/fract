

/*
#declare ffu = function(ffx, pover, mult) {pow((ffx)/mult, pover)}

#macro heart1(HX, HY, HZ, m, c1, c2, c3, c4)
isosurface {
	function {
		pow((c1*ffu(x-HX,2,m)+c2*ffu(z-HZ,2,m)+ffu(y-HY,2,m) - 1), 3)
		- (c3*ffu(x-HX,2,m) + c4*ffu(z-HZ,2,m))*ffu(y-HY,3,m)
	}
}
#macro heart2(HX, HY, HZ, m, c1, c2, c3, c4)
isosurface {
	function {
		pow((c1*ffu(x-HX,2,m)+c2*ffu(z-HZ,2,m)+ffu(y-HY,2,m) - 1), 3)
		- (c3*ffu(x-HX,2,m) + c4*ffu(z-HZ,2,m))*ffu(y-HY,3,m)
	}
}

heart1(0, -6.5, 5, 3.5, 1, 9/4, 1, 9/80)
heart2(8, -6.5, -1, 2.2, 9/4, 1, 9/80 ,1)
*/

#include <stdio.h>
#include <math.h>

#define MIN_COORDINATE -10.0
#define MAX_COORDINATE +10.0
#define steps 100

int vseg, hseg;
const double radius = 100.0;


double ffu(double ffx, double pover, double mult)
{
	return pow(ffx/mult, pover);
}

double heart1(double x, double y, double z, double HX, double HY, double HZ, double m, double c1, double c2, double c3, double c4)
{
	return pow((c1*ffu(x-HX,2,m)+c2*ffu(z-HZ,2,m)+ffu(y-HY,2,m) - 1), 3)
		- (c3*ffu(x-HX,2,m) + c4*ffu(z-HZ,2,m))*ffu(y-HY,3,m);

}

double heart2(double x, double y, double z, double HX, double HY, double HZ, double m, double c1, double c2, double c3, double c4)
{
	return	pow((c1*ffu(x-HX,2,m)+c2*ffu(z-HZ,2,m)+ffu(y-HY,2,m) - 1), 3)
		- (c3*ffu(x-HX,2,m) + c4*ffu(z-HZ,2,m))*ffu(y-HY,3,m);

}


double fun(double x, double y, double z)
{
	return heart1(x, y, z, 0, 0, 0, 2.2, 9/4.0, 1, 9/80.0 ,1);
}

double X[2], Y[2], Z[2];
#define prep(a, A, nA) a[2] = a[0]=A[0]; a[1] = (A[1]-A[0])/steps; nA[0]=1e9; nA[1]=-1e9
#define FOR(a, A) for(a[0]=a[2];a[0]<=A[1];a[0]+=a[1])
#define assign(a, nA) a[0]=nA[0]; a[1] = nA[1]
#define pd(A) printf(" (%.4lf - %.4lf)", A[0], A[1]);

/*void dowork(void)
{
	X[0] = Y[0] = Z[0] = MIN_COORDINATE;
	X[1] = Y[1] = Z[1] = MAX_COORDINATE;
	double nX[2], nY[2], nZ[2];
	double x[3], y[3], z[3];
	for (int k = 0; k < 5; k++) {
		printf("Step %d of %d...", k+1, 5); fflush(stdout);
		prep(x, X, nX);
		prep(y, Y, nY);
		prep(z, Z, nZ);
		FOR(x,X) FOR(y,Y) FOR(z,Z) {
			double t = fun (x[0], y[0], z[0]);
			if (t<0) {
				nX[0] <?= x[0]-x[1];
				nX[1] >?= x[0]+x[1];
				nY[0] <?= y[0]-y[1];
				nY[1] >?= y[0]+y[1];
				nZ[0] <?= z[0]-z[1];
				nZ[1] >?= z[0]+z[1];
			}
		}

		assign(X, nX);
		assign(Y, nY);
		assign(Z, nZ);
		printf("\n");
	}
	printf("The bounding box is:");
	pd(X); pd(Y); pd(Z);
	printf("\n");
}*/

#define id0(y,x) ((y)*hseg+(x)+1)
#define id1(y,x) ((y)*(hseg+1)+(x)+1)

void generate(void)
{
	printf("##\n# File generated with Veselin Georgiev's tesselator\n##\n\n");
	printf("mtllib heart.mtl\n");
	printf("#vertices:\n");
	for (int i = 0; i <= vseg; i++)
	{
		for (int j = 0; j < hseg; j++) {
			double ux = -cos(2.0 * M_PI * j / hseg) * sin(M_PI * i /vseg);
			double uy = cos(M_PI * i / vseg);
			double uz = -sin(2.0 * M_PI * j / hseg) * sin(M_PI * i /vseg);
			double r = radius;
			double l = 0;
			while (r - l > 1e-9) {
				double m = (r+l) * 0.5;
				if (fun(ux * m, uy * m, uz * m)>0) {
					r = m;
				} else {
					l = m;
				}
			}
			printf("v %10.6lf %10.6lf %10.6lf\n", ux * r, uy * r, uz * r);
		}
	}
	printf("\n\n#texture vertices:\n");
	for (int i = 0; i <= vseg; i++)
	{
		for (int j = 0; j <= hseg; j++) {
			printf("vt %8.4lf %8.4lf\n", (double) j / hseg, (double) i / vseg);
		}
	}
	printf("\n\n#Triangles:\n");
	for (int i = 0; i < vseg; i++) {
		for (int j = 0; j < hseg; j++) {
			int jp1 = (j+1) % hseg;
			int jj = i ? j : 0;
			if (i < vseg-1)
			printf("f %d/%d %d/%d %d/%d\n",
				id0(i  , jj ), id1(i  , j  ),
				id0(i+1, jp1), id1(i+1, j+1),
				id0(i+1, j  ), id1(i+1, j  ));
			int jjj = i < vseg - 1 ? jp1 : 0;
			if (i > 0)
			printf("f %d/%d %d/%d %d/%d\n",
				id0(i  , j  ), id1(i  , j  ),
				id0(i  , jp1), id1(i  , j+1),
				id0(i+1, jjj), id1(i+1, j+1));

		}
	}
}

int main(void)
{
	/*fprintf(stderr, "Vertical   segments: ");*/ scanf("%d", &vseg);
	/*fprintf(stderr, "Horizontal segments: ");*/ scanf("%d", &hseg);
	generate();

	return 0;
}
