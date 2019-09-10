/*!
 * \file
 * Stack Call Info insertion operators.
 *
 * \author Andrew C., Morgan Stanley
 */

#include <vivify/StackCallInfoOStream.H>

#include <iterator>


namespace vivify {

std::ostream& operator<<(std::ostream& out_, const StackCallInfo& call_)
{
  std::ios l_fmt(nullptr);
  l_fmt.copyfmt(out_);

  const auto& l_callStr = [](const std::string& str_) noexcept -> const std::string& {
    static const std::string l_empty{"??"};
    return (str_.empty() ? l_empty : str_);
  };

  const auto& l_info{call_._info};
  out_ << l_callStr(l_info._func) << '\n'
       << "    at  " << l_callStr(l_info._file) << ':' << l_info._line << '\n';

  const auto& l_inlInfo{call_._inlInfo};
  if (l_inlInfo._valid)
  {
    out_ << "    inlined by  " << l_callStr(l_inlInfo._func) << '\n'
         << "    inlined at  " << l_callStr(l_inlInfo._file) << ':' << l_inlInfo._line << '\n';
  }

  out_ << "    in  " << call_._bfile << " [0x" << std::hex << call_._ip << ']';

  out_.copyfmt(l_fmt);

  return out_;
}

std::ostream& operator<<(std::ostream& out_, const std::vector<StackCallInfo>& calls)
{
  if (!calls.empty())
  {
    std::copy(calls.cbegin(), std::prev(calls.cend()),
              std::ostream_iterator<StackCallInfo>{out_, "\n"});
    out_ << (*calls.crbegin());
  }
  return out_;
}

} // namespace vivify
