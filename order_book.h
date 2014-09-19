#pragma once
#include <unordered_map>

// TODO replace casts with following:
#define MKPRIMITIVE(__x) ((std::underlying_type<decltype(__x)>::type)__x)

constexpr bool is_power_of_two(uint64_t n) { // stolen from linux header
	return (n != 0 && ((n & (n - 1)) == 0));
}

enum class sprice_t : int32_t {};
bool constexpr is_bid(sprice_t const x) { return int32_t(x) >= 0; }

#define MEMORY_DEFS \
	using __ptr = ptr_t; \
using __size_t = typename std::underlying_type<ptr_t>::type; \
static constexpr __size_t N = __size_t(MAX_SIZE); \
static bool constexpr IS_VALID(__size_t idx) { \
	return idx < N; \
} \
static bool constexpr IS_VALID(ptr_t ptr) { return IS_VALID(__size_t(ptr)); } \

// use custom pointers, __size_t, to save space.
template<class T, typename ptr_t, ptr_t MAX_SIZE>
class fixed_array_allocator
{
	public :
		MEMORY_DEFS;
		__size_t m_size = 0;
		__size_t m_first_free = 0;
		__size_t m_last_free = 0;
		__size_t const BUF_MASK;
		// maintain a fixed size array. allocate requires two __size_t increments
		T m_allocated[N];
		// maintain a FIFO queue of free locations. this is implemented
		// as a deque in a circular buffer so that calls to alloc or free
		// only require one pointer dereference. another option would be
		// to use an array stack starting at zero but calls to free
		// would require memmove, thrashing the cache
		__size_t m_free[N];
		fixed_array_allocator() : BUF_MASK(N - 1)
	{
		static_assert(is_power_of_two(N), "pool size must be power of two!");
		//static_assert(std::numeric_limits<__size_t>::max() > N, "");
		memset(m_allocated, 0, sizeof(m_allocated));
		memset(m_free, 0, sizeof(m_allocated));
	}
		T *get(ptr_t idx) {
			return &m_allocated[__size_t(idx)];
		}
		T &operator[](ptr_t idx) {
			return m_allocated[__size_t(idx)];
		}
		__ptr alloc(void) {
			if (m_first_free == m_last_free) {
				__size_t ret = m_size++;
				assert(IS_VALID(m_size));
				return __ptr(ret++);
			}
			return __ptr(m_free[(m_first_free++) & BUF_MASK]);
		}
		void free(__ptr idx) {
			m_free[(m_last_free++) & BUF_MASK] = __size_t(idx);
		}
};
class level
{
	public :
		sprice_t m_price;
		qty_t m_qty;
		level(sprice_t __price, qty_t __qty) :
			m_price(__price), m_qty(__qty)
	{}
		level() {}
};
class order
{
	public :
		qty_t m_qty;
		/* if one wanted to maintain queue positions and other such nasty
		 * stuff, it would probably be fast if you maintained a doubly
		 * linked list:
		 * uint32_t next = -1;
		 * uint32_t prev = -1;
		 */
		order() {}
		order(qty_t __qty) :
			m_qty(__qty)
	{}
};
enum class book_id_t : uint16_t {};
#define DETYPE_BOOK(__book_idx) uint16_t(__book_idx)
enum class level_id_t : uint16_t {};
enum class order_id_t : uint32_t {};
typedef struct order_ptr
{
	book_id_t book_idx;
	level_id_t level_idx;
	order_id_t order_idx;
} order_ptr_t;

class price_level // such confusing name
{
	public :
		price_level() {}
		price_level(sprice_t __price, level_id_t __ptr) :
			m_price(__price), m_ptr(__ptr) {}
		sprice_t m_price;
		level_id_t m_ptr;
};

template<typename T, typename ptr_t, size_t MAX_SIZE>
class fixed_size_array
{
	public :
		MEMORY_DEFS;
		size_t m_size = 0;
		T m_data[MAX_SIZE];
		void push_back(T &val) {
			m_data[m_size++] = val;
		}
		void pop_back(void) {
			--m_size;
		}
		void insert(T const &val, size_t idx) {
			assert(idx <= m_size);
			memmove(&m_data[idx+1], &m_data[idx], m_size - idx);
			m_data[idx] = val;
			++m_size;
		}
		void erase(size_t idx) {
			memmove(&m_data[idx], &m_data[idx+1], m_size - idx);
			--m_size;
		}
};

bool operator > (price_level a, price_level b) {
	return int32_t(a.m_price) > int32_t(b.m_price);
}

struct order_id_hash {
	size_t operator()(order_id_t const id) const {
		return size_t(id);
	}
};

qty_t operator + (qty_t const a, qty_t const b) {
	return qty_t(MKPRIMITIVE(a) + MKPRIMITIVE(b));
}

class order_book
{
	public :
		static constexpr size_t MAX_BOOKS = 1<<14;
		static constexpr size_t MAX_LEVELS = 1<<15;
		static constexpr size_t MAX_ORDERS = 1<<20;
		static order_book *s_books;//[MAX_BOOKS]; // can we allocate this on the stack??
		static std::unordered_map<order_id_t, order_ptr_t, order_id_hash> oid_map;
		using level_vector = fixed_array_allocator<level, level_id_t, level_id_t(MAX_LEVELS)>;
		level_vector m_levels;
		using order_vector = fixed_array_allocator<order, order_id_t, order_id_t(MAX_ORDERS)>;
		order_vector m_orders;
		using sorted_levels_t = fixed_size_array<price_level, level_id_t, MAX_LEVELS / 2>;
		sorted_levels_t m_bids;
		sorted_levels_t m_offers;
		using level_ptr_t = level_vector::__ptr;
		using order_idx_t = order_vector::__ptr;

		// TODO remove static signature on these things and put in object
		static void add_order(order_id_t const oid, book_id_t const book_idx, sprice_t const price, qty_t const qty) {
			assert(!oid_map.count(oid));
			oid_map[oid] = s_books[size_t(book_idx)].ADD_ORDER(book_idx, price, qty);
		}
		order_ptr_t ADD_ORDER(book_id_t const book_idx, sprice_t const price, qty_t const qty)
		{
			order_ptr_t ptr;
			ptr.book_idx = book_idx;

			sorted_levels_t *sorted_levels = is_bid(price) ? &m_bids : &m_offers;
			// search descending
			sorted_levels_t::__size_t insertion_point = sorted_levels->m_size;
			bool found = false;
			while (insertion_point--) {
				price_level &curprice = sorted_levels->m_data[insertion_point];
				if (curprice.m_price == price) {
					ptr.level_idx = curprice.m_ptr;
					found = true;
					break;
				} else if (price > curprice.m_price) {
					// insertion pt will be zero if price < all prices
					insertion_point++;
					break;
				}
			}
			if (!found) {
				ptr.level_idx = m_levels.alloc();
				m_levels[ptr.level_idx].m_qty = qty_t(0);
				m_levels[ptr.level_idx].m_price = price;
				price_level const px(price, ptr.level_idx);
				sorted_levels->insert(px, insertion_point);
			}
			m_levels[ptr.level_idx].m_qty = m_levels[ptr.level_idx].m_qty + qty;
			//book->m_levels[idx].num_orders += 1;

			// allocate order
			ptr.order_idx = m_orders.alloc();
			m_orders[ptr.order_idx].m_qty = qty;

			return ptr;
		}
		static void delete_order(order_id_t const oid) {
			order_ptr_t ptr = oid_map[oid];
			s_books[size_t(ptr.book_idx)].DELETE_ORDER(ptr);
		}
		static void cancel_order(order_id_t const oid, qty_t const qty) {
			order_ptr_t ptr = oid_map[oid];
			s_books[size_t(ptr.book_idx)].REDUCE_ORDER(ptr, qty);
		}
		// shared between cancel(aka partial cancel aka reduce) and execute
		void REDUCE_ORDER(order_ptr_t const ptr, qty_t const qty) {
			uint32_t tmp;

			tmp = uint32_t(m_levels[ptr.level_idx].m_qty);
			tmp -= uint32_t(qty);
			m_levels[ptr.level_idx].m_qty = qty_t(tmp);

			tmp = uint32_t(m_orders[ptr.order_idx].m_qty);
			tmp -= uint32_t(qty);
			m_orders[ptr.order_idx].m_qty = qty_t(tmp);
		}
		// shared between delete and execute
		void DELETE_ORDER(order_ptr_t const ptr) {
			uint32_t tmp;
			assert(uint32_t(m_levels[ptr.level_idx].m_qty)
					>= uint32_t(m_orders[ptr.order_idx].m_qty));
			tmp = uint32_t(m_levels[ptr.level_idx].m_qty);
			tmp -= uint32_t(m_orders[ptr.order_idx].m_qty);
			m_levels[ptr.level_idx].m_qty = qty_t(tmp);
			if (qty_t(0) == m_levels[ptr.level_idx].m_qty) {
				sprice_t price = m_levels[ptr.level_idx].m_price;
				sorted_levels_t *sorted_levels = is_bid(price) ?
					&m_bids : &m_offers;
				//DELETE([ptr.level_idx].price);
				//for (sorted_levels_t::__size_t i = sorted_levels->m_size - 1; i; --i) {
				sorted_levels_t::__size_t i = sorted_levels->m_size;
				while (i--) {
					if (sorted_levels->m_data[i].m_price == price) {
						sorted_levels->erase(i);
						break;
					}
				}
				m_levels.free(ptr.level_idx);
			}
			m_orders.free(ptr.order_idx);
			}
			static void execute_order(order_id_t const oid, qty_t const qty) {
				order_ptr_t const ptr = oid_map[oid];
				order_book *book = &s_books[DETYPE_BOOK(ptr.book_idx)];

				if (qty == book->m_orders[ptr.order_idx].m_qty) {
					book->DELETE_ORDER(ptr);
				} else {
					book->REDUCE_ORDER(ptr, qty);
				}
			}
			static void replace_order(order_id_t const old_oid, order_id_t const new_oid, sprice_t new_price, qty_t const new_qty)
			{
				order_ptr_t const ptr = oid_map[old_oid];
				order_book *book = &s_books[DETYPE_BOOK(ptr.book_idx)];
				bool bid = MKPRIMITIVE(book->m_levels[ptr.level_idx].m_price) >= 0;
				book->DELETE_ORDER(ptr);
				if (!bid) {
					new_price = sprice_t(-1*MKPRIMITIVE(new_price));
				}
				book->add_order(new_oid, ptr.book_idx, new_price, new_qty);
			}
		};

		std::unordered_map<order_id_t, order_ptr_t, order_id_hash> order_book::oid_map = std::unordered_map<order_id_t, order_ptr_t, order_id_hash>();
		order_book *order_book::s_books = new order_book[MAX_BOOKS];
