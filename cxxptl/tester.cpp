/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mxgail.com (change to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "cxxptl.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

ThreadPool pool;

struct DoWork: public Parallel {
	double data[1600];
	void entry(int j, int k)
	{
		printf("DoWork::entry(%d, %d)\n", j, k); fflush(stdout);
		data[j*100] = 0;
		for (int i = j; i < 1000000000; i+=k) {
			if (!i) continue;
			double d = i;
			data[j*100] += 1.0 / (d*d);
		}
	}
};

int main(void)
{
	int n;
	printf("This PC has %d processors\n", get_processor_count());
	for (n = 1; n <= 8; n*=2) {
		printf("Using %d processors for internal calculations.\n", n);
		DoWork dowork;
		time_t xt = time(NULL);
		pool.run(&dowork, n);
		xt = time(NULL) - xt;
		
		double sum = 0;
		for (int i = 0; i < n; i++) {
			printf("Processor %d gave %.5lf as result\n", i, dowork.data[i*100]);
			sum += dowork.data[i*100];
		}
		printf("pi = %.9lf\n", sqrt(sum * 6.0));
		printf("Result produced in %u sec\n", xt);
	}
	return 0;
}
