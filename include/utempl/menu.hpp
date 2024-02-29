#pragma once
#include <utempl/optional.hpp>
#include <utempl/constexpr_string.hpp>
#include <utempl/tuple.hpp>
#include <utempl/utils.hpp>
#include <iostream>
#include <array>
#include <fmt/format.h>
#include <fmt/compile.h>


namespace utempl {

constexpr std::size_t CountDigits(std::size_t num) {
  std::size_t count = 0;
  do {
    ++count;
    num /= 10;
  } while (num != 0);
  return count;
};

constexpr std::size_t GetDigit(std::size_t num, std::size_t index) {
  for (std::size_t i = 0; i < index; ++i) {
    num /= 10;
  }
  return num % 10;
};

template <std::size_t num>
consteval auto ToString() {
  constexpr std::size_t digits = CountDigits(num);
  return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    return ConstexprString{std::array{static_cast<char>('0' + GetDigit(num, digits - 1 - Is))..., '\0'}};
  }(std::make_index_sequence<digits>());
};

template <typename Range>
constexpr auto GetMax(Range&& range) {
  std::remove_cvref_t<decltype(range[0])> response = 0;
  for(const auto& element : range) {
    response = element > response ? element : response;
  };
  return response;
};


namespace menu {

namespace impl {

template <std::size_t N1, std::size_t N2>
struct CallbackMessage {
  ConstexprString<N1> message;
  Optional<ConstexprString<N2>> need;
  consteval CallbackMessage(const char (&need)[N2], const char (&message)[N1]) : 
    message(std::move(message))
    ,need(std::move(need)) {};
  consteval CallbackMessage(const char (&message)[N1]) : 
    message(std::move(message))
    ,need(std::nullopt) {};
};
template <std::size_t N1, std::size_t N2>
CallbackMessage(const char(&)[N1], const char(&)[N2]) -> CallbackMessage<N2, N1>;

template <std::size_t N1>
CallbackMessage(const char(&)[N1]) -> CallbackMessage<N1, 0>;

} // namespace impl



template <Tuple storage = Tuple{}, typename... Fs>
struct Menu {
private:
  template <ConstexprString fmt, ConstexprString message, ConstexprString alignment, ConstexprString neededInput>
  static consteval auto FormatMessage() {
    // + 1 - NULL Terminator
    constexpr auto size = fmt::formatted_size(FMT_COMPILE(fmt.data.begin())
      ,neededInput
      ,message
      ,alignment) + 1;
    char data[size]{};
    fmt::format_to(data, FMT_COMPILE(fmt.data.begin())
      ,neededInput
      ,message
      ,alignment);
    return ConstexprString<size>(data);
  };
  template <ConstexprString fmt, std::size_t I>
  static consteval auto FormatMessageFor() {
    constexpr ConstexprString message = Get<I>(storage).message;
    constexpr ConstexprString neededInput = [&] {
      if constexpr(Get<I>(storage).need) {
        return *Get<I>(storage).need;
      } else {
        return ToString<I>();            
      };
    }();
    constexpr ConstexprString alignment = CreateStringWith<GetMaxSize() - (Get<I>(storage).need ? Get<I>(storage).need->size() : CountDigits(I))>(' ');
    return FormatMessage<fmt, message, alignment, neededInput>();
  };
public:
  Tuple<Fs...> functionStorage;

  static consteval auto GetMaxSize() -> std::size_t {
    return [&]<auto... Is>(std::index_sequence<Is...>){
      constexpr auto list = ListFromTuple(storage); 
      return GetMax(std::array{(std::remove_cvref_t<decltype(*std::declval<decltype(Get<Is>(list))>().need)>::kSize != 0 ? std::remove_cvref_t<decltype(*std::declval<decltype(Get<Is>(list))>().need)>::kSize : CountDigits(Is))...});
    }(std::index_sequence_for<Fs...>());
  };
  template <impl::CallbackMessage message, std::invocable F>
  constexpr auto With(F&& f) const {
    return Menu<storage + Tuple{message}, Fs..., std::remove_cvref_t<F>>{.functionStorage = this->functionStorage + Tuple(std::forward<F>(f))};
  };
  template <ConstexprString fmt, ConstexprString enter = "|> ">
  inline constexpr auto Run(std::istream& in = std::cin, std::FILE* out = stdout) const -> std::size_t {
    return [&]<auto... Is>(std::index_sequence<Is...>) -> std::size_t {
      constexpr auto message = ((FormatMessageFor<fmt, Is>() + ...) + enter);
      auto result = std::fwrite(message.begin(), 1, message.size(), out);
      if(result < message.size()) {
        return EOF;
      };
      if(std::fflush(out) != 0) {
        return EOF;
      };
      std::string input;
      std::getline(in, input);
      ([&]<auto I, impl::CallbackMessage message = Get<I>(storage)>(Wrapper<I>) {
        if constexpr(message.need) {
          if(*message.need == input) {
            Get<I>(this->functionStorage)();
          };
        } else {
          if(ToString<I>() == input) {
            Get<I>(this->functionStorage)();
          };
        }; 
      }(Wrapper<Is>{}), ...);
      return 0;
    }(std::index_sequence_for<Fs...>());
  };

};

} // namespace menu
} // namespace utempl


