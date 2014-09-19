
// use custom pointers, __size_t, to save space.
template<class T, typename ptr_t, prim_t N>
class fixed_array_allocator
{
	using __size_t = ptr_t;
	__size_t m_size = 0;
	__size_t m_first_free = 0;
	__size_t m_last_free = 0;
	__size_t const BUF_MASK;
	bool constexpr IS_VALID(__size_t idx) {
		return idx < N;
	}

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
		static_assert(std::numeric_limits<__size_t>::max() > N);
		memset(m_allocated, 0, sizeof(m_allocated));
		memset(m_free, 0, sizeof(m_allocated));
	}
	T *get(__size_t idx) {
		return &m_allocated[idx];
	}
	T &operator[](__size_t idx) {
		return m_allocated[idx];
	}
	__size_t alloc(void) {
		if (first_free == last_free) {
			__size_t ret = m_size++;
			assert(IS_VALID(m_size));
			return ret++;
		}
		return m_free[(m_first_free++) & BUF_MASK];
	}
	void free(__size_t idx) {
		m_free[(m_last_free++) & BUF_MASK] = idx;
	}
};
class book
{
	static constexpr MAX_BOOKS = 1<<15;
	static constexpr MAX_LEVELS = 1<<12;
	static constexpr MAX_ORDERS = 1<<24;
	static book s_books[MAX_BOOKS]; // can we allocate this on the stack??
	typedef struct book_ptr
	{
		uint16_t book_idx;
		uint16_t level_idx;
		uint32_t order_idx;
	} book_ptr_t;
	static std::unordered_map<order_id_t, order_ptr_t> oid_map;
	using fixed_array_allocator<level, uint16_t, (1<<12)> = level_vector;
	level_vector m_levels;
	using order_vector = fixed_array_allocator<order, uint32_t, (1<<24)>;
	order_vector m_orders;
	using price_t = uint32_t;
	using qty_t = int32_t;
	class order
	{
	public :
		qty_t m_qty;
		// if one wanted to maintain queue positions and other such nasty
		// stuff, it would probably be fast if you maintained a doubly
		// linked list:
#if 0
		uint32_t next = -1;
		uint32_t prev = -1;
#endif
		order(qty_t __qty) :
			m_qty(__qty)
		{}
	};
	class level
	{
	public :
		price_t m_price;
		qty_t m_qty;
		level(price_t __price, qty_t __qty) :
			m_price(__price), m_qty(__qty)
		{}
	};
	using level_ptr_t = level_vector::__size_t;
	using order_idx_t = level_vector::__size_t;
	// TODO remove static signature on these things and put in object
	static void add_order(order_id_t const oid, book_id_t const __book_idx, price_t const __price, qty_t const signed_qty) {
		order_ptr_t ptr;
		ptr.book_idx = __book_idx;
		book *__book = &s_books[ptr.book_idx];

		// allocate price level
		ptr.level_idx = __book->SEARCH_LEVELS(__price);
		if (!m_levels::IS_VALID(__lidx)) {
			ptr.level_idx = __book->m_levels.alloc();
			__book->m_levels[ptr.level_idx] = level(__price, signed_qty);
			__book->INSERT_LEVEL(__price, ptr.level_idx);
		}
		__book->m_levels[__lidx].qty += signed_qty;
		//__book->m_levels[idx].num_orders += 1;

		// allocate order
		ptr.order_idx = __book->m_orders.alloc();
		__book->m_orders[ptr.order_idx].qty = signed_qty;

		assert(!oid_map.count(oid));
		oid_map[oid] = ptr;
	}
	static void delete_order(order_id_t const oid) {
		order_ptr_t ptr = oid_map[oid];
		DELETE_ORDER(ptr);
	}
	static int SIGN(int32_t arg) {
		return arg < 0 ? -1 : 1;
	}
	static void cancel_order(order_id_t const oid, uint32_t const __qty) {
		s_books[ptr.book_idx]->REDUCE_ORDER(ptr, __qty);
	}
	// shared between cancel(aka partial cancel aka reduce) and execute
	void REDUCE_ORDER(order_ptr_t const ptr, uint32_t const __qty) {
		this->m_levels[ptr.level_idx].qty -= __qty;
		this->m_orders[ptr.order_idx].qty -= __qty;
	}
	// shared between delete and execute
	void DELETE_ORDER(order_ptr_t const ptr) {
		assert(m_levels[ptr.level_idx].qty
				>= m_orders[ptr.order_idx].qty);
		m_levels[ptr.level_idx].qty
			-= m_orders[ptr.order_idx].qty;
		if (0==m_levels[ptr.level_idx].qty) {
			DELETE_LEVEL(__price, ptr.level_idx);
			m_levels.free(ptr.level_idx);
		}
		m_orders.free(ptr.order_idx);
	}
	static void execute_order(order_id_t const oid, uint32_t const __qty) {
		order_ptr_t const ptr = oid_map[oid];
		book *__book = s_books[ptr.book_idx];

		if (__qty == __book->m_orders[ptr.order_idx].qty) {
			__book->DELETE_ORDER(ptr);
		} else {
			__book->REDUCE_ORDER(ptr, __qty);
		}
	}
}

