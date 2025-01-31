/**
 * @brief AVL木
 * @note  AVL木(AVL tree)は高さ平衡(height
 * balanced)2分探索木である.すなわち、各節点xの左右の部分木の高さは高々1しか違わない
 *        AVL木を実現するために各節点に特別な属性を管理する. x.hはxの高さである
 *        他の任意の2分探索木Tと同様、T.rootはその根を指すものと仮定する
 *
 * @note
 * Fhをh番目のフィボナッチ数とするとき、高さhのAVL木には少なくともFh個の節点があることから
 *        n個のAVL木の高さはΟ(lgn)であることがわかる
 *
 * @note  今回は属性pを省略してコードの簡略化を図ることにする
 */

#ifndef AVL_TREE_HPP
#define AVL_TREE_HPP

#include "container.hpp"
#include <algorithm>
#include <boost/assert.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <memory>
#include <optional>
#include <utility>

namespace container {

template <class Key, class T> struct avl_tree_node {
  using height_t = std::int32_t;
  union {
    struct {
      avl_tree_node *l; /**< 左の子    */
      avl_tree_node *r; /**< 右の子    */
    };
    avl_tree_node *c[2]; /**< 左右の子(0:左, 1:右) */
  };
  height_t h; /**< 高さ */
  Key key;    /**< キー    */
  T v;        /**< 付属データ */

  constexpr explicit avl_tree_node(const Key &k, const T &v) noexcept
      : l(nullptr), r(nullptr), h(1), key(k), v(v) {}
};

/**
 * @brief  AVL木
 * @tparam Key     キーの型
 * @tparam T       付属データの型
 * @tparam Compare キーを引数にとる比較述語の型
 */
template <class Key, class T, class Compare = std::less<Key>,
          class Allocator = boost::container::pmr::polymorphic_allocator<
              avl_tree_node<Key, T>>>
struct avl_tree {
  static_assert(std::is_nothrow_constructible_v<Key> &&
                std::is_nothrow_constructible_v<T>);
  using alloc = std::allocator_traits<Allocator>;
  using sides_t = std::int32_t;
  using pair_t = std::pair<const Key, T>;
  using node = avl_tree_node<Key, T>;
  using height_t = typename node::height_t;

  explicit avl_tree(std::size_t n = 32) { allocate_pool(n); }
  ~avl_tree() noexcept { free_pool(); } // 確保した記憶領域の解放

  /**
   * @brief  AVL木からキーkに対応する付属データを返す
   * @note   実行時間はΟ(lgn)
   * @param  const Key& k キーk
   * @return キーkに対応する付属データ
   */
  std::optional<T> find(const Key &k) const {
    const node *x = find(root_, k);
    return x == nullptr ? std::nullopt : std::make_optional(x->v);
  }

  /**
   * @brief AVL木Tにキーkの挿入を行う
   * @note  実行時間はΟ(lgn)
   * @param const Key& k キーk
   * @param const T& v   付属データv
   * @return キーkに対応していた付属データ
   */
  std::optional<T> insert(const Key &k, const T &v) {
    std::optional<T> opt = std::nullopt;
    root_ = insert(root_, k, v, opt);
    return opt;
  }

  /**
   * @brief AVL木Tからキーkを持つ節点の削除を行う
   * @note  実行時間はΟ(lgn)
   * @param const Key&git  k キーk
   * @return キーkに対応していた付属データ
   */
  std::optional<T> erase(const Key &k) {
    std::optional<T> opt = std::nullopt;
    root_ = erase(root_, k, opt);
    return opt;
  }

  /**
   * @brief  中間順木巡回を行う
   * @note   n個の節点を持つ2分探索木の巡回はΘ(n)かかる
   * @tparam class F const Key&を引数に取る関数オブジェクトの型
   * @param F fn     const T&を引数に取る関数オブジェクト
   */
  template <class F> void inorder(F fn) { inorder(root_, fn); }

private:
  /**
   * @brief  中間順木巡回を行う
   * @note   n個の節点を持つ2分探索木の巡回はΘ(n)かかる
   * @tparam class F const Key&を引数に取る関数の型
   * @param F fn     const Key&を引数に取る関数
   * @param node*x   巡回を行う部分木の根
   */
  template <class F> void inorder(node *x, F fn) {
    if (x == nullptr) {
      return;
    } // xがNILを指すとき、再帰は底をつく
    inorder(x->l, fn);
    fn(x->key, x->v);
    inorder(x->r,
            fn); // xの左右の子を根とする部分木に対して中間順木巡回を行う
  }

  /**
   * @brief  AVL木からキーkに対応する付属データを返す
   * @note   実行時間はΟ(lgn)
   * @param  const Key& k キーk
   * @return キーkに対応する付属データへのポインタ
   */
  node *find(node *x, const Key &k) const {
    if (x == nullptr || eq(x->key, k)) {
      return x;
    } else {
      return find(cmp_(k, x->key) ? x->l : x->r, k);
    }
  }

  /**
   * @brief 節点xを根とする部分木にキーkの挿入を行う
   * @param node*x       節点x
   * @param node*z       キーkを持つ節点z
   */
  node *insert(node *x, const Key &k, const T &v, std::optional<T> &opt) {
    if (x == nullptr) {
      return create_node(k, v);
    }                      // xがNILを指すとき、再帰は底をつく
    if (cmp_(k, x->key)) { // xの適切な子に再帰し、
      x->l = insert(x->l, k, v, opt);
    } else if (cmp_(x->key, k)) {
      x->r = insert(x->r, k, v, opt);
    } else {
      opt = x->v;
      x->v = v;
      return x;
    }
    return balance(x); // xを根とする部分木を高さ平衡にする
  }

  /**
   * @brief 節点xを根とする部分木からキーkを削除する
   * @param node*x       節点x
   * @param const Key& k キーk
   */
  node *erase(node *x, const Key &k, std::optional<T> &opt) {
    if (x == nullptr) {
      return nullptr;
    } // キーkはAVL木Tに存在しなかった
    if (cmp_(k, x->key)) {
      x->l = erase(x->l, k, opt);
      return balance(x);
    } // xの適切な子に再帰し、
    if (cmp_(x->key, k)) {
      x->r = erase(x->r, k, opt);
      return balance(x);
    } // xを根とする部分木を高さ平衡にする
    opt = x->v;
    node *y = x->l,
         *z = x->r; // x.key == kのとき、yをxの左の子、zをxの右の子とし、
    destroy_node(x); // xを解放する
    if (z == nullptr) {
      return y;
    } // zがNILを指しているならば、新たな部分木の根としてyを返す
    node *w = leftmost(
        z); // zがNILを指していないならば、wをzを根とする部分木の中で最も左の子とする
    w->r = erase__(
        z); // zに対して再帰する.そして、wの右の子に新たな部分木を受け取る
    w->l = y;          // yをwの左の子にする
    return balance(w); // wを根とする部分木を高さ平衡にして戻る
  }

  /**
   * @brief
   * 節点xの左右の部分木はそれぞれ高さ平衡しており、その左右の子の高さの差は2以下であるとする
   *         xを入力とし、xを根とする部分木を高さ平衡になるように変換する
   * @note   回転は高々2回であるため実行時間はΟ(1)
   * @param  node*x 節点x
   * @return 高さ平衡な部分木の根
   */
  static node *balance(node *x) {
    x->h = reheight(x); // xの高さを更新する
    if (bias(x) > 1) {  // 左に2つ分偏っている場合、left-l
                        // caseおよびleft-r caseが考えられる
      if (bias(x->l) < 0) {
        x->l = left_rotate(x->l);
      } // l-r caseならば、左回転を行うことで、left-l caseに帰着させる
      return right_rotate(x); // 右回転を行うことでleft-l
                              // caseを解消し、高さ平衡を満たす部分木の根を返す
    }
    if (bias(x) < -1) { // 右に2つ分偏っている場合、right-r
                        // caseおよびright-l caseが考えられる
      if (bias(x->r) > 0) {
        x->r = right_rotate(x->r);
      } // r-l caseならば、右回転を行うことで、right-r
        // caseに帰着させる
      return left_rotate(x); // 左回転を行うことでright-r
                             // caseを解消し、高さ平衡を満たす部分木の根を返す
    }
    return x; // 高さ平衡の場合、xを返す
  }
  /**
   * @brief
   * 節点xからy(xのjの子)へのリンクを"ピボット"とするi回転で、回転の結果、
   *         yが部分木の新しい根となり、xがyのiの子、yのiの子がxのjの子になる
   * @note   x.c[j] != NILを仮定している. 実行時間はΟ(1)
   * @param  node*   x  節点x
   * @param  sides_t i  添字i(0のとき左、1のとき右)
   * @param  sides_t j  添字j(0のとき左、1のとき右)
   */
  static node *rotate(node *x, sides_t i, sides_t j) {
    node *y = x->c[j];  // yをxのjの子とする
    x->c[j] = y->c[i];  // yのi部分木をxのj部分木にする
    y->c[i] = x;        // xをyのiの子にする
    x->h = reheight(x); // xの高さを更新する
    y->h = reheight(y); // yの高さを更新する
    return y;           // 部分木の新しい根yを返す
  }
  static node *left_rotate(node *x) { return rotate(x, 0, 1); }
  static node *right_rotate(node *x) { return rotate(x, 1, 0); }

  /**
   * @brief 節点xを根とする部分木の中から最も左にある子を取得する
   * @param node*x 節点x
   */
  static node *leftmost(node *x) { return x->l ? leftmost(x->l) : x; }

  /**
   * @brief xを根とする部分木の最も左の子を削除する補助関数
   * @param node*x 節点x
   */
  static node *erase__(node *x) {
    if (x->l == nullptr) {
      return x->r;
    }
    x->l = erase__(x->l);
    return balance(x);
  }

private:
  /**< @brief 節点xの高さを取得する  */
  static constexpr height_t height(node *x) noexcept { return x ? x->h : 0; }
  /**< @brief 節点xの更新される高さを返す */
  static constexpr height_t reheight(node *x) noexcept {
    return std::max(height(x->l), height(x->r)) + 1;
  }
  /**< @brief 節点xの左右の子の高さの差(x.l - x.r)を返す */
  static constexpr height_t bias(node *x) noexcept {
    return height(x->l) - height(x->r);
  }

private:
  /**< @brief 節点xの記憶領域の確保を行う */
  node *create_node(const Key &k, const T &v) {
    BOOST_ASSERT_MSG(size_ < cap_, "AVL tree capacity over.");
    node *x = pool_ + size_;
    alloc::construct(alloc_, x, k, v);
    size_++;
    return x;
  }

  /**< @brief 節点xの記憶領域の解放を行う */
  void destroy_node(node *x) noexcept {
    alloc::destroy(alloc_, x);
    size_--;
  }

  /**< @brief 節点xを根とした部分木を再帰的に解放する*/
  void postorder_destroy_nodes(node *x) noexcept {
    if (x == nullptr) {
      return;
    }
    postorder_destroy_nodes(x->l);
    postorder_destroy_nodes(x->r);
    destroy_node(x);
  }

  /**< @brief メモリプールの解放 */
  void free_pool() noexcept {
    postorder_destroy_nodes(root_);
    alloc::deallocate(alloc_, pool_, cap_);
    root_ = pool_ = nullptr;
    size_ = cap_ = 0;
  }

  /**< @brief メモリプールの確保 */
  void allocate_pool(std::size_t n) {
    pool_ = alloc::allocate(alloc_, n);
    cap_ = n;
  }

private:
  /**< ＠brief キーlとキーrの非同値判定を行う */
  inline bool neq(const Key &l, const Key &r) const {
    return (cmp_(l, r) || cmp_(r, l));
  }
  /**< ＠brief キーlとキーrの同値判定を行う */
  inline bool eq(const Key &l, const Key &r) const { return !neq(l, r); }

private:
  node *root_ = nullptr; /**< AVL木の根 */
  std::size_t cap_ = 0;  /**< AVL木のバッファサイズ    */
  std::size_t size_ = 0; /**< AVL木のサイズ           */
  node *pool_ = nullptr; /**< AVL木の節点用メモリプール */
  Compare cmp_;          /**< 比較述語  */
  Allocator alloc_;      /**< アロケータ */
};

} // namespace container

#endif // end of AVL_TREE_HPP
