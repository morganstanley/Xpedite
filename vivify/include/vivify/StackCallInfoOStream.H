/*!
 * \file
 * Stack Call Info insertion operators.
 *
 * \author Andrew C., Morgan Stanley
 */

#pragma once

#include <vector>
#include <ostream>

#include "StackCallInfo.H"


namespace vivify {

std::ostream& operator<<(std::ostream& out_, const StackCallInfo& call_);
std::ostream& operator<<(std::ostream& out_, const std::vector<StackCallInfo>& calls_);

} // namespace vivify
