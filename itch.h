
enum class itch_msg_t 
{
	SYSEVENT = 'S',
	STOCK_DIRECTORY = 'R',
	TRADING_ACTION = 'H',
	REG_SHO_RESTRICT = 'Y', // 20
	MPID_POSITION = 'L', // 26
	MWCB_DECLINE = 'V', // market wide circuit breaker // 35
	MWCB_STATUS = 'W',
	IPO_QUOTE_UPDATE = 'K', // 28
	ADD_ORDER = 'A', // 36
	ADD_ORDER_MPID = 'F',
	EXECUTE_ORDER = 'E',
	EXECUTE_ORDER_WITH_PRICE = 'C',
	REDUCE_ORDER = 'X',
	DELETE_ORDER = 'D',
	REPLACE_ORDER = 'U',
	TRADE = 'P',
	CROSS_TRADE = 'Q',
	BROKEN_TRADE = 'B',
	NET_ORDER_IMBALANCE = 'I',
	RETAIL_PRICE_IMPROVEMENT = 'N'
};
template<itch_t __code>
struct itch_message
{};

using MSG = itch_t;
template<MSG __type> unsigned char netlen;
netlen<MSG::SYSEVENT>					= 12;
netlen<MSG::STOCK_DIRECTORY>			= 39;
netlen<MSG::TRADING_ACTION>				= 25;
netlen<MSG::REG_SHO_RESTRICT>			= 20;
netlen<MSG::MPID_POSITION>				= 26;
netlen<MSG::MWCB_DECLINE>				= 35;
netlen<MSG::MWCB_STATUS>				= 12;
netlen<MSG::IPO_UPDATE>					= 28;
netlen<MSG::ADD_ORDER>					= 36;
netlen<MSG::ADD_ORDER_MPID>				= 40;
netlen<MSG::EXECUTE_ORDER>				= 31;
netlen<MSG::EXECUTE_ORDER_WITH_PRICE>	= 36;
netlen<MSG::REDUCE_ORDER>-				= 23;
netlen<MSG::DELETE_ORDER>-				= 19;
netlen<MSG::REPLACE_ORDER>-				= 35;
netlen<MSG::TRADE>-						= 44;
netlen<MSG::CROSS_TRADE>				= 40;
netlen<MSG::BROKEN_TRADE>				= 19;
netlen<MSG::NET_ORDER_IMBALANCE>		= 50;
netlen<MSG::RETAIL_PRICE_IMPROVEMENT>	= 20;

enum class BUY_SELL : char {
	BUY='B',
	SELL='S'
};
typedef struct itch_message<MSG::ADD_ORDER>
{
	uint64_t timestamp;
	uint64_t oid;
	uint32_t price;
	uint32_t qty;
	uint16_t stock_locate;
	BUY_SELL buy;
} add_order_t;
typedef struct itch_message<MSG::ADD_ORDER_MPID>
{
	add_order_t add_msg;
	char mpid[4];
} add_order_mpid_t;
typedef struct itch_message<MSG::EXECUTE_ORDER>
{
	uint64_t oid;
	uint64_t timestamp;
	uint32_t exec_shares;
	uint16_t stock_locate;
} execute_order_t;
typedef struct itch_message<MSG::EXECUTE_ORDER_WITH_PRICE>
{
	execute_order_t exec;
	// printable
	// price
} execute_with_price_t;
typedef struct itch_message<MSG::REDUCE_ORDER>
{
	uint64_t oid;
	uint64_t timestamp;
	uint32_t shares;
} order_reduce_t;
typedef struct itch_message<MSG::DELETE_ORDER>
{
	uint64_t oid;
	uint64_t timestamp;
} order_delete_t;
typedef struct itch_message<MSG::REPLACE_ORDER>
{
	uint64_t oid;
	uint64_t new_order_id;
	uint64_t new_price;
	uint32_t new_shares;
} order_replace_t;
typedef struct itch_message<MSG::TRADE>
{} trade_t;
typedef struct itch_message<MSG::CROSS_TRADE>
{} cross_t;
typedef struct itch_message<MSG::BROKEN_TRADE>
{} broken_trade_t;
typedef struct itch_message<MSG::NET_ORDER_IMBALANCE>
{} noii_t;
typedef struct itch_message<MSG::RETAIL_PRICE_IMPROVEMENT>
{} rpii_t;


