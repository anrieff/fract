void voxel_single_frame_do1(Uint32 *fb, int thread_index, Vector & tt, Vector & ti, Vector & tti)
{
	double fx, fz, fxi, fzi, dCX;
	float rcpwf[MAX_VOXEL_DIST];
	int x, z, xi, zi, index, index2;
	Uint32 lastcol;

	int k, i, wf, j, xr, yr, sz, lasty, newy, xr2, yr2, txadd, texandmask, tempx;
	int yrhl, newymp, opty;
	int sizemask, sizeshift;
	int render_dist;

	Uint32 *tex;
	Uint32 *c_column;
	float  *h;
	Uint32 color;


	xr = xsize_render(xres());
	yr = ysize_render(yres());
	xr2 = xr/2;
	yr2 = yr/2;

	yrhl = xr*yr;


	dCX = 1.0/tan(fov*0.75*FOURTY_FIVE_DEGREES);
	double tgB = tan(CVars::beta);
	float sinb = sin(-CVars::beta), cosb = cos(-CVars::beta);
	double Thing = sqrt(1+ sqr(tgB));
	double rcpThing = 1.0 / Thing;


	prof_enter(PROF_BUFFER_CLEAR);
	if (1 == cpu.count) {
		memset(fb, 0, xr*yr*4);
	} else {
		// crap; we need to zero the memory in parallel; COLUMNWISE!
		for (i = thread_index; i < xr; i+=cpu.count)
			for (j=0;j<yr;j++)
				fb[j*xr+i] = 0;
	}
	prof_leave(PROF_BUFFER_CLEAR);

	// do the actual work
	for (k=0;k<NUM_VOXELS;k++) {
		bool isfloor = vox[k].floor;
		int swOne = isfloor?-1:1;
		//render_dist = render_dist_min + (int) ((render_dist_max-render_dist_min)*component/(daing-daFloor));
		render_dist = 730;
		for (i=1;i<=render_dist;i++) rcpwf[i] = 1.0f / i;
		tex = vox[k].texture; h = vox[k].heightmap;
		sz = vox[k].size;
		sizemask = sz-1;
		sizeshift = power_of_2(sz);
		texandmask = sz*sz-1;
		fx = cur[0] + sin(CVars::alpha - fov * FOURTY_FIVE_DEGREES)*render_dist;
		fz = cur[2] + cos(CVars::alpha - fov * FOURTY_FIVE_DEGREES)*render_dist;
		fxi = ((cur[0] + sin(CVars::alpha + fov * FOURTY_FIVE_DEGREES)*render_dist) - fx) / xr;
		fzi = ((cur[2] + cos(CVars::alpha + fov * FOURTY_FIVE_DEGREES)*render_dist) - fz) / xr;
		fx += thread_index * fxi;
		fz += thread_index * fzi;

		for (i=thread_index;i<xr;i+=cpu.count) {
			c_column = fb + i;
			x = dbl2int16(cur[0]); z = dbl2int16(cur[2]);
			xi = dbl2int16((fx - cur[0]) / render_dist);
			zi = dbl2int16((fz - cur[2]) / render_dist);

			//wf = render_dist;
			opty = isfloor ? yr : 0;
			x+=xi;z+=zi;
			lastcol = 0xbabababa;
			for (lasty=999666111, wf=1; wf < render_dist; wf++, lasty = opty, x+=xi, z+=zi) {
				//if (((x>>16)&~sizemask)||((z>>16)&~sizemask)) continue;
				if (((unsigned)x>>16) > (unsigned) sz - 2 || ((unsigned)z>>16) > (unsigned) sz - 2) continue;
				if (!CVars::bilinear) {
					prof_enter(PROF_ADDRESS_GENERATE);
					index = ((((unsigned)z>>16)&sizemask)<<sizeshift) + (((unsigned)x>>16)&sizemask);

					double CFp = wf*Thing;
					double FpFpp = wf*tgB;
					double FFpp = h[index] - cur.v[1];

					double FQMUL = (double) (( (int) (FpFpp*FFpp>0&&fabs(FFpp)>fabs(FpFpp)))*2-1);
					double FpF;
					if (FpFpp*FFpp>0.0f)
						FpF = fabs(fabs(FpFpp)-fabs(FFpp));
						else
						FpF = fabs(FpFpp)+fabs(FFpp);

					double FQ = fabs(FpF)*rcpThing;
					double FpQ= fabs(FpFpp*FpF*rcpwf[wf]*rcpThing);
					double CQ = fabs(CFp)+FQMUL*fabs(FpQ);
					double XF3p=fabs(FQ*dCX/CQ)*((double)(((int)(FFpp<FpFpp))*2-1));

					newy   = yr2 + (int)(yr2*XF3p);
					prof_leave(PROF_ADDRESS_GENERATE);
					if ((isfloor && newy >= opty)||((!isfloor) && newy <= opty)) continue;
					prof_enter(PROF_INTERPOL_INIT);
					color = tex[index];
					if (isfloor) { // newy must be no less than 1
						newy = newy>=1 ? newy : 1;
					} else { //newy must be no more than yr-2
						newy = newy<= yr - 2 ? newy : yr-2;
					}
					newymp = newy * xr + swOne*xr;
					opty += swOne;
					prof_leave(PROF_INTERPOL_INIT);
					prof_enter(PROF_INTERPOLATE);
					if (lasty!=999666111) {
						j = opty * xr;
						if (isfloor) {
							do {
								c_column[j] = color;
								j -= xr;
							} while (j>newymp);
						} else {
							do {
								c_column[j] = color;
								j += xr;
							} while (j<newymp);
						}
					}
					opty = newy;
					prof_leave(PROF_INTERPOLATE);

				} else {
					prof_enter(PROF_ADDRESS_GENERATE);
					tempx = (((unsigned) x >> 16)&sizemask);
					txadd = ((tempx+1)&sizemask) - tempx;
					index = ((((unsigned)z>>16)&sizemask)<<sizeshift) + tempx;
					index2 = (index + sz) & texandmask;
					double FFpp = filter4(x&0xffff, z&0xffff,
						h[ index		],
						h[ index + txadd	],
						h[ index2		],
						h[ index2 + txadd	]) - cur.v[1];
					double CFp = wf*Thing;
					double FpFpp = wf*tgB;

					double FQMUL = (double) (( (int) (FpFpp*FFpp>0&&fabs(FFpp)>fabs(FpFpp)))*2-1);
					double FpF;
					if (FpFpp*FFpp>0.0f)
						FpF = fabs(fabs(FpFpp)-fabs(FFpp));
						else
						FpF = fabs(FpFpp)+fabs(FFpp);

					double FQ = fabs(FpF)*rcpThing;
					double FpQ= fabs(FpFpp*FpF*rcpwf[wf]*rcpThing);
					double CQ = fabs(CFp)+FQMUL*fabs(FpQ);
					double XF3p=fabs(FQ*dCX/CQ)*((double)(((int)(FFpp<FpFpp))*2-1));
					newy   = yr2 + (int)(yr2*XF3p);
					prof_leave(PROF_ADDRESS_GENERATE);
					if ((isfloor && newy >= opty)||((!isfloor) && newy <= opty)) continue;
					prof_enter(PROF_INTERPOL_INIT);
					if (cpu.sse) {
						color = bilinea4(tex[index], tex[index+txadd], tex[index2], tex[index2+txadd],
							x & 0xffff, z & 0xffff);
					} else {
						color = bilinea_p5(tex[index], tex[index+txadd], tex[index2], tex[index2+txadd],
							x & 0xffff, z & 0xffff);
					}
					if (lastcol == 0xbabababa) lastcol = color;
					if (isfloor) { // newy must be no less than 1
						newy = newy>=1 ? newy : 1;
					} else { //newy must be no more than yr-2
						newy = newy<= yr - 2 ? newy : yr-2;
					}
					newymp = newy * xr + swOne*xr;
					opty += swOne;
					int oldc[3], newc[3];
					decompose(lastcol, oldc); decompose(color, newc);
					int steps = swOne*(newy-lasty);
					int stepsrcp = (int) (65536.0f/steps);
					int incc[3] = {
						(((newc[0]-oldc[0])>>8)*stepsrcp)/256,
						(((newc[1]-oldc[1])>>8)*stepsrcp)/256,
						(((newc[2]-oldc[2])>>8)*stepsrcp)/256
						};
					oldc[0] += swOne*(opty-lasty)*incc[0];
					oldc[1] += swOne*(opty-lasty)*incc[1];
					oldc[2] += swOne*(opty-lasty)*incc[2];
					prof_leave(PROF_INTERPOL_INIT);
					prof_enter(PROF_INTERPOLATE);
					if (lasty!=999666111) {
						j = opty * xr;
						if (isfloor) {
							do {
								c_column[j] =
										((oldc[0]&0xff0000) >> 16) |
										((oldc[1]&0xff0000) >> 8 ) |
										(oldc[2]&0xff0000);
								j -= xr;
								oldc[0] += incc[0];
								oldc[1] += incc[1];
								oldc[2] += incc[2];
							} while (j>newymp);
						} else {
							do {
								c_column[j] =
										((oldc[0]&0xff0000) >> 16) |
										((oldc[1]&0xff0000) >> 8 ) |
										(oldc[2]&0xff0000);
								j += xr;
								oldc[0] += incc[0];
								oldc[1] += incc[1];
								oldc[2] += incc[2];
								
							} while (j<newymp);
						}
					}
					opty = newy;
					lastcol = color;
					prof_leave(PROF_INTERPOLATE);
				}
			}
			fx += cpu.count*fxi; fz += cpu.count*fzi;
		}
	}
	/*if (!thread_index) {
		show_light(fb, xr, yr);
	}*/
}
