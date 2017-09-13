#pragma once
#include <vector>

/* This is an optimized order book implementation.
 * Conceptually an order book is two sets of levels, with each
 * level representing all the queued orders at a price.
 *
 * This implementation provides O(1) lookup of the best bid and offer
 * as well as the aggregated quantity for any price. It provides
 * very fast throughput (61.5ns / tick at last benchmark), although
 * note that there are some poor performance worst-case ticks.
 *
 * The ITCH feed sends some metadata about each order with each add_order
 * message, such as the price, quantity and symbol (really a number
 * referring to the symbol). The user is expected after that to keep
 * track of the metadata; for instance the delete message only has
 * the order id and the user is expected to know which symbol
 * that refers to as well as the price and size.
 *
 * This implementation uses several tricks. The first is that, while
 * a hashmap seems like a reasonable data structure to keep track of
 * the metadata for each order, in practice the ITCH feed generates
 * the order ids close together, not going past a max id of several
 * hundred million. We thus store the metadata for each order in an
 * array, so looking up the order metadata is a single dereference.
 * The order id generation has a side benefit that new orders are
 * likely to be near recently added orders, so the pages are likely
 * to be in the TLB and memory cache.
 *
 * Each order knows which book and price level it belongs to. So
 * to find all the data about an order requires two to three
 * dereferences. Also the quantities in each price level are held
 * in a pointed to data structure so that a reduce operation does
 * not need to search the levels but can modify the quantity at the
 * level directly from the pointer stored in the order metadata.
 *
 * Another trick is that each price level is represented as a price
 * and quantity, and the levels are stored in a sorted array instead
 * of a tree. Averagely speaking most activity is near the top of
 * the book, so the implementation only needs to go 1-5 levels deep
 * into the book. Thus using an array is fast since it improves
 * locality and uses less memory. Note though that the worst case
 * performance could be bad - if somebody adds and deletes orders
 * far away from the inside of the book it could result in longer
 * processing for those messages.
 *
 * Lastly, since the orders and levels are stored in their own global
 * pools, they are likely to be local and there is very little pressure
 * on the allocator. In fact the only allocations are bulk allocations
 * from stl container resizing.
 */

// TODO replace casts with following:
#define MKPRIMITIVE(__x) ((std::underlying_type<decltype(__x)>::type)__x)

//#define TRACE 1

constexpr bool is_power_of_two(uint64_t n)
{  // stolen from linux header
  return (n != 0 && ((n & (n - 1)) == 0));
}

enum class sprice_t : int32_t {};
bool constexpr is_bid(sprice_t const x) { return int32_t(x) >= 0; }
#define MEMORY_DEFS    \
  using __ptr = ptr_t; \
  using size_t__ = typename std::underlying_type<ptr_t>::type;

/* A custom, pooling allocator. It uses a non-shrinking vector as its pool,
 * and a vector (LIFO stack) as its free list.
 * If there are no free locations, increment m_size, representing growing
 * the pool. If there is a free location, pop its address off the free list
 * and return that. To free an object, push its address onto the free list.
 * Note that pointers are not guaranteed to be stable across invocations
 * of `allocate`. To have a stable way of referencing an object use
 * `get` on its address.
 *
 * Performance: Since the pool and the free list are both represented
 * as arrays, an allocate is an increment if the free list is empty,
 * or a decrement and dereference (to the tail of the pool which is
 * likely in cache), if the free list is not empty.
 * A deallocate is a decrement and a write to memory.
 *
 * The implementation uses custom pointer types to save space, and
 * to preserve addresses if the underlying container is resized.
 * For instance we define `enum class order_id_t : uint32_t {}`
 * instead of (order *)
 */
template <class T, typename ptr_t, size_t SIZE_HINT>
class pool
{
 public:
  MEMORY_DEFS;
  std::vector<T> m_allocated;
  std::vector<ptr_t> m_free;
  pool() { m_allocated.reserve(SIZE_HINT); }
  pool(size_t reserve_size) { m_allocated.reserve(reserve_size); }
  T *get(ptr_t idx) { return &m_allocated[size_t__(idx)]; }
  T &operator[](ptr_t idx) { return m_allocated[size_t__(idx)]; }
#define ALLOC_INVARIANT \
  (m_free_size >= 0) /* aka can't free more than has been allocated */
  __ptr alloc(void)
  {
    if (m_free.empty()) {
      auto ret = __ptr(m_allocated.size());
      m_allocated.push_back(T());
      return ret;
    } else {
      auto ret = __ptr(m_free.back());
      m_free.pop_back();
      return ret;
    }
  }
  void free(__ptr idx) { m_free.push_back(idx); }
#undef ALLOC_INVARIANT
};
class level
{
 public:
  sprice_t m_price;
  qty_t m_qty;
  level(sprice_t __price, qty_t __qty) : m_price(__price), m_qty(__qty) {}
  level() {}
};

enum class book_id_t : uint16_t {};
enum class level_id_t : uint32_t {};
enum class order_id_t : uint32_t {};

/* A datatype representing an order. Since this order book only wants
 * to know the size at each level it just remembers its quantity. If
 * one wanted to maintain knowledge about the queue it would probably
 * be fast to maintain a doubly linked list.
 * It also remember which book it belongs to and its price level. This
 * is so that given just an oid we can look up the book structure
 * as well as just the price level if we want to modify the quantity
 * at the level without searching for it in the book.
 */
typedef struct order {
  qty_t m_qty;
  level_id_t level_idx;
  book_id_t book_idx;
} order_t;

class price_level
{
 public:
  price_level() {}
  price_level(sprice_t __price, level_id_t __ptr)
      : m_price(__price), m_ptr(__ptr)
  {
  }
  sprice_t m_price;
  level_id_t m_ptr;
};

template <class T>
class oidmap
{
 public:
  std::vector<T> m_data;
  size_t m_size;
  void reserve(order_id_t const oid)
  {
    size_t const idx = size_t(oid);
    if (idx >= m_data.size()) {
      m_data.resize(idx + 1);
    }
  }
  T &operator[](order_id_t const oid)
  {
    size_t const idx = size_t(oid);
    return m_data[idx];
  }
  T *get(order_id_t const oid)
  {
    size_t const idx = size_t(oid);
    return &m_data[idx];
  }
};

bool operator>(price_level a, price_level b)
{
  return int32_t(a.m_price) > int32_t(b.m_price);
}

struct order_id_hash {
  size_t operator()(order_id_t const id) const { return size_t(id); }
};

qty_t operator+(qty_t const a, qty_t const b)
{
  return qty_t(MKPRIMITIVE(a) + MKPRIMITIVE(b));
}

class order_book
{
 public:
  static constexpr size_t MAX_BOOKS = 1 << 14;
  static constexpr size_t NUM_LEVELS = 1 << 20;
  static order_book *s_books;  // can we allocate this on the stack?
  static oidmap<order_t> oid_map;
  using level_vector = pool<level, level_id_t, NUM_LEVELS>;
  using sorted_levels_t = std::vector<price_level>;
  // A global allocator for all the price levels allocated by all the books.
  static level_vector s_levels;
  sorted_levels_t m_bids;
  sorted_levels_t m_offers;
  using level_ptr_t = level_vector::__ptr;

  static void add_order(order_id_t const oid, book_id_t const book_idx,
                        sprice_t const price, qty_t const qty)
  {
#if TRACE
    printf("ADD %lu, %u, %d, %u", oid, book_idx, price, qty);
#endif  // TRACE
    oid_map.reserve(oid);
    order *order = oid_map.get(oid);
    order->m_qty = qty;
    order->book_idx = book_idx;

    s_books[size_t(book_idx)].ADD_ORDER(order, price, qty);
#if TRACE
    auto lvl = oid_map[oid].level_idx;
    printf(", %u, %u \n", lvl, s_books[size_t(book_idx)].s_levels[lvl].m_qty);
#endif  // TRACE
  }
  void ADD_ORDER(order_t *order, sprice_t const price, qty_t const qty)
  {
    sorted_levels_t *sorted_levels = is_bid(price) ? &m_bids : &m_offers;
    // search descending for the price
    auto insertion_point = sorted_levels->end();
    bool found = false;
    while (insertion_point-- != sorted_levels->begin()) {
      price_level &curprice = *insertion_point;
      if (curprice.m_price == price) {
        order->level_idx = curprice.m_ptr;
        found = true;
        break;
      } else if (price > curprice.m_price) {
        // insertion pt will be -1 if price < all prices
        break;
      }
    }
    if (!found) {
      order->level_idx = s_levels.alloc();
      s_levels[order->level_idx].m_qty = qty_t(0);
      s_levels[order->level_idx].m_price = price;
      price_level const px(price, order->level_idx);
      ++insertion_point;
      sorted_levels->insert(insertion_point, px);
    }
    s_levels[order->level_idx].m_qty = s_levels[order->level_idx].m_qty + qty;
  }
  static void delete_order(order_id_t const oid)
  {
#if TRACE
    printf("DELETE %lu\n", oid);
#endif  // TRACE
    order_t *order = oid_map.get(oid);
    s_books[size_t(order->book_idx)].DELETE_ORDER(order);
  }
  static void cancel_order(order_id_t const oid, qty_t const qty)
  {
#if TRACE
    printf("REDUCE %lu, %u\n", oid, qty);
#endif  // TRACE
    order_t *order = oid_map.get(oid);
    s_books[size_t(order->book_idx)].REDUCE_ORDER(order, qty);
  }
  // shared between cancel(aka partial cancel aka reduce) and execute
  void REDUCE_ORDER(order_t *order, qty_t const qty)
  {
    auto tmp = MKPRIMITIVE(s_levels[order->level_idx].m_qty);
    tmp -= MKPRIMITIVE(qty);
    s_levels[order->level_idx].m_qty = qty_t(tmp);

    tmp = MKPRIMITIVE(order->m_qty);
    tmp -= MKPRIMITIVE(qty);
    order->m_qty = qty_t(tmp);
  }
  // shared between delete and execute
  void DELETE_ORDER(order_t *order)
  {
    assert(MKPRIMITIVE(s_levels[order->level_idx].m_qty) >=
           MKPRIMITIVE(order->m_qty));
    auto tmp = MKPRIMITIVE(s_levels[order->level_idx].m_qty);
    tmp -= MKPRIMITIVE(order->m_qty);
    s_levels[order->level_idx].m_qty = qty_t(tmp);
    if (qty_t(0) == s_levels[order->level_idx].m_qty) {
      // DELETE_SORTED([order->level_idx].price);
      sprice_t price = s_levels[order->level_idx].m_price;
      sorted_levels_t *sorted_levels = is_bid(price) ? &m_bids : &m_offers;
      auto it = sorted_levels->end();
      while (it-- != sorted_levels->begin()) {
        if (it->m_price == price) {
          sorted_levels->erase(it);
          break;
        }
      }
      s_levels.free(order->level_idx);
    }
  }
  static void execute_order(order_id_t const oid, qty_t const qty)
  {
#if TRACE
    printf("EXECUTE %lu %u\n", oid, qty);
#endif  // TRACE
    order_t *order = oid_map.get(oid);
    order_book *book = &s_books[MKPRIMITIVE(order->book_idx)];

    if (qty == order->m_qty) {
      book->DELETE_ORDER(order);
    } else {
      book->REDUCE_ORDER(order, qty);
    }
  }
  static void replace_order(order_id_t const old_oid, order_id_t const new_oid,
                            qty_t const new_qty, sprice_t new_price)
  {
#if TRACE
    printf("REPLACE %lu %lu %d %u\n", old_oid, new_oid, new_price, new_qty);
#endif  // TRACE
    order_t *order = oid_map.get(old_oid);
    order_book *book = &s_books[MKPRIMITIVE(order->book_idx)];
    bool const bid = is_bid(book->s_levels[order->level_idx].m_price);
    book->DELETE_ORDER(order);
    if (!bid) {
      new_price = sprice_t(-1 * MKPRIMITIVE(new_price));
    }
    book->add_order(new_oid, order->book_idx, new_price, new_qty);
  }
};

oidmap<order_t> order_book::oid_map = oidmap<order_t>();
order_book *order_book::s_books = new order_book[order_book::MAX_BOOKS];
order_book::level_vector order_book::s_levels = level_vector();
