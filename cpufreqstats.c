/*
 * Copyright 2007, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * Authors:
 * 	Arjan van de Ven <arjan@linux.intel.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>

#include "powertop.h"

#define MAX_PSTATES 32
#define MAX_CPUS 8
struct cpufreqdata {
	uint64_t	frequency;
	uint64_t	count;
};

struct cpufreqdata freqs[MAX_CPUS][MAX_PSTATES];
struct cpufreqdata oldfreqs[MAX_CPUS][MAX_PSTATES];

struct cpufreqdata delta[MAX_CPUS][MAX_PSTATES];

char cpufreqstrings[25][256];
int topfreq = -1;

static void zap(void)
{	
	memset(freqs, 0, sizeof(freqs));
}

int sort_by_count (const void *av, const void *bv)
{
        const struct cpufreqdata       *a = av, *b = bv;
        return b->count - a->count;
} 

int sort_by_freq (const void *av, const void *bv)
{
        const struct cpufreqdata       *a = av, *b = bv;
        return b->frequency - a->frequency;
} 

static char *HzToHuman(unsigned long hz)
{	
	static char buffer[1024];
	memset(buffer, 0, 1024);
	unsigned long long Hz;

	Hz = hz;

	/* default: just put the Number in */
	sprintf(buffer,_("%9lli"), Hz);

	if (Hz>1000)
		sprintf(buffer, _("%6lli Mhz"), (Hz+500)/1000);

	if (Hz>1500000)
		sprintf(buffer, _("%6.2f Ghz"), (Hz+5000.0)/1000000);


	return buffer;
}


void  do_cpufreq_stats(void)
{
	DIR *dir;
	struct dirent *dirent;
	FILE *file;
	char filename[PATH_MAX];
	char line[1024];

	int cpu = -1;
	int ret, cpucount;
	int maxfreq = 0;
	uint64_t total_time[MAX_CPUS] = {0};

	memcpy(&oldfreqs, &freqs, sizeof(freqs));
	memset(&cpufreqstrings, 0, sizeof(cpufreqstrings));
	sprintf(cpufreqstrings[0], _("P-states (frequencies)\n"));

	for (ret = 0; ret<MAX_PSTATES; ret++) {
		for (cpucount = 0; cpucount < MAX_CPUS; cpucount++)
			freqs[cpucount][ret].count = 0;
	}

	dir = opendir("/sys/devices/system/cpu");
	if (!dir)
		return;

	while ((dirent = readdir(dir))) {
		int i;
		if (dirent->d_name[0]=='.')
			continue;
		sprintf(filename, "/sys/devices/system/cpu/%s/cpufreq/stats/time_in_state", dirent->d_name);
		file = fopen(filename, "r");
		if (!file)
			continue;
		memset(line, 0, 1024);
		cpu++;
		if ( cpu >= MAX_CPUS)
			cpu = MAX_CPUS -1;
		i = 0;
		while (!feof(file)) {
			uint64_t f,count;
			char *c;
			if (fgets(line, 1023,file)==NULL)
				break;
			f = strtoull(line, &c, 10);
			if (!c)
				break;
			count = strtoull(c, NULL, 10);

			if (freqs[cpu][i].frequency && freqs[cpu][i].frequency != f) {
				zap();
				break;
			}

			freqs[cpu][i].frequency = f;
			freqs[cpu][i].count += count;

			if (f && maxfreq < i)
				maxfreq = i;
			i++;
			if (i>(MAX_PSTATES - 1))
				break;
		}
		fclose(file);
	}

	closedir(dir);

	for (ret = 0; ret < MAX_PSTATES; ret++) {
		for (cpucount = 0; cpucount < MAX_CPUS; cpucount++) {
			delta[cpucount][ret].count = freqs[cpucount][ret].count - oldfreqs[cpucount][ret].count;
			total_time[cpucount] += delta[cpucount][ret].count;
			delta[cpucount][ret].frequency = freqs[cpucount][ret].frequency;
			if (freqs[cpucount][ret].frequency != oldfreqs[cpucount][ret].frequency)
				return;  /* duff data */
		}
	}

	for (cpucount = 0 ; cpucount < MAX_CPUS; cpucount++) {
		if (total_time[cpucount])
			break;
	}
	if (cpucount == MAX_CPUS)
		return;

	topfreq = -1;

	for (ret = 0 ; ret<=maxfreq; ret++) {
	uint64_t last_freq = -1;
	strlcpy(cpufreqstrings[ret+1], " ",sizeof(cpufreqstrings[ret+1]));
		for ( cpucount = 0; cpucount <= cpu; cpucount++) {
			char temp_freq[32];
			if (delta[cpucount][ret].frequency != last_freq && delta[cpucount][ret].frequency != 0) {
				snprintf(temp_freq, sizeof(temp_freq), "%6s", HzToHuman(delta[cpucount][ret].frequency));
				strncat(cpufreqstrings[ret+1], temp_freq, 32);
				last_freq = delta[cpucount][ret].frequency;
			}
			snprintf(temp_freq, sizeof(temp_freq), "\t%5.1f%% ", delta[cpucount][ret].count * 100.0 / total_time[cpucount]);
			strncat(cpufreqstrings[ret+1], temp_freq, 32);
			if (delta[cpucount][ret].count > total_time[cpucount]/2)
				topfreq = ret;
		}
		strncat(cpufreqstrings[ret+1], "\n", 1);
	}

}
