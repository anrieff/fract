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


frameinfo cd[MAXCD];
int c;

void DoWork(void)
{
	int i;
	frameinfo fi;
	double t, tr;
	printf("Time rescaling factor: ");
	scanf("%lf", &tr);
	tr = 1/tr;
	t = 0;
	char infile[256], outfile[256];
	printf("In file: "); scanf("%s", infile);
	printf("Out file: "); scanf("%s", outfile);
	freopen(infile, "rt", stdin);
	freopen(outfile, "wt", stdout);
	c = 0;
	while (scanf("%lf%lf%lf%lf%lf%lf%lf%lf%lf%d%f", 	&fi.time  , fi.cam, fi.cam+1,
	       fi.cam+2, &fi.alpha, &fi.beta, fi.light, fi.light+1, fi.light+2,
	       &fi.pp_state, &fi.shader_param)==11) {
		       cd[c++] = fi;
	       }
	for (t = 0; t < c - 1; t += tr) {
		int l = (int) t;
		float f = t - l;
		fi = cd[l];
		fi.time = cd[l].time * (1-f) + cd[l+1].time * f;
		fi.alpha = cd[l].alpha * (1-f) + cd[l+1].alpha * f;
		fi.beta = cd[l].beta * (1-f) + cd[l+1].beta * f;
		for (i = 0; i < 3; i++) {
			fi.cam[i] = cd[l].cam[i] * (1 - f) + cd[l+1].cam[i] * f;
		}
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
