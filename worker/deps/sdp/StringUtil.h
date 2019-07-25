#ifndef RTC_GATEWAY_STRINGUTIL_H_
#define RTC_GATEWAY_STRINGUTIL_H_

#include <string>
#include <vector>

namespace stringutil {

std::vector<std::string> splitOneOf(const std::string &str,
		const std::string &delims, const size_t maxSplits = 0);

}  // namespace stringutil

#endif  // RTC_GATEWAY_STRINGUTIL_H_
