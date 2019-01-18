This is a very fast implementation of the ITCH order book, clocking in at around 61ns / tick (or 16 million messages / second, tested on a 2012 i7-3820), offering fast updates and O(1) access to any price level (to get the price is a single dereference, the aggregate quantity is another dereference). It only calculates the aggregate quantities at each price and does not track the queue depth for each order.

Among other things, there is no hashtable or trees used, only `<vector>`s and C arrays, and no allocation beyond what `<vector>` does. For description and implementation see [order_book.h](order_book.h).

Protocol specification: http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf (ITCH 5.0). Binary file spec: http://www.nasdaqtrader.com/content/technicalSupport/specifications/dataproducts/binaryfile.pdf.

In order to run it, `./build.sh && ./a.out < [file]`. Note that the implementation is fast enough that you will likely to be I/O bound - in order to find out how fast it really is you should 'warm-up' by loading the file into the buffer cache using `cat [file] > /dev/null`. Sample files available at ftp://emi.nasdaq.com/ITCH/MMDDYYYY.NASDAQ_ITCH50.gz.
