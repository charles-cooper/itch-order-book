#pragma once
enum class itch_t
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
using MSG = itch_t;
template<MSG __type> unsigned char netlen;
template<> constexpr unsigned char
netlen<MSG::SYSEVENT>					= 12;
template<> constexpr unsigned char
netlen<MSG::STOCK_DIRECTORY>			= 39;
template<> constexpr unsigned char
netlen<MSG::TRADING_ACTION>				= 25;
template<> constexpr unsigned char
netlen<MSG::REG_SHO_RESTRICT>			= 20;
template<> constexpr unsigned char
netlen<MSG::MPID_POSITION>				= 26;
template<> constexpr unsigned char
netlen<MSG::MWCB_DECLINE>				= 35;
template<> constexpr unsigned char
netlen<MSG::MWCB_STATUS>				= 12;
template<> constexpr unsigned char
netlen<MSG::IPO_QUOTE_UPDATE>			= 28;
template<> constexpr unsigned char
netlen<MSG::ADD_ORDER>					= 36;
template<> constexpr unsigned char
netlen<MSG::ADD_ORDER_MPID>				= 40;
template<> constexpr unsigned char
netlen<MSG::EXECUTE_ORDER>				= 31;
template<> constexpr unsigned char
netlen<MSG::EXECUTE_ORDER_WITH_PRICE>	= 36;
template<> constexpr unsigned char
netlen<MSG::REDUCE_ORDER>				= 23;
template<> constexpr unsigned char
netlen<MSG::DELETE_ORDER>				= 19;
template<> constexpr unsigned char
netlen<MSG::REPLACE_ORDER>				= 35;
template<> constexpr unsigned char
netlen<MSG::TRADE>						= 44;
template<> constexpr unsigned char
netlen<MSG::CROSS_TRADE>				= 40;
template<> constexpr unsigned char
netlen<MSG::BROKEN_TRADE>				= 19;
template<> constexpr unsigned char
netlen<MSG::NET_ORDER_IMBALANCE>		= 50;
template<> constexpr unsigned char
netlen<MSG::RETAIL_PRICE_IMPROVEMENT>	= 20;

template<itch_t __code>
struct itch_message
{
	static constexpr itch_t code = __code;
};

enum class BUY_SELL : char {
	BUY='B',
	SELL='S'
};
template<>
struct itch_message<MSG::ADD_ORDER>
{
	uint64_t timestamp;
	uint64_t oid;
	uint32_t price;
	uint32_t qty;
	uint16_t stock_locate;
	BUY_SELL buy;
};
using add_order_t = itch_message<MSG::ADD_ORDER>;
template<>
struct itch_message<MSG::ADD_ORDER_MPID>
{
	add_order_t add_msg;
	char mpid[4];
};
using add_order_mpid_t = itch_message<MSG::ADD_ORDER_MPID>;
template<>
struct itch_message<MSG::EXECUTE_ORDER>
{
	uint64_t oid;
	uint64_t timestamp;
	uint32_t exec_shares;
	uint16_t stock_locate;
};
using execute_order_t = itch_message<MSG::EXECUTE_ORDER>;
template<>
struct itch_message<MSG::EXECUTE_ORDER_WITH_PRICE>
{
	execute_order_t exec;
	// ignore printable and price
};
using execute_with_price_t = itch_message<MSG::EXECUTE_ORDER_WITH_PRICE>;
template<>
struct itch_message<MSG::REDUCE_ORDER>
{
	uint64_t oid;
	uint64_t timestamp;
	uint32_t shares;
};
using order_reduce_t = itch_message<MSG::REDUCE_ORDER>;
template<>
struct itch_message<MSG::DELETE_ORDER>
{
	uint64_t oid;
	uint64_t timestamp;
};
using order_delete_t = itch_message<MSG::DELETE_ORDER>;
template<>
struct itch_message<MSG::REPLACE_ORDER>
{
	uint64_t oid;
	uint64_t new_order_id;
	uint64_t new_price;
	uint32_t new_shares;
};
using order_replace_t = itch_message<MSG::REPLACE_ORDER>;


