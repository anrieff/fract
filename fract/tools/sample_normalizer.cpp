#include <stdio.h>
#include <vector>
using namespace std;
struct sample {
    double x, y, w;
};
vector<sample> v;

int main(void)
{
    sample a;
    while (3 == scanf("%lf%lf%lf", &a.x, &a.y, &a.w)) {
	v.push_back(a);
    }
    double sum = 0.0;
    int n = v.size();
    for (int i = 0;i < n ; i++) {
	sum += v[i].w;
    }
    sum = 1/sum;
    printf("static const fsaa_kernel aaa_kernel_%d = {\n", n);
    printf("\t{\n");
    for (int i = 0; i < n; i++)
	printf("\t\t{ %.3lf, %.3lf },\n", v[i].x, v[i].y);
    printf("\t},\n");
    printf("\t{");
    for (int i = 0; i < n; i++) {
	if (i % 4 == 0) {
	    printf("\n\t\t");
	}
	printf("%.4lf, ", v[i].w  * sum);
    }
    printf("\n\t},\n\t%d\n};\n", n);
    return 0;
}
