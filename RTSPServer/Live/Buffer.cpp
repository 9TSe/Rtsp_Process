#include "Buffer.h"
#include "SocketOps.h"
#include "../Scheduler/Log.h"

const int Buffer::initialsize = 1024;
const char* Buffer::m_kCRLF = "\r\n";

Buffer::Buffer()
	//:m_buffer(new char[initialsize], std::default_delete<char[]>())
	:m_buffersize(initialsize)
	,m_readindex(0)
	, m_writeindex(0)
{
	m_buffer = new char[initialsize];
}

Buffer::~Buffer()
{
	delete[] m_buffer;
}

const char* Buffer::findCRLF() const
{
	const char* crlf = std::search(peek(), beginwrite(), m_kCRLF, m_kCRLF + 2);
	return crlf == beginwrite() ? nullptr : crlf;
}

const char* Buffer::findCRLF(const char* start) const
{
	assert(start >= peek());
	assert(start <= beginwrite());
	const char* crlf = std::search(start, beginwrite(), m_kCRLF, m_kCRLF + 2);
	return crlf == beginwrite() ? nullptr : crlf;
}

const char* Buffer::findLastCRLF() const
{
	const char* crlf = std::find_end(peek(), beginwrite(), m_kCRLF, m_kCRLF + 2);
	return crlf == beginwrite() ? nullptr : crlf;
}

void Buffer::retrieve(int len)
{
	assert(len <= readable());
	if (len < readable())
		m_readindex += len;
	else
		retrieveAll();
}

void Buffer::retrieveUntil(const char* end)
{
	assert(end >= peek());
	assert(end <= beginwrite());
	retrieve(end - peek());
}

void Buffer::unwrite(int len)
{
	assert(len <= readable());
	m_writeindex -= len;
}

void Buffer::makeSpace(int len)
{
	if (len > prependable() + writeable())
	{
		m_buffersize = m_writeindex + len;
		m_buffer = (char*)realloc(m_buffer, m_buffersize);
		//m_buffer.reset(new char[m_buffersize]);
		if (!m_buffer)
		{
			LOG_CORE_ERROR("realloc error");
			return;
		}
	}
	else
	{
		int readable1 = readable();
		std::copy(begin() + m_readindex, begin() + m_writeindex, begin());
		m_writeindex = readable1;	
		m_readindex = 0;
	}
}

void Buffer::ensurewriteable(int len)
{
	if (len > writeable())
		makeSpace(len);
	assert(writeable() >= len);
}

void Buffer::append(const char* data, int len)
{
	ensurewriteable(len);
	std::copy(data, data + len, beginwrite());
	m_writeindex += len;
}

void Buffer::append(const void* data, int len)
{
	append((const char*)data, len);
}

int Buffer::read(int fd)
{
	char* recvbuf[65536];
	int n = ::recv(fd, recvbuf, sizeof(recvbuf), 0);
	if (n <= 0) return -1;
	else append(recvbuf, n);
	return n;
}

int Buffer::write(int fd)
{
	return sockets::write(fd, peek(), readable());
}