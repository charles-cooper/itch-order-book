#include <cstdio>
#include <memory>
#include <limits>
#include "bufferedreader.h"
#include "itch.h"
#include "order_book.h"

template<itch_t __code>
class PROCESS
{
	public :
		static itch_message<__code> read_from(buf_t *__buf) {
			uint16_t const msglen = be16toh(*(uint16_t*)__buf->get(0));
			__buf->advance(2);
			assert(msglen == netlen<__code>);

			__buf->ensure(netlen<__code>);
			printf("%d %c\n", char(__code), char(__code));
			itch_message<__code> ret = itch_message<__code>::parse(__buf->get(0));
			__buf->advance(netlen<__code>);
			return ret;
		}
};

static sprice_t mksigned(price_t price, BUY_SELL buy)
{
	assert(MKPRIMITIVE(price) < std::numeric_limits<int32_t>::max());
	auto ret = MKPRIMITIVE(price);
	if (BUY_SELL::SELL == buy)
		ret = -ret;
	return sprice_t(ret);
}
#define DO_CASE(__itch_t) case (__itch_t) : { \
	PROCESS<__itch_t>::read_from(&buf); \
	break; \
}

int main()
{
	buf_t buf(1024);
	buf.fd = STDIN_FILENO;
	printf("HI\n");
	while (is_ok(buf.ensure(3)))
	{
		itch_t const msgtype = itch_t(*buf.get(2));
		switch (msgtype)
		{
			DO_CASE(itch_t::SYSEVENT);
			DO_CASE(itch_t::STOCK_DIRECTORY);
			DO_CASE(itch_t::TRADING_ACTION);
			DO_CASE(itch_t::REG_SHO_RESTRICT);
			DO_CASE(itch_t::MPID_POSITION);
			DO_CASE(itch_t::MWCB_DECLINE);
			DO_CASE(itch_t::MWCB_STATUS);
			DO_CASE(itch_t::IPO_QUOTE_UPDATE);
			DO_CASE(itch_t::TRADE);
			DO_CASE(itch_t::CROSS_TRADE);
			DO_CASE(itch_t::BROKEN_TRADE);
			DO_CASE(itch_t::NET_ORDER_IMBALANCE);
			DO_CASE(itch_t::RETAIL_PRICE_IMPROVEMENT);

			case (itch_t::ADD_ORDER) :
			{
				auto const pkt = PROCESS<itch_t::ADD_ORDER>::read_from(&buf);
				order_book::add_order(order_id_t(pkt.oid), book_id_t(pkt.stock_locate), mksigned(pkt.price, pkt.buy), pkt.qty);
				break;
			}
			case (itch_t::ADD_ORDER_MPID) :
			{
				auto const pkt = PROCESS<itch_t::ADD_ORDER_MPID>::read_from(&buf);
				order_book::add_order(order_id_t(pkt.add_msg.oid), book_id_t(pkt.add_msg.stock_locate), mksigned(pkt.add_msg.price, pkt.add_msg.buy), pkt.add_msg.qty);
				break;
			}
			case(itch_t::EXECUTE_ORDER) :
			{
				auto const pkt = PROCESS<itch_t::EXECUTE_ORDER>::read_from(&buf);
				order_book::execute_order(order_id_t(pkt.oid), pkt.qty);
				break;
			}
			case(itch_t::EXECUTE_ORDER_WITH_PRICE) :
			{
				auto const pkt = PROCESS<itch_t::EXECUTE_ORDER_WITH_PRICE>::read_from(&buf);
				order_book::execute_order(order_id_t(pkt.exec.oid), pkt.exec.qty);
				break;
			}
			case(itch_t::REDUCE_ORDER) :
			{
				auto const pkt = PROCESS<itch_t::REDUCE_ORDER>::read_from(&buf);
				order_book::cancel_order(order_id_t(pkt.oid), pkt.qty);
				break;
			}
			case(itch_t::DELETE_ORDER) :
			{
				auto const pkt = PROCESS<itch_t::DELETE_ORDER>::read_from(&buf);
				order_book::delete_order(order_id_t(pkt.oid));
				break;
			}
			case (itch_t::REPLACE_ORDER) :
			{
				auto const pkt = PROCESS<itch_t::REPLACE_ORDER>::read_from(&buf);
				order_book::replace_order(order_id_t(pkt.oid), order_id_t(pkt.new_order_id), mksigned(pkt.new_price, BUY_SELL::BUY), pkt.new_qty);
				// actually it will get re-signed inside. code smell
				break;
			}
			default :
			{
				printf("Uh oh bad code %d\n", char(msgtype));
				assert(false);
				break;
			}
		}
	}
}

