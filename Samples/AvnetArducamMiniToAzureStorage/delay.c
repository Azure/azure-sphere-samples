#if defined(AzureSphere_CA7)
#include <errno.h>
#include <time.h>
#elif defined(AzureSphere_CM4)
#endif

#include "delay.h"

void delay_ms(uint32_t period)
{
#if defined(AzureSphere_CA7)
	struct timespec ts = {
		(time_t)(period / 1000),
		(long)((period % 1000) * 1000000)
	};

	while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));
#elif defined(AzureSphere_CM4)

#if defined(FreeRTOS)

#else
	// TODO: diff impl for RTOS and baremetal (base on GPT)
	uint32_t cnt = 10000 * period;
	while (cnt-- > 0);
#endif
#endif
}