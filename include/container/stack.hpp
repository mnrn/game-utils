/**
 * @brief スタック
 */

#ifndef STACK_HPP
#define STACK_HPP

#include "container.hpp"
#include <algorithm>
#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>

namespace container {

/**
 * @brief  スタック
 * @tparam class   T スタックの要素の型
 */
template <class T,
          class Allocator = boost::container::pmr::polymorphic_allocator<T>>
struct stack {
public:
  static_assert(std::is_nothrow_constructible_v<T>);
  using alloc = std::allocator_traits<Allocator>;

  explicit stack(std::size_t n = 32) { allocate_stack(n); }
  ~stack() noexcept { free_stack(); }

  /**< @brief スタックが空かどうかを返す */
  constexpr bool empty() const noexcept { return top_ == 0; }

  /**< @brief スタックが満杯かどうか返す */
  constexpr bool full() const noexcept { return top_ >= cap_; }

  /**< @brief スタックにxを挿入する  */
  template <class... Args> void push(Args &&... args) {
    BOOST_ASSERT_MSG(!full(), "Stack overflow."); // オーバーフローチェック
    alloc::construct(alloc_, &S_[top_++],
                     std::forward<Args>(args)...); // コンストラクタ呼び出し
  }

  /**< @brief スタックから一番上の要素を削除する */
  std::optional<T> pop() noexcept {
    if (empty()) { // アンダーフローチェック
      return std::nullopt;
    }
    decltype(auto) top = std::move(S_[top_ - 1]);
    alloc::destroy(alloc_, &S_[--top_]); // デストラクタ呼び出し
    return std::make_optional(top);
  }

  /**< @brief スタックのサイズを返す */
  constexpr std::size_t size() const noexcept { return top_; }

private:
  T *S_ = nullptr;      /**< スタックS */
  std::size_t top_ = 0; /**< スタックトップ */
  std::size_t cap_ = 0; /**< スタックSのバッファサイズ */
  Allocator alloc_;     /**< アロケータ */

private:
  /**< @brief スタックを破棄する */
  template <class U, typename std::enable_if<!std::is_trivially_destructible<
                         U>::value>::type * = nullptr>
  void destroy_stack() noexcept {
    std::for_each(S_, S_ + top_, [this](T &t) { alloc::destroy(alloc_, &t); });
  }
  template <class U, typename std::enable_if<std::is_trivially_destructible<
                         U>::value>::type * = nullptr>
  void destroy_stack() noexcept {}

  /**< @brief スタックを解放する */
  void free_stack() noexcept {
    destroy_stack<T>();
    alloc::deallocate(alloc_, S_, cap_);
    top_ = cap_ = 0;
    S_ = nullptr;
  }
  /**< @brief スタックを確保する */
  void allocate_stack(std::size_t n) {
    S_ = alloc::allocate(alloc_, n);
    cap_ = n;
  }
};

} // namespace container

#endif // end of STACK_HPP
