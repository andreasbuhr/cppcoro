///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include <cppcoro/net/socket_send_operation.hpp>
#include <cppcoro/net/socket.hpp>

#if CPPCORO_OS_WINNT
# include <winsock2.h>
# include <ws2tcpip.h>
# include <mswsock.h>
# include <windows.h>

bool cppcoro::net::socket_send_operation_impl::try_start(
	cppcoro::detail::win32_overlapped_operation_base& operation) noexcept
{
	// Need to read this flag before starting the operation, otherwise
	// it may be possible that the operation will complete immediately
	// on another thread and then destroy the socket before we get a
	// chance to read it.
	const bool skipCompletionOnSuccess = m_socket.skip_completion_on_success();

	DWORD numberOfBytesSent = 0;
	int result = ::WSASend(
		m_socket.native_handle(),
		reinterpret_cast<WSABUF*>(&m_buffer),
		1, // buffer count
		&numberOfBytesSent,
		0, // flags
		operation.get_overlapped(),
		nullptr);
	if (result == SOCKET_ERROR)
	{
		int errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// Failed synchronously.
			operation.m_errorCode = static_cast<DWORD>(errorCode);
			operation.m_numberOfBytesTransferred = numberOfBytesSent;
			return false;
		}
	}
	else if (skipCompletionOnSuccess)
	{
		// Completed synchronously, no completion event will be posted to the IOCP.
		operation.m_errorCode = ERROR_SUCCESS;
		operation.m_numberOfBytesTransferred = numberOfBytesSent;
		return false;
	}

	// Operation will complete asynchronously.
	return true;
}

void cppcoro::net::socket_send_operation_impl::cancel(
	cppcoro::detail::win32_overlapped_operation_base& operation) noexcept
{
	(void)::CancelIoEx(
		reinterpret_cast<HANDLE>(m_socket.native_handle()),
		operation.get_overlapped());
}
#elif CPPCORO_OS_LINUX
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netinet/udp.h>

bool cppcoro::net::socket_send_operation_impl::try_start(
	cppcoro::detail::linux_async_operation_base& operation) noexcept
{
	operation.m_completeFunc = [operation, this]() {
		int res = send(m_socket.native_handle(), m_buffer, m_byteCount, 0);
		operation.m_mq->remove_fd_watch(m_socket.native_handle());
		return res;
	};
	operation.m_mq->add_fd_watch(m_socket.native_handle(), reinterpret_cast<void*>(&operation), EPOLLOUT);
	return true;
}

void cppcoro::net::socket_send_operation_impl::cancel(
	cppcoro::detail::linux_async_operation_base& operation) noexcept
{
	operation.m_mq->remove_fd_watch(m_socket.native_handle());
}

#endif
