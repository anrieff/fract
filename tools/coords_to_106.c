#include <stdio.h>

#define MAXCD 5000

typedef struct {
	double time;
	double cam[3];
	double alpha, beta;
	// new in 1.06:
	
	double light[3];
	int pp_state;
	float shader_param;
}frameinfo;




void DoWork(void)
{
	frameinfo fi;
	printf("Light.Coords: ");
	scanf("%lf%lf%lf", fi.light, fi.light + 1, fi.light + 2);
	printf("Shader: "); scanf("%d", &fi.pp_state);
	printf("Shader param: "); scanf("%f", &fi.shader_param);
	char infile[256], outfile[256];
	printf("In file: "); scanf("%s", infile);
	printf("Out file: "); scanf("%s", outfile);
	freopen(infile, "rt", stdin);
	freopen(outfile, "wt", stdout);
	while (scanf("%lf%lf%lf%lf%lf%lf", 	&fi.time  , fi.cam, fi.cam+1,
	       fi.cam+2, &fi.alpha, &fi.beta)==6) 
	{
		printf("%.3lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%d\t%.4f\n",
			fi.time,
			fi.cam[0], fi.cam[1], fi.cam[2],
			fi.alpha,
			fi.beta,
			fi.light[0], fi.light[1], fi.light[2],
			fi.pp_state,
			fi.shader_param);
	}
}



int main(void)
{
	DoWork();
	return 0;
}
