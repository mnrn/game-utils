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

//****************************************
// インクルードガード
//****************************************

#ifndef AVL_TREE_HPP
#define AVL_TREE_HPP

//****************************************
// 必要なヘッダファイルのインクルード
//****************************************

#include "container.hpp"
#include <algorithm>
#include <optional>
#include <utility>

//****************************************
// 構造体の定義
//****************************************

/**
 * @brief  AVL木
 * @tparam Key     キーの型
 * @tparam T       付属データの型
 * @tparam Compare キーを引数にとる比較述語の型
 */
template <class Key, class T, class Compare = std::less<Key>> struct avl_tree {
  static_assert(std::is_nothrow_constructible_v<Key> &&
                std::is_nothrow_constructible_v<T>);
  using height_t = std::int32_t;
  using sides_t = std::int32_t;
  using pair_t = std::pair<const Key, T>;

  struct node {
    union {
      struct {
        node *left;  /**< 左の子    */
        node *right; /**< 右の子    */
      };
      node *c[2]; /**< 左右の子(0:左, 1:右) */
    };
    height_t h; /**< 高さ */
    union {
      const Key key;             /**< キー    */
      const char d[sizeof(Key)]; /**< ダミー  */
    };
    T v;        /**< 付属データ */
    node *next; /**< 単方向未使用リストL */

    constexpr node() noexcept
        : left(nullptr), right(nullptr), h(1), d("\0"), next(nullptr) {}
    constexpr explicit node(const Key &k) noexcept
        : left(nullptr), right(nullptr), h(1), key(k), next(nullptr) {}
    constexpr node(const Key &k, const T &v) noexcept
        : left(nullptr), right(nullptr), h(1), key(k), v(v), next(nullptr) {}
  };

  node *root_;  /**< AVL木の根 */
  Compare cmp_; /**< 比較述語  */

  std::size_t cap_;  /**< AVL木のバッファサイズ    */
  std::size_t size_; /**< AVL木のサイズ           */
  node *pool_;       /**< AVL木の節点用メモリプール */
  node *free_;       /**< 空き節点へのポインタ     */

  explicit avl_tree(std::size_t n = 32)
      : root_(nullptr), cap_(n), size_(0), pool_(nullptr), free_(nullptr) {
    allocate_pool(n);
  }
  ~avl_tree() noexcept { free_pool(); } // 確保した記憶領域の解放

  /**
   * @brief  中間順木巡回を行う
   * @note   n個の節点を持つ2分探索木の巡回はΘ(n)かかる
   * @tparam class F const Key&を引数に取る関数オブジェクトの型
   * @param F fn     const Key&を引数に取る関数オブジェクト
   */
  template <class F> void inorder(F fn) { inorder(root_, fn); }

  /**
   * @brief AVL木Tにキーkの挿入を行う
   * @note  実行時間はΟ(lgn)
   * @param const Key& k キーk
   * @param const T& v   付属データv
   */
  void insert(const Key &k, const T &v) {
    node *z = create_node(k, v);
    assert(z != nullptr);
    root_ = insert(root_, z);
    size_++;
  }

  /**
   * @brief AVL木Tからキーkを持つ節点の削除を行う
   * @note  実行時間はΟ(lgn)
   * @param const Key& k キーk
   */
  void erase(const Key &k) {
    root_ = erase(root_, k);
    size_--;
  }

  /**
   * @brief  AVL木からキーkに対応する付属データを返す
   * @note   実行時間はΟ(lgn)
   * @param  const Key& k キーk
   * @return キーkに対応する付属データ
   */
  std::optional<T> find(const Key &k) { return find(root_, k); }

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
    inorder(x->left, fn);
    fn(x->key);
    inorder(x->right,
            fn); // xの左右の子を根とする部分木に対して中間順木巡回を行う
  }

  /**
   * @brief  AVL木からキーkに対応する付属データを返す
   * @note   実行時間はΟ(lgn)
   * @param  const Key& k キーk
   * @return キーkに対応する付属データへのポインタ
   */
  std::optional<T> find(node *x, const Key &k) {
    while (x != nullptr && neq(x->key, k)) {
      if (cmp_(x->key, k)) {
        x = x->left;
      } else {
        x = x->right;
      }
    }
    return x == nullptr ? std::nullopt : std::make_optional(x->v);
  }

  /**
   * @brief 節点xを根とする部分木にキーkの挿入を行う
   * @param node*x       節点x
   * @param node*z       キーkを持つ節点z
   */
  node *insert(node *x, node *z) {
    if (x == nullptr) {
      return z;
    } // xがNILを指すとき、再帰は底をつく
    cmp_(z->key, x->key)
        ? x->left = insert(x->left, z)
        : x->right = insert(x->right, z); // xの適切な子に再帰し、
    return balance(x); // xを根とする部分木を高さ平衡にする
  }

  /**
   * @brief 節点xを根とする部分木からキーkを削除する
   * @param node*x       節点x
   * @param const Key& k キーk
   */
  node *erase(node *x, const Key &k) {
    if (x == nullptr) {
      return nullptr;
    } // キーkはAVL木Tに存在しなかった
    if (cmp_(k, x->key)) {
      x->left = erase(x->left, k);
      return balance(x);
    } // xの適切な子に再帰し、
    if (cmp_(x->key, k)) {
      x->right = erase(x->right, k);
      return balance(x);
    } // xを根とする部分木を高さ平衡にする
    node *y = x->left,
         *z = x->right; // x.key == kのとき、yをxの左の子、zをxの右の子とし、
    destroy_node(x); // xを解放する
    if (z == nullptr) {
      return y;
    } // zがNILを指しているならば、新たな部分木の根としてyを返す
    node *w = leftmost(
        z); // zがNILを指していないならば、wをzを根とする部分木の中で最も左の子とする
    w->right = erase__(
        z); // zに対して再帰する.そして、wの右の子に新たな部分木を受け取る
    w->left = y;       // yをwの左の子にする
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
  node *balance(node *x) {
    x->h = reheight(x); // xの高さを更新する
    if (bias(x) > 1) {  // 左に2つ分偏っている場合、left-left
                        // caseおよびleft-right caseが考えられる
      if (bias(x->left) < 0) {
        x->left = rotate(x->left, 0, 1);
      } // left-right caseならば、左回転を行うことで、left-left caseに帰着させる
      return rotate(x, 1, 0); // 右回転を行うことでleft-left
                              // caseを解消し、高さ平衡を満たす部分木の根を返す
    }
    if (bias(x) < -1) { // 右に2つ分偏っている場合、right-right
                        // caseおよびright-left caseが考えられる
      if (bias(x->right) > 0) {
        x->right = rotate(x->right, 1, 0);
      } // right-left caseならば、右回転を行うことで、right-right
        // caseに帰着させる
      return rotate(x, 0, 1); // 左回転を行うことでright-right
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
  node *rotate(node *x, sides_t i, sides_t j) {
    node *y = x->c[j];  // yをxのjの子とする
    x->c[j] = y->c[i];  // yのi部分木をxのj部分木にする
    y->c[i] = x;        // xをyのiの子にする
    x->h = reheight(x); // xの高さを更新する
    y->h = reheight(y); // yの高さを更新する
    return y;           // 部分木の新しい根yを返す
  }
  /**
   * @brief 節点xを根とする部分木の中から最も左にある子を取得する
   * @param node*x 節点x
   */
  node *leftmost(node *x) { return x->left ? leftmost(x->left) : x; }

  /**
   * @brief xを根とする部分木の最も左の子を削除する補助関数
   * @param node*x 節点x
   */
  node *erase__(node *x) {
    if (x->left == nullptr) {
      return x->right;
    }
    x->left = erase__(x->left);
    return balance(x);
  }

private:
  /**< @brief 節点xの高さを取得する  */
  constexpr height_t height(node *x) noexcept { return x ? x->h : 0; }

  /**< @brief 節点xの更新される高さを返す */
  constexpr height_t reheight(node *x) noexcept {
    return std::max(height(x->left), height(x->right)) + 1;
  }

  /**< @brief 節点xの左右の子の高さの差(x.left - x.right)を返す */
  constexpr height_t bias(node *x) noexcept {
    return height(x->left) - height(x->right);
  }

private:
  /**< @brief 節点xの記憶領域の確保を行う */
  node *create_node(const Key &k, const T &v) {
    if (free_ == nullptr) {
      return nullptr;
    }
    node *x = free_;
    free_ = x->next;
    return construct(x, k, v);
  }

  /**< @brief 節点xの記憶領域の解放を行う */
  void destroy_node(node *x) noexcept {
    destroy(x);
    x->next = free_;
    free_ = x;
  }

  /**< @brief 節点n個分の記憶領域を確保する */
  node *allocate_nodes(std::size_t n) {
    return static_cast<node *>(::operator new(sizeof(node) * n, std::nothrow));
  }

  /**< @brief 節点xの記憶領域を解放する */
  void free_node(node *x) noexcept { ::operator delete(x); }

  /**< @brief 節点xを根とした部分木を再帰的に解放する*/
  void postorder_destroy_nodes(node *x) noexcept {
    if (x == nullptr) {
      return;
    }
    postorder_destroy_nodes(x->left);
    postorder_destroy_nodes(x->right);
    destroy_node(x);
  }

  /**< @brief メモリプールの解放 */
  void free_pool() noexcept {
    postorder_destroy_nodes(root_);
    free_node(pool_);
    root_ = pool_ = free_ = nullptr;
    size_ = cap_ = 0;
  }

  /**< @brief メモリプールの確保 */
  void allocate_pool(std::size_t n) {
    if (n == 0) {
      return;
    }
    pool_ = allocate_nodes(n); // 節点n個分の記憶領域確保
    if (pool_ == nullptr) {
      return;
    }
    for (std::size_t i = 0; i < n - 1; i++) {
      pool_[i].next = &pool_[i + 1];
    }
    pool_[n - 1].next = nullptr;
    free_ = pool_; // 空き節点へのポインタを保持しておく
    cap_ = n;
  }

private:
  /**< ＠brief キーlとキーrの非同値判定を行う */
  bool neq(const Key &l, const Key &r) { return (cmp_(l, r) || cmp_(r, l)); }
};

#endif // end of AVL_TREE_HPP
