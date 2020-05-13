#pragma once
// Assert utility methods

#include <cstdio>
#include <cstdlib>

#define assertNotNull(condition, msg) do {\
		if ((condition) == nullptr) { \
			fprintf(stderr, "%s", msg); \
			exit(1); \
		} \
	} while (0)

#define assertThat(condition, msg) do {\
		if (!condition) { \
			fprintf(stderr, "%s", msg); \
			exit(1); \
		} \
	} while (0)

#define assertVkSuccess(call) do {\
		VkResult result = call; \
		if (result != VK_SUCCESS) { \
		} \
	} while (0)
