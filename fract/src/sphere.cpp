/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *                                                                         *
 *   sphere.cpp - handles sphere creation (the fractal formula) and        *
 *                sorting                                                  *
 *                                                                         *
 ***************************************************************************/


#include <math.h>
#include <stdlib.h>
#include "MyGlobal.h"
#include "MyTypes.h"
#include "antialias.h"
#include "cmdline.h"
#include "common.h"
#include "fract.h"
#include "gfx.h"
#include "profile.h"
#include "random.h"
#include "render.h"
#include "sphere.h"
#include "shadows.h"
#include "mesh.h"
#include "vectormath.h"

Sphere *a;
extern int spherecount, r_shadows;
extern Sphere *sp;

int n;
int defaultflags=0;

AniSphere AS[100];
int anicnt;
const char *earthmap = "data/800.bmp";
double speed_spec = 0.3;
double time_hit = 999999;

void sphere_check_for_first_hit(void)
{
	static bool fh = true;
	if (fh) {
		fh = false;
		time_hit = bTime();
	}
}

// decompose a RGB value to its components
void setcolor(Sphere *a, int col)
{
 a->b = (col & 0xff);
 col >>= 8;
 a->g = (col & 0xff);
 col >>= 8;
 a->r = (col & 0xff);
}

int rand_color(void)
{
    return (0x606060 + ((rand()%0x1000000)&0x7f7f7f));
}
int isec = 0;
int intersect(Sphere *a)
{
    int i;

    for (i=0;i<n;i++)
        if (a[i].pos.distto(a[n].pos) < a[i].d + a[n].d) {
            fflush(stdout);
            isec++;
            if (isec > 100)  {
                printf("A:\n");
                print_sphere_info(stdout, a+i);
                printf("B:\n");
                print_sphere_info(stdout, a+n);
                exit(1);
                }
            return 1;
            }

    return 0;
}

void set_params(Sphere *a, double x, double y, double z, double d, double xi, double yi, double zi)
{
 a->pos = Vector(x , y , z );
 a->mov = Vector(xi, yi, zi);
 a->d = d;
 a->mass = 1.0;
 a->refl = 0.25;

 setcolor(a, rand()%0x1000000);
 a->flags = defaultflags|SEETHROUGH;
 a->opacity = 0.9;
 a->AniIndex = 0;
}

// the recursive routine to create the fractal
void recu(Sphere *a, double x, double y, double z, double r, int level)
{
 if (level == 0) {
	a[n].pos = Vector(x, y, z);
	a[n].d = r;
	a[n].flags = RAYTRACED|COLLIDEABLE/*|SEETHROUGH*/|RECURSIVE|STATIC;
	a[n].mass = 1;
	a[n].opacity = 0.90;
	//this->tex = new RawImg(earthmap);
	a[n].refl = 0.3;
	if (r<10)
	setcolor(a+n,multiplycolorf(KOLOR1, r/8.5) + multiplycolorf(KOLOR2, (1-r/8.5)));
	else setcolor(a+n, KOLOR3);
	n++;
	return;
 	}

 recu(a, x       , y       , z+0.4*r, r*0.5, level-1);
 recu(a, x       , y       , z-0.7*r, r*0.3, level-1);
 recu(a, x+0.7*r , y       , z      , r*0.3, level-1);
 recu(a, x-0.7*r , y       , z      , r*0.3, level-1);
 recu(a, x       , y+0.7*r , z      , r*0.3, level-1);
 recu(a, x       , y-0.7*r , z      , r*0.3, level-1);

}

void create_fract_array2(Sphere *a, int *count)
{
	for (int i = 0; i < 6; i++) {
		double ang = M_PI * 2 * i / 6.0;
		a[*count].d = 16;
		a[*count].refl = 0.25;
		a[*count].flags = RAYTRACED /*| RECURSIVE */| STATIC;
		a[*count].opacity = 1.0;
		a[*count].pos = Vector(120, 15, 200) + Vector(75 * sin(ang), 0, 75 * cos(ang));
		setcolor (a + *count, rand_color());
		
		(*count)++;
	}
}

void create_fract_array1(Sphere *a, int *count)
{int j;
 double M_SPEED=2.5;

 if (option_exists("--speed"))
    M_SPEED = option_value_float("--speed");
 n = 0;
 srand(5);
 recu(a, 200, 95, 330, 26, 0);
 a[n].d = 15;
 a[n].refl = 0.01;
 a[n].flags = RAYTRACED|COLLIDEABLE|SEETHROUGH;
 a[n].flags |= RECURSIVE;
 //a[n].flags |= SEETHROUGH;
 a[n].mass = 2;
 a[n].opacity = 0.77;
 setcolor(a+n, rand_color());
 a[n].pos = Vector(201.0, 170.0, 325.0);
 a[n].mov.zero();
 a[n].time = 0;
 n++;


 for (j=0;j<10;j++) {
    do {
     	a[n].d = rand()%4 + 6;
    	a[n].refl = 0.28;
    	a[n].flags = RAYTRACED|RECURSIVE|ANIMATED|COLLIDEABLE;//|SEETHROUGH;
    	a[n].AniIndex = n;
    	a[n].mass = 1;
	a[n].opacity = 0.9;
    	setcolor(a+n, rand_color());
    	AS[n].alpha = (double) (rand()%628) * 0.01;
    	//AS[n].beta =  (double) (rand()%314) * 0.01 - M_PI/2.0;
    	AS[n].beta = (double) (rand()%50)*0.01 - 0.25;
    	AS[n].speed = (0.2 + (rand()%20)*0.1)*0.11*M_SPEED;
	if (rand()%2) AS[n].speed = -AS[n].speed;
	AS[n].betaspeed = (0.2 + (rand()%20)*0.1)*0.11*M_SPEED;
	if (rand()%2) AS[n].betaspeed = -AS[n].betaspeed;
	//AS[n].speed *= speed_spec;
	//AS[n].betaspeed *= speed_spec;
    	AS[n].radius = 36 + rand()%60;
    	AS[n].cx = a[0].pos[0];
    	AS[n].cy = a[0].pos[1];
    	AS[n].cz = a[0].pos[2];
    	calculate_XYZ(a+n, 0, a[n].pos);
//	calculate_XYZ(a+n, (rand()%1000) * 0.01, oo);
    	} while (intersect(a));
	n++;
 	}
	//	

	a[n].d = 8;
	a[n].flags = STATIC | COLLIDEABLE;
	a[n].mass = 10000;
	a[n].pos = Vector(200, 8, 330);
	n++;
	
 anicnt = n;
 *count = n;
 }
 
void create_fract_array(Sphere *a, int *count)
{
	create_fract_array2(a, count);
}

void create_1fract_array(Sphere *a, int *count)
{
	*count = 1;
	a[0].d = 10;
	a[0].pos = Vector(0, 10, 0);
	a[0].flags = RAYTRACED|STATIC|SEETHROUGH;
	a[0].mass = 1;
	a[0].opacity = 0.9;
	a[0].refl = 0.008;
	setcolor(a, rand_color());
}

void create_voxel_sphere_array(Sphere *a, int *count)
{
	/**count = 3;
	a[0].d = 6;
	a[0].refl = 0.82;
	a[0].flags = RAYTRACED|RECURSIVE;
	a[0].opacity = 1.0;
	setcolor(a, rand_color());
	a[0].pos = Vector(215.0, 26.0, 329.0);
	a[0].mov.zero();
	a[0].time = 0;
	a[1] = a[0];
	a[1].pos = Vector(204.0, 29.0, 330.0);
	a[2] = a[1];
	a[2].pos = Vector(216.0, 42.0, 309.0);
	a[2].flags |= SEETHROUGH;
	a[2].opacity = 0.66;
	a[2].refl = 0.5;
	a[2].r = 64; a[2].g = 94; a[2].b = 246;
	a[2].d = 10.0;*/
	*count = 1;
	a[0].d = 6;
	a[0].flags = RAYTRACED|SEETHROUGH;
	a[0].pos = Vector(216.0000, 42.0000, 347.0000);
	a[0].opacity = 1.0;
	a[0].refl = 0.082;
	a[0].r = 64; a[0].g = 94; a[0].b = 246;
	a[0].d = 10.0;
}
/*
void create_fract_array(Sphere *a, int *count)
{
 srand(5);
 defaultflags = GRAVITY|AIR|COLLIDEABLE;
 set_params(a+0, 400, 222, 360, 10, 0, 0, 0);
 set_params(a+1, 350, 222, 360, 10, 0, 0, 0);
 set_params(a+2, 375, 222, 390, 10, 0, 0, 0);
 set_params(a+3, 375, 222, 330, 10, 0, 0, 0);

 set_params(a+4, 375, 272, 361, 30, 0, 0, 0);
 a[0].mass = 4;
 a[1].mass = 4;
 a[2].mass = 4;
 a[3].mass = 4;
 a[4].mass = 1;
 *count = 5;
}
*/
/*
	This is used for animation.
	The idea is, that each sphere has its `AniSphere index', which is an index to the table, where
	animation data is stored. This function determines where a sphere should be at a specific time,
	according to the `animation data'.
*/
void calculate_XYZ(Sphere *a, double time, Vector& o)
{
	double alpha, beta, speed, betaspeed;
	int num = a->AniIndex;

	alpha = AS[num].alpha;
	beta = AS[num].beta;
	speed = AS[num].speed;
	betaspeed = AS[num].betaspeed;
	if (betaspeed == 0) {
	o = Vector(
		AS[num].cx + AS[num].radius*(sin(alpha)*cos(time*speed) + cos(alpha) * sin(time*speed) * sin(beta)),
		AS[num].cy + AS[num].radius*(sin(time*speed) * cos(beta)),
		AS[num].cz + AS[num].radius*(cos(alpha)*cos(time*speed) - sin(alpha) * sin(time*speed) * sin(beta))
		);
	} else {
		speed = AS[num].speed;
		betaspeed = AS[num].betaspeed;
		double tbh = time > time_hit ? time_hit : time;
		double tah = time > time_hit ? time-time_hit : 0;
		alpha = AS[num].alpha + tbh * AS[num].speed * speed_spec + tah * AS[num].speed;
		beta =  AS[num].beta + tbh * AS[num].betaspeed * speed_spec + tah * AS[num].betaspeed;
		o = Vector(
				AS[num].cx + AS[num].radius * (cos(alpha) * cos(beta)),
				AS[num].cy + AS[num].radius * sin(beta),
				AS[num].cz + AS[num].radius * (sin(alpha) * cos(beta)));
	}
}

void print_sphere_info(FILE *f, Sphere *a)
{
    fprintf(f, " ->(x , y , z ) = (%.4lf, %.4lf, %.4lf)\n", a->pos[0], a->pos[1], a->pos[2]);
    fprintf(f, " ->(xi, yi, zi) = (%.4lf, %.4lf, %.4lf)\n", a->mov[0], a->mov[1], a->mov[2]);
    fprintf(f, " ->flags = 0x%x\n", a->flags);
    fprintf(f, " ->radius = %.4lf\n", a->d);
    fprintf(f, " ->mass = %.3lf\n", a->mass);
    fprintf(f, " ->time = %.4lf\n", a->time);
    fflush(stdout);
}

/*
	Sphere class implementation
*/
double Sphere::get_depth(const Vector &camera)
{
	return pos.distto(camera);
}

bool Sphere::is_visible(const Vector & camera, Vector w[3])
{
	int x, y;
	return project_sphere(this, camera, w, &x, &y, xsize_render(xres()), ysize_render(yres()));
}


int Sphere::calculate_convex(Vector pt[], const Vector& c)
{
	int steps;
	steps = 4;
	while (steps < MAX_SPHERE_SIDES &&
 		this->d*sqrt(2.0f-2.0f*cos(2.0f*M_PI/steps)) / this->dist * xres() > M_SIDE_CONSTANT)
	{
		steps += 4;
	}
	if (steps == MAX_SPHERE_SIDES) steps -= 4;
	if (steps >= MAX_SPHERE_SIDES) steps = MAX_SPHERE_SIDES-1;
	if (steps <  MIN_SPHERE_SIDES) steps = MIN_SPHERE_SIDES;
	
	calculate_fixed_convex(pt, c, steps);

	return steps;
}

void Sphere::calculate_fixed_convex(Vector pt[], const Vector& c, int steps)
{
	double alpha, beta;
	double sina, sinb, cosa, cosb;
	double *f = &PreSC[steps][0][0];
	
	alpha = atan2(this->pos[0] - c[0], this->pos[2] - c[2]) + M_PI / 2;
	beta  = atan2(this->pos[1] - c[1], sqrt(sqr(this->pos[2] - c[2]) + sqr(this->pos[0] - c[0])));
	sina  = sin(alpha);
	sinb  = sin(beta);
	cosa  = cos(alpha);
	cosb  = cos(beta);

	double D = c.distto(this->pos);
	double K = sqrt(D*D-this->d*this->d);
	double K_plus_delta_K = D*D/K;
	double delta_K = K_plus_delta_K - K;
	double delta_D = D*delta_K/K_plus_delta_K;
	Vector newCenter;
	newCenter.make_vector(c, this->pos);
	newCenter.make_length(delta_D);
	newCenter += this->pos;
	double newRadius = sqrt(this->d*this->d - delta_D*delta_D);

	for (int i=0;i<=steps;i++) {
		pt[(i+steps/4*3)%steps] = newCenter + Vector(
				newRadius * (sina * f[1] + cosa * f[0] * sinb),
		newRadius *  f[0] * cosb,
		newRadius * (cosa * f[1] - sina * f[0] * sinb));
		f+=2;
	}
	pt[steps] = pt[0];
}

void Sphere::map2screen(Uint32 *framebuffer, int color, int sides, Vector pt[], const Vector& camera, Vector w[3],
			 int & min_y, int & max_y)
{
	int L[RES_MAXY], R[RES_MAXY];
	int ys, ye, bs, be;
	min_y = ys = accumulate(pt, sides, camera, w, fun_less, 999666111, bs);
	max_y = ye = accumulate(pt, sides, camera, w, fun_more,-999666111, be);
	int size = (be + sides - bs) % sides;
	project_hull_part(L, pt, bs, +1, size, sides, color, camera, w, fun_min);
	project_hull_part(R, pt, bs, -1, sides - size, sides, color, camera, w, fun_max);
	map_hull(framebuffer, L, R, ys, ye, color);
}

/**********************************************************************************
 *    SOLVE3D - calculates the phong shading for a point I, which is the          *
 *              crossing point of the sphere A and the vector V, originating      *
 *              from the camera (point C), with a light source of L.              *
 *              `iteration' is used for raytracing - pass 0 when calling Solve3D  *
 *                                                                                *
 *    Returns:                                                                    *
 *             - a color, depending on the calculated phong and the r, g and b    *
 *               values of the sphere                                             *
 *             - an opacity, double between 0.0 and 1.0. Opacity is 1 anywhere    *
 *               except for the edge of the sphere, where it gradually drops.     *
 *                                                                                *
 **********************************************************************************/
/**
 * 
 * @param v 
 * @param c 
 * @param l 
 * @param rlsrcp 
 * @param opacity 
 * @param IntersectContext 
 * @param iteration 
 * @param finfo 
 * @return 
 */
Uint32 Sphere::shade(Vector& v, const Vector& c, const Vector& l, double rlsrcp,
		float &opacity, void *IntersectContext, int iteration, FilteringInfo& finfo)
{
#define limit1(a) (a>1?1:(a))
#define lift(x) (((x)*(x))*((x)*(x))*((x)*(x))*((x)*(x))*((x)*(x))*((x)*(x))*((x)*(x))*((x)*(x)))
	Vector i, xv, f, e, li, rt, ci;
	double doi=1.0/this->d, dd, lambda;
	double cosbeta, cosbetalim;
	int rezult;
	int i2x, i2y;
	Uint32 ttt;

	double IP, IOp, cosalpha, cosalphap, OOp;
	Vector Ip, Op, q, o;

#ifdef MAKE_PROFILING
	if (!iteration) prof_enter(PROF_SOLVE3D);
#endif
	sphere_intersection_context *spc = (sphere_intersection_context*) IntersectContext;

	spc->determinant = fsqrt(spc->determinant); // <- this assumes we've called FastIntersect before Solve3D
	lambda = (-(spc->gB) - (spc->determinant)) * rlsrcp;
	if (!iteration) {	// check if we're close to the edge:
		if (spc->determinant * doi > 0.25f) opacity = 1.0f;
			else    {
				opacity = sqr(spc->determinant*doi*4.0f);
			}
	}
	
// d is the sphere's center
	Vector d = pos;

 // calculate the point I where the ray from the user crosses the sphere's surface
	i.macc(c, v, lambda);

 // xv is a vector, originating from D, pointing towards I
	xv.make_vector(i, d);

	xv.scale(doi); //normalise the vector xv

 // li is a vector, originating from I, pointing towards L
	li.make_vector(i, l);
	
 // ci is a vector, originating from I, pointing towards C
	ci.make_vector(c, i);

 // find the dot product of li and xv
	dd = li*xv;

	Vector ie;// = li - xv * (2.0 * dd);
	ie.macc(li, xv, -2.0 * dd);
	ci.norm();
	ie.norm();

 // to calculate phong shading we need the cosine of the angle CIE

	float shadow_mul = 1.0;
	
	if (r_shadows && (flags & (SHADOWED | PART_SHADOWED))) {
		shadow_mul = 0.0;
		if ((flags & PART_SHADOWED) && occluders != NULL) {
			const float rnss[3] = {1.0, 1 / 7.0f, 1 / 13.0f };
#if SHADOW_TYPE == ST_FIXED
			shadow_mul += _shadow_test(i, l, 0);
			if (g_shadowquality > 0) {
				double R = light_radius;
				shadow_mul += _shadow_test(i, l + Vector( +R, 0.0, 0.0), 1);
				shadow_mul += _shadow_test(i, l + Vector( -R, 0.0, 0.0), 2);
				shadow_mul += _shadow_test(i, l + Vector(0.0,  +R, 0.0), 3);
				shadow_mul += _shadow_test(i, l + Vector(0.0,  -R, 0.0), 4);
				shadow_mul += _shadow_test(i, l + Vector(0.0, 0.0,  +R), 5);
				shadow_mul += _shadow_test(i, l + Vector(0.0, 0.0,  -R), 6);
				if (g_shadowquality > 1) {
					double R1 = R * 0.707106781186;
					shadow_mul += _shadow_test(i, l + Vector(+R1, +R1, 0.0), 7);
					shadow_mul += _shadow_test(i, l + Vector(-R1, -R1, 0.0), 8);
					shadow_mul += _shadow_test(i, l + Vector(+R1, 0.0, +R1), 9);
					shadow_mul += _shadow_test(i, l + Vector(-R1, 0.0, -R1), 10);
					shadow_mul += _shadow_test(i, l + Vector(0.0, +R1, +R1), 11);
					shadow_mul += _shadow_test(i, l + Vector(0.0, -R1, -R1), 12);
				}
			}
			
			shadow_mul *= rnss[g_shadowquality]; 

#else 
			for (int q = 0; q < SHADOW_SAMPLES; q++) {
				shadow_mul += _shadow_test(i, l + Vector(
						R * (-1.0 + 2.0*drandom()),
						R * (-1.0 + 2.0*drandom()),
						R * (-1.0 + 2.0*drandom())),
					0);
			}
			shadow_mul /= SHADOW_SAMPLES;
#endif
		}
	}
	

	rezult = iambient;
	if (shadow_mul) {
		cosbetalim = -dd / li.length();
		cosbeta    = ci * ie;
		if (cosbetalim < 0.0) cosbetalim = 0.0;
		if (cosbeta    < 0.0) cosbeta    = 0.0;
		rezult += (int) ((lift(cosbeta)*specular + cosbetalim * diffuse)*shadow_mul*65536.0f);
	}

	rezult = (rezult>65536)?65536:rezult;
	if (flags & MAPPED) {
 		int ax = tex->get_x(), ay = tex->get_y();
 		i2x = (int) (0.5*normangle(atan2(-i[0]+d[0], i[2]-d[2]))*ax);
		i2y = (int) (0.5*((d[1]+this->d-i[1])/this->d)*ay);
		int *adata = (int*) tex->get_data();
		rezult = multiplycolor(adata[(i2x + i2y * ax)%(ax*ay)], rezult);
 	}
	else rezult=(((this->b * rezult)>>16) + (((this->g * rezult)>>16)<<8) + ((this->r * rezult) & 0xff0000));
#ifdef MAKE_PROFILING
	if (!iteration) prof_leave(PROF_SOLVE3D);
#endif
	if (! (flags & RAYTRACED)) return rezult;
 	else {
#ifdef MAKE_PROFILING
		if (!iteration) prof_enter(PROF_RAYTRACE);
#endif
	 // li is a vector, originating at I, pointing towards C
		li.make_vector(c, i);

	// find the dot product of li and xv in dd
		dd = li*xv;

	// point f is the projection of C onto the vector xv
		f.macc(i,xv,dd);

		e = f.scale(2.0)-c;

		rt.make_vector(e, i);

		finfo.lectionType = REFLECTION;
		ttt = blend(Raytrace(i,rt, (flags & RECURSIVE) | RAYTRACE_BILINEAR_MASK, iteration+1, this, finfo), rezult, refl);
		if (flags & SEETHROUGH) { // is the sphere transparent?
		/*
			Explanation of the refraction algorithm:
			(see http://www.ps.missouri.edu/rickspage/refract/refraction.html)

				The user's ray comes from C and hits the sphere at I. The ray is then refracted.
			We have xv, the normalized vector from D to I (D is the sphere's centrum).
			The (lesser) angle between Xv and CI is called an incident angle. This is the angle Alpha.
			Let the vector CI continues (refracted) through the sphere until it hits the surface again in I'.
			According to the Snell's Law the incident angle Alpha' between II' and Xv can be calculated
			as follows:

			Sin(Alpha') = Ni/Nr*Sin(Alpha),
				where Ni and Nr are the Refraction Indices of the  materials, which the
			ray is leaving/entering correspondingly (that is, Ni is the RI(refractive Index) of the air, and
			Nr is the RI of the glass - see REFRACT_FACTOR define in the beginning of this source file)

			(Of course, we don't need the actual angle Alpha' (nor Alpha, btw), so we won't bother doing an arcsin
			to obtain the exact angle)

			Quick brief: We get Cos(Alpha) using dot prouct. Get Sin(Alpha) using Pithagor. Get Sin(Alpha') using
			Snell's Law. Get Cos(Alpha') using pithagor.

			Now we notice that, I, D and I' are on the same plane (let us denote that plane with Zeta). Zeta also
			contains the vector q, which exits the sphere after the second refraction. Let we denote the crossing point
			of the rays CI and q with O'. OO' cross II' = P. O'' belongs to OO' and O'P = PO''. Now, let the vector o
			denotes the vector O'O''. It is easy to see, that IOI'O' is a romboid in Zeta, and q = O'I' = IO' + o;
			moreover, II' = 2*IO' + o. As we have the exit point and the exit vector, we simply call raytrace to continue
			seeking what the ray "sees":)

			if you have the full source of the project, you may well consider looking in the refract_scheme.gif
		*/
		// cosine of alpha is the dot product of v and xv divided by the length of xv (1) * v(1)
			v.norm(); //v is still not normalized... :)
			cosalpha = v*xv;
			cosalpha = fabs(cosalpha);
			cosalphap = fsqrt(REFRACT_FACTOR_SQR_M1 + REFRACT_FACTOR_SQR*sqr(cosalpha));
			IP = this->d*cosalphap;

			IOp = IP / (cosalphap*cosalpha + REFRACT_FACTOR * (1.0f - sqr(cosalpha)));
			Vector IOpv(
				v[0] * IOp,
				v[1] * IOp,
				v[2] * IOp);
			Op.add2(i, IOpv);
		// o is the vector, originating from Op, pointing towards d
			o.make_vector(d, Op);
			OOp = sqr(IOp) - sqr(IP);
			OOp = OOp < 0.0f ? 0.0 : OOp; //lower clamp
			OOp = 2.0 * fsqrt(OOp);
			o.make_length(OOp);
			q.add2(IOpv, o);
			IOpv.scale(2.0);
			Ip.add3(IOpv, o, i);
			finfo.lectionType = REFRACTION;
		// Ip is the exit point, q is the exit vector...
			ttt = blend(
				Raytrace(Ip,q , (flags & RECURSIVE) | RAYTRACE_BILINEAR_MASK, iteration+1, this, finfo),
				ttt,
				this->opacity);
		}
#ifdef MAKE_PROFILING
		if (!iteration) prof_leave(PROF_RAYTRACE);
#endif
		return ttt;
	}
}

float Sphere::_shadow_test(const Vector & I, const Vector & light, int opt)
{
	Vector dir;
	dir.make_vector(light, I);
	dir.norm();
	for (int i = 0; i < occluders_size; i++) {
		if (occluders[occluders_ind + i]->sintersect(I, dir, opt)) return 0.0f;
	}
	return 1.0f;
}

int Sphere::get_best_miplevel(double x0, double z0, FilteringInfo & fi)
{
	char context0[128], context1[128];
	int& last_val = fi.last_val;

	Vector p0, p1, v0, v1, fin0, fin1;
	p0.add2(fi.through, fi.xinc);
	p1.add2(fi.through, fi.yinc);
	v0.make_vector(p0, fi.camera);
	v1.make_vector(p1, fi.camera);
	double A0, A1;
	A0 = v0.lengthSqr();
	A1 = v1.lengthSqr();
	if (!fastintersect(v0, fi.camera, A0, context0) ||
	    !fastintersect(v1, fi.camera, A1, context1)) return last_val;
	//
	double lambda0, lambda1;
	lambda0 = intersection_dist(context0) / A0;
	lambda1 = intersection_dist(context1) / A1;
	Vector i0, i1;
	i0.macc(fi.camera, v0, lambda0);
	i1.macc(fi.camera, v1, lambda1);
	Vector xv0, xv1;
	xv0.make_vector(i0, this->pos);
	xv1.make_vector(i1, this->pos);
	xv0.norm();
	xv1.norm();
	if (fi.lectionType == REFLECTION) {
		Vector ic0, ic1;
		ic0.make_vector(fi.camera, i0);
		ic1.make_vector(fi.camera, i1);
		double dd0, dd1;
		dd0 = xv0 * ic0;
		dd1 = xv1 * ic1;
		Vector f0, f1;
		f0.macc(i0, xv0, dd0);
		f1.macc(i1, xv1, dd1);
		Vector e0, e1;
		e0 = f0.scale(2.0) - fi.camera;
		e1 = f1.scale(2.0) - fi.camera;
		
		fin0.make_vector(e0, i0);
		fin1.make_vector(e1, i1);
	} else {
		v0.norm();
		v1.norm();
		double cosalpha0 = fabs(v0 * xv0);
		double cosalpha1 = fabs(v1 * xv1);
		double cosalphap0= fsqrt(REFRACT_FACTOR_SQR_M1 + REFRACT_FACTOR_SQR*sqr(cosalpha0));
		double cosalphap1= fsqrt(REFRACT_FACTOR_SQR_M1 + REFRACT_FACTOR_SQR*sqr(cosalpha1));
		double IP0, IP1, IOp0, IOp1;
		IP0 = this->d * cosalphap0;
		IP1 = this->d * cosalphap1;
		IOp0 = IP0 / (cosalphap0*cosalpha0 + REFRACT_FACTOR * (1.0f - sqr(cosalpha0)));
		IOp1 = IP1 / (cosalphap1*cosalpha1 + REFRACT_FACTOR * (1.0f - sqr(cosalpha1)));
		Vector IOpv0(
			v0[0] * IOp0,
			v0[1] * IOp0,
			v0[2] * IOp0);
		Vector IOpv1(
			v1[0] * IOp1,
			v1[1] * IOp1,
			v1[2] * IOp1);
		Vector Op0, Op1;
		Op0.add2(i0, IOpv0);
		Op1.add2(i1, IOpv1);
		Vector o0, o1;
		o0.make_vector(this->pos, Op0);
		o1.make_vector(this->pos, Op1);
		double OOp0, OOp1;
		OOp0 = sqr(IOp0) - sqr(IP0);
		OOp1 = sqr(IOp1) - sqr(IP1);
		OOp0 = OOp0 < 0.0f ? 0.0 : OOp0;
		OOp1 = OOp1 < 0.0f ? 0.0 : OOp1;
		OOp0 = 2.0 * fsqrt(OOp0);
		OOp1 = 2.0 * fsqrt(OOp1);
		o0.make_length(OOp0);
		o1.make_length(OOp1);
		Vector Ip0, Ip1;
		fin0.add2(IOpv0, o0);
		fin1.add2(IOpv1, o1);
		IOpv0.scale(2.0);
		IOpv1.scale(2.0);

		Ip0.add3(IOpv0, o0, i0);
		Ip1.add3(IOpv1, o1, i1);
		i0 = Ip0; i1 = Ip1;
	}
	double ydist0 = fabs(fin0.v[1]), ydist1 = fabs(fin1.v[1]);
	if ( ydist0 < DST_THRESHOLD || ydist1 < DST_THRESHOLD )
		return (last_val = LOG2_MAX_TEXTURE_SIZE);
	double dp0, dp1, scalefactor0, scalefactor1;
	dp0		 = fin0.v[1] < 0.0f ? -daFloor : + daCeiling;
	dp1		 = fin1.v[1] < 0.0f ? -daFloor : + daCeiling;
	dp0		-= copysign(i0.v[1], fin0.v[1]);
	dp1		-= copysign(i1.v[1], fin1.v[1]);
	scalefactor0	 = dp0 / ydist0;
	scalefactor1	 = dp1 / ydist1;
	double x1, z1, x2, z2;
	x1 = i0[0] + fin0[0] * scalefactor0;
	z1 = i0[2] + fin0[2] * scalefactor0;
	x2 = i1[0] + fin1[0] * scalefactor1;
	z2 = i1[2] + fin1[2] * scalefactor1;
	float fin_area = fabs(x0 * z1 + x1 * z2 + x2 * z0 - x2 * z1 - x1 * z0 - x0 * z2);
	last_val = (int) (0.5*log2(fin_area*3.14f));
	if (last_val > LOG2_MAX_TEXTURE_SIZE)
		last_val = LOG2_MAX_TEXTURE_SIZE;
	if (last_val < 0)
		last_val = 0;
	return last_val;
}

bool Sphere::sintersect(const Vector & start, const Vector & dir, int opt)
{
	char ctx[128];
	return fastintersect(dir, start, 1.0, ctx);
}

void sphere_close(void)
{
}
