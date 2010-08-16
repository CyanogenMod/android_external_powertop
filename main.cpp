#include <iostream>
#include <stdlib.h>

#include "cpu/cpu.h"
#include "process/process.h"
#include "perf/perf.h"
#include "perf/perf_bundle.h"


int main(int argc, char **argv)
{
	int i;

	system("/sbin/modprobe cpufreq_stats > /dev/null 2>&1");

	enumerate_cpus();


	for (i = 0; i < 4; i++) {
		start_process_measurement();
		start_cpu_measurement();


		cout << "measuring\n";
		sleep(3);


		end_cpu_measurement();
		end_process_measurement();

		cout << "doing math \n";

		process_cpu_data();
		process_process_data();
	}


	end_process_data();
//	display_cpu_cstates();
//	cout << "\n\n\n";

//	display_cpu_pstates();

//	display_cpu_cstates("<table>\n", "</table>\n", "<tr><td>","</td><td>", "</td></tr>\n");
//	display_cpu_pstates("<table>\n", "</table>\n", "<tr><td>","</td><td>", "</td></tr>\n");


	
	return 0;
}