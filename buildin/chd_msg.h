#ifndef _CHD_MESSAGE_H
#define _CHD_MESSAGE_H
#include <iostream>
#include <stdint.h>
#include <string.h>

#include "Poco/Data/LOB.h"
#include "Poco/Data/LOBStream.h"
#include "Poco/BinaryReader.h"
#include "Poco/BinaryWriter.h"
#include "Poco/Net/Socket.h"
#include "Poco/MemoryStream.h"
#include "Poco/ByteOrder.h"
#include "Poco/Data/Session.h"
#include "Poco/Data/Statement.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/DateTime.h"
#include "Poco/MD5Engine.h"


// #include "CHD_EXCEPT"

#include "Poco/Logger.h"
#include "Poco/zlib.h"

#define OLD_FORMAT_EN 0

#if OLD_FORMAT_EN>0
typedef uint16_t MSG_LEN_TYPE;
typedef uint16_t MSG_ID_TYPE;
#define CHD_MSG_HEADER_SIZE 4
#define MSG_BYTE_ORDER NETWORK_BYTE_ORDER
#else

typedef uint32_t MSG_LEN_TYPE;
typedef uint16_t MSG_ID_TYPE;
#define CHD_MSG_HEADER_SIZE 10
#define MSG_BYTE_ORDER NATIVE_BYTE_ORDER
#endif

namespace chd{
extern "C" uint32_t crc32(uint32_t crc, const unsigned char *buf, int len);
}

class MsgOut
{
protected:
	Poco::Data::CLOB _clob;
	Poco::Buffer<char> &buf;
	Poco::MemoryOutputStream bos;
	Poco::BinaryWriter bw;
	bool have_header;

	void update()
	{
		if (have_header)
		{
			MSG_LEN_TYPE len = (MSG_LEN_TYPE) (this->bos.charsWritten() - CHD_MSG_HEADER_SIZE);
			MSG_ID_TYPE *pdst = (MSG_ID_TYPE*) this->buf.begin();
			pdst++;
#if OLD_FORMAT_EN>0
			len = Poco::ByteOrder::flipBytes(len);
#endif
			memcpy(pdst, &len, sizeof(len));
			if ((int) this->bos.charsWritten() >= (int) buf.capacity())
			                {
				std::cerr
				          << "MessageOut memmory not enough"
				          << std::endl;
				throw std::runtime_error("MessageOut memmory not enough");
			}

#if OLD_FORMAT_EN==0
			uint32_t crc = chd::crc32(0, 0, 0);
			const unsigned char *p = (const unsigned char *) this->buf.begin();
			crc = chd::crc32(crc, p, 6);
			crc = chd::crc32(crc, p + CHD_MSG_HEADER_SIZE, (int) len);
			uint32_t c = crc;
			memcpy((void*) (p + 2 + 4), (void*) &c, sizeof(c));
#endif
		}
	}
	MsgOut(const MsgOut &o);
	MsgOut();
	MsgOut(const MsgOut &&o);
	MsgOut &operator =(const MsgOut &o);

public:
	MsgOut(MSG_ID_TYPE messageType, Poco::Buffer<char> &_buf, bool _have_header = true)
					:
					  buf(_buf),
					  bos(buf.begin(), buf.size()),
					  bw(bos,
					      Poco::BinaryWriter::MSG_BYTE_ORDER),
					  have_header(_have_header)
	{
		if (have_header) {
#if OLD_FORMAT_EN==0
			MSG_LEN_TYPE messageLen = 0;
			uint32_t crc32 = 0;
			bw
			   << messageType
			   << messageLen
			   << crc32;
#else
			MSG_LEN_TYPE messageLen = 0;
			bw << messageType << messageLen;
#endif
		}
	}

	void update_header()
	{
		update();
	}

	virtual ~MsgOut()
	{
	}

	int sendto(Poco::Net::Socket &socket)
	{
		int n = this->size();
		const char *ptr = this->rawContent();
		while (n > 0)
		{
			int sent = socket.impl()->sendBytes(ptr, n);
			if (sent > 0)
			                {
				n -= sent;
			}
		}
		return this->size();
	}

	int sendto(Poco::Net::SocketImpl *socket)
	{
		int n = this->size();
		const char *ptr = this->rawContent();
		while (n > 0)
		{
			int sent = socket->sendBytes(ptr, n);
			if (sent > 0)
			                {
				n -= sent;
			}
		}
		return this->size();
	}

	template<typename T>
	Poco::BinaryWriter& operator <<(T value)
	{
		bw << value;
		// update_len();  no need to do this, for performance.
		return bw;
	}

	void append_raw(const char *data, int len)
	{
		bw.writeRaw(data, len);
		// update_len();no need to do this, for performance.
	}

	Poco::Data::CLOB &clob()
	{
		this->_clob.assignRaw(rawContent(), size());
		return this->_clob;
	}

	Poco::BinaryWriter&writer()
	{
		return this->bw;
	}

	const char *rawContent()
	{
		update();
		return this->buf.begin();
	}
	int size()
	{
		return (int) this->bos.charsWritten();
	}

	Poco::BinaryWriter & operator <<(MsgOut &o)
	{
		bw.writeRaw(o.rawContent(), o.size());
		return bw;
	}

	Poco::BinaryWriter & operator <<(Poco::Data::CLOB &o)
	{
		bw.writeRaw(o.rawContent(), o.size());
		return bw;
	}
};

class MessageOut
{
private:
	Poco::Data::CLOB _clob;
	Poco::Buffer<char> buf;
	Poco::MemoryOutputStream bos;
	Poco::BinaryWriter bw;
	bool have_header;
	bool assiged_clob;

	void update()
	{
		if (have_header)
		{
			MSG_LEN_TYPE len = (MSG_LEN_TYPE) (this->bos.charsWritten() - CHD_MSG_HEADER_SIZE);
			MSG_ID_TYPE *pdst = (MSG_ID_TYPE*) this->buf.begin();
			pdst++;
#if OLD_FORMAT_EN>0
			len = Poco::ByteOrder::flipBytes(len);
#endif
			memcpy(pdst, &len, sizeof(len));
			if ((int) this->bos.charsWritten() >= (int) buf.capacity())
			                {
				std::cerr
				          << "MessageOut memmory not enough"
				          << std::endl;
				throw std::runtime_error("MessageOut memmory not enough");
			}
#if OLD_FORMAT_EN==0
			uint32_t crc = chd::crc32(0, 0, 0);
			const unsigned char *p = (const unsigned char *) this->buf.begin();
			crc = chd::crc32((unsigned long) crc, p, 6);
			crc = chd::crc32((unsigned long) crc, p + CHD_MSG_HEADER_SIZE, (int) len);
			uint32_t c = crc;
			memcpy((void*) (p + 2 + 4), (void*) &c, sizeof(c));
#endif
		}
	}
	MessageOut(const MessageOut &o);
	MessageOut();
	MessageOut(const MessageOut &&o);
	MessageOut &operator =(const MessageOut &o);

public:
	MessageOut(MSG_ID_TYPE messageType, int size = 2048, bool _have_header = true)
					:
					  buf(size),
					  bos(buf.begin(), buf.size()),
					  bw(bos,
					      Poco::BinaryWriter::MSG_BYTE_ORDER),
					  have_header(_have_header)
	{
		if (have_header) {
#if OLD_FORMAT_EN==0
			MSG_LEN_TYPE messageLen = 0;
			uint32_t crc32 = 0;
			bw
			   << messageType
			   << messageLen
			   << crc32;
#else
			MSG_LEN_TYPE messageLen = 0;
			bw << messageType << messageLen;
#endif
		}
		assiged_clob = false;
	}

	void update_header()
	{
		update();
	}

	virtual ~MessageOut()
	{
	}

	int sendto(Poco::Net::Socket socket)
	{
		int n = this->size();
		const char *ptr = this->rawContent();
		while (n > 0)
		{
			int sent = socket.impl()->sendBytes(ptr, n);
			if (sent > 0)
			{
				n -= sent;
			}
			else if (sent < 0)
				return sent;
		}
		return this->size();
	}

	int sendto(Poco::Net::SocketImpl *socket)
	{
		int n = this->size();
		const char *ptr = this->rawContent();
		while (n > 0)
		{
			int sent = socket->sendBytes(ptr, n);
			if (sent > 0)
			                {
				n -= sent;
			}
			else if (sent < 0)
				return sent;
		}
		return this->size();
	}

	int sendto(int socket)
	{
		int n = this->size();
		const char *ptr = this->rawContent();
		while (n > 0)
		{
			int sent = 0;
#ifdef WIN32
			sent = send(socket, ptr, n, 0);
#else
			sent = write(socket, ptr, n);
#endif
			if (sent > 0)
			                {
				n -= sent;
			}
			else if (sent < 0)
				return sent;
		}
		return this->size();
	}

	template<typename T>
	Poco::BinaryWriter& operator <<(T value)
	{
		bw << value;
		// update_len();  no need to do this, for performance.
		return bw;
	}

	void append_raw(const char *data, int len)
	{
		bw.writeRaw(data, len);
		// update_len();no need to do this, for performance.
	}

	Poco::Data::CLOB &clob()
	{
		if (!assiged_clob) {
			this->_clob.assignRaw(rawContent(), size());
			assiged_clob = true;
		}
		return this->_clob;
	}

	Poco::BinaryWriter&writer()
	{
		return this->bw;
	}

	const char *rawContent()
	{
		update();
		return this->buf.begin();
	}
	int size()
	{
		return (int) this->bos.charsWritten();
	}

	char *begin(){
		update();
		return this->buf.begin();
	}
	char *begin_payload(){
		update();
		return this->buf.begin() + CHD_MSG_HEADER_SIZE;
	}
	int size_payload(){
		return this->size() - CHD_MSG_HEADER_SIZE;
	}

	void md5_sign_append(const std::string &key){
		char *stream = this->begin_payload();
		int stream_size = this->size_payload();
		Poco::MD5Engine md5;
		md5.update(stream, stream_size);
		md5.update(key);
		auto &dig = md5.digest();
		this->append_raw((char*)&dig[0], dig.size());
	}

	Poco::BinaryWriter & operator <<(MessageOut &o)
	{
		bw.writeRaw(o.rawContent(), o.size());
		return bw;
	}

	Poco::BinaryWriter & operator <<(Poco::Data::CLOB &o)
	{
		bw.writeRaw(o.rawContent(), o.size());
		return bw;
	}
};

class MessageIn
{
private:
	/*
	 * MemoryInputStream�� 0����, ��ʹ��CLOB������10%
	 */
	Poco::MemoryInputStream bos;
	Poco::BinaryReader br;
	int pkg_len;
	int offset;
	std::shared_ptr<std::vector<uint8_t>> my_buf_ptr;

public:
	const char *ptr_origin;
	const char *ptr;
	int len;
	MSG_ID_TYPE msgType;
	MSG_LEN_TYPE msgLen;       // payload len.

#if OLD_FORMAT_EN==0
	uint32_t crc32;       // crc32 calculate
	uint32_t crc32_rx;       // crc32_rx
#endif

	int package_len(void) const
	                {
		return pkg_len;
	}

	static MessageIn* copy_and_read_head(MessageIn& src) {
		auto buf_ptr = std::make_shared<std::vector<uint8_t>>();
		buf_ptr->resize(src.package_len());
		memcpy(buf_ptr->data(), src.ptr_origin, src.package_len());
		auto ret = new MessageIn((const char*)buf_ptr->data(), buf_ptr->size());
		ret->my_buf_ptr = buf_ptr;
		ret->ReadHead();

		return ret;
	}

public:
	MessageIn(const char *buff, int size)
					:
					  bos(buff, size),
					  br(bos,
					      Poco::BinaryReader::MSG_BYTE_ORDER),
					  pkg_len(size),
					  offset(
					      0),
					  ptr_origin(buff),
					  ptr(buff),
					  len(size),
					  msgType(0),
					  msgLen(0)
					      #if OLD_FORMAT_EN==0
					      ,
					  crc32(0),
					  crc32_rx(0)
	#endif

	{
	}

	MessageIn(std::shared_ptr<std::vector<uint8_t>> sbuf)
		:
		my_buf_ptr(sbuf),
		bos((const char*)(sbuf->data()), sbuf->size()),
		br(bos,
			Poco::BinaryReader::MSG_BYTE_ORDER),
		pkg_len(sbuf->size()),
		offset(
			0),
		ptr_origin((const char*)(sbuf->data())),
		ptr((const char*)(sbuf->data())),
		len(sbuf->size()),
		msgType(0),
		msgLen(0)
#if OLD_FORMAT_EN==0
		,
		crc32(0),
		crc32_rx(0)
#endif
	{
	}

	void restart()
	{
		ptr = ptr_origin;
		bos.seekg(0, std::ios::beg);
		offset = 0;
	}

private:
	uint8_t *__ptr0 = 0;
	int size_sign = 0;
public:
	void md5_sign_verify_begin(){
		__ptr0 = (uint8_t*)this->begin();
		size_sign = this->size();
	}
	bool md5_sign_verify(const std::string &key){
		if (__ptr0==0 || size_sign == 0){
			throw Poco::Exception("MessageIn md5sign verify missing begin");
		}
		uint8_t *__ptr = (uint8_t*)(__ptr0 + size_sign - 16 );
		Poco::MD5Engine __md5;
		__md5.update(__ptr0, __ptr-__ptr0);
		__md5.update(key);
		auto &__d__=__md5.digest();
		bool sign = true;
		for (int i=0; i<16; i++){
			if (__d__[i] != __ptr[i]){
				sign=false;
				break;
			}
		}
		return sign;
	}

	/*
	 * skip last head only
	 */
	void restart_skiphead()
	{
		bos.seekg(CHD_MSG_HEADER_SIZE, std::ios::beg);
		offset = CHD_MSG_HEADER_SIZE;
	}

	bool ReadHead(void)
	{
		if (size() < CHD_MSG_HEADER_SIZE)
		{
			return false;
		}
		ptr += offset;
		len -= offset;
		offset = 0;
		this->msgType = 0;
		this->msgLen = 0;
		(*this) >> this->msgType;
		(*this) >> this->msgLen;
#if OLD_FORMAT_EN==0
		(*this) >> this->crc32_rx;
#endif
		pkg_len = this->msgLen + CHD_MSG_HEADER_SIZE;
		return true;
	}

	bool verify()
	{
#if OLD_FORMAT_EN==0
		uint32_t crc = chd::crc32(0, 0, 0);
		const unsigned char *p = (const unsigned char *) ptr_origin;
		crc = chd::crc32(crc, p, 6);
		int n = std::min(this->len, (int) this->msgLen);
		if (n < 0) {
			return false;
		}
		crc = chd::crc32(crc, p + CHD_MSG_HEADER_SIZE, n);
		this->crc32 = crc;
		return crc32 == crc32_rx;
#else
		return true;
#endif
	}

	const char *begin()
	{
		return ptr + offset;
	}

	template<typename T>
	MessageIn& operator >>(T &value)
	{
		br >> value;
		offset += sizeof(value);
		return *this;
	}

	MessageIn& operator >>(std::string &value)
	{
		br >> value;
		int len_str = value.length();
		int len_marker = 0;
		unsigned int v = len_str;
		do
		{
			v >>= 7;
			len_marker++;
		} while (v);
		offset += len_str + len_marker;
		return *this;
	}

	void drop(int size)
	{
		offset += size;
		bos.seekg(offset, std::ios::beg);
	}

	std::size_t size()
	{
		return package_len() - offset;
	}

private:
	MessageIn(const MessageIn &o);

};


/*
 * example:
 *
		Poco::Data::Statement stm(sec);
		stm << "select * from friendship";
		stm.execute();
		record_set rs(stm);
		std::cout << "data:{\n";
		rs.writeto(std::cout);
		// rs.writeto(out);
		std::cout << "\n}" << std::endl;
 */
class record_set: public Poco::Data::RecordSet
{
public:
	const int *type_direct;
	record_set(Poco::Data::Statement &stm, const int *td = 0)
					: Poco::Data::RecordSet(stm),
					  type_direct(td)
	{
	}
	static const int DATETIME32 = 1;
	static const int DATETIME64 = 2;

	template <class T>
	void writeto(T &out, int limit = 0)
	{
		Poco::Data::RecordSet &rs = *this;
		bool more = rs.moveFirst();
		int n = rs.columnCount();
		for (int i = 0; more; i++) {
			for (int col = 0; col < n; col++) {
				if (!type_direct) {
					if (!rs[col].isEmpty())
					{
						out << rs[col].convert<std::string>();
					} else {
						out << "";
					}
				} else {
					if (type_direct[col] == 0){
						if (!rs[col].isEmpty())
						{
							out << rs[col].convert<std::string>();
						} else {
							out << "";
						}
					} else if (type_direct[col] == 8) {
						if (!rs[col].isEmpty())
						{
							out << (uint8_t) rs[col].convert<int>();
						} else {
							out << (uint8_t)0;
						}
					} else if (type_direct[col] == 32) {
						if (!rs[col].isEmpty())
						{
							out << (uint32_t) rs[col].convert<int32_t>();
						} else {
							out << (uint32_t)0;
						}
					} else if (type_direct[col] == 64) {
						if (!rs[col].isEmpty())
						{
							out << (uint64_t) rs[col].convert<int64_t>();
						}else {
							out << (uint64_t)0;
						}
					} else if (type_direct[col] == DATETIME64) {
						if (!rs[col].isEmpty())
						{
							out << (uint64_t) (rs[col].convert<Poco::DateTime>().timestamp().raw() / 1000000ULL);
						}else {
							out << (uint64_t)0;
						}
					} else if (type_direct[col] == DATETIME32) {
						if (!rs[col].isEmpty())
						{
							out << (uint32_t) (rs[col].convert<Poco::DateTime>().timestamp().raw() / 1000000ULL);
						}else {
							out << (uint32_t)0;
						}
					} else {
						if (!rs[col].isEmpty())
						{
							out << rs[col].convert<std::string>();
						}else {
							out<< "";
						}
					}
				}
			}
			more = rs.moveNext();
			if (limit && i>=limit){
				break;
			}
		}
	}
};

class pro_io
{
public:
	pro_io()
	{
	}
	virtual ~pro_io()
	{
	}

public:
	virtual bool write(MessageIn &in)
	{
		return false;
	}
	virtual bool read(MessageOut &out) const
	                {
		return false;
	}

};

#endif // !_MESSAGE_H
