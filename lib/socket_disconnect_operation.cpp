///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include <cppcoro/net/socket_disconnect_operation.hpp>
#include <cppcoro/net/socket.hpp>

#include "socket_helpers.hpp"

#include <system_error>

#if CPPCORO_OS_WINNT
# include <winsock2.h>
# include <ws2tcpip.h>
# include <mswsock.h>
# include <windows.h>

bool cppcoro::net::socket_disconnect_operation_impl::try_start(
	cppcoro::detail::win32_overlapped_operation_base& operation) noexcept
{
	// Lookup the address of the DisconnectEx function pointer for this socket.
	LPFN_DISCONNECTEX disconnectExPtr;
	{
		GUID disconnectExGuid = WSAID_DISCONNECTEX;
		DWORD byteCount = 0;
		const int result = ::WSAIoctl(
			m_socket.native_handle(),
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			static_cast<void*>(&disconnectExGuid),
			sizeof(disconnectExGuid),
			static_cast<void*>(&disconnectExPtr),
			sizeof(disconnectExPtr),
			&byteCount,
			nullptr,
			nullptr);
		if (result == SOCKET_ERROR)
		{
			operation.m_errorCode = static_cast<DWORD>(::WSAGetLastError());
			return false;
		}
	}

	// Need to read this flag before starting the operation, otherwise
	// it may be possible that the operation will complete immediately
	// on another thread and then destroy the socket before we get a
	// chance to read it.
	const bool skipCompletionOnSuccess = m_socket.skip_completion_on_success();

	// Need to add TF_REUSE_SOCKET to these flags if we want to allow reusing
	// a socket for subsequent connections once the disconnect operation
	// completes.
	const DWORD flags = 0;

	const BOOL ok = disconnectExPtr(
		m_socket.native_handle(),
		operation.get_overlapped(),
		flags,
		0);
	if (!ok)
	{
		const int errorCode = ::WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING)
		{
			// Failed synchronously.
			operation.m_errorCode = static_cast<DWORD>(errorCode);
			return false;
		}
	}
	else if (skipCompletionOnSuccess)
	{
		// Successfully completed synchronously and no completion event
		// will be posted to an I/O thread so we can return without suspending.
		operation.m_errorCode = ERROR_SUCCESS;
		return false;
	}

	return true;
}

void cppcoro::net::socket_disconnect_operation_impl::cancel(
	cppcoro::detail::win32_overlapped_operation_base& operation) noexcept
{
	(void)::CancelIoEx(
		reinterpret_cast<HANDLE>(m_socket.native_handle()),
		operation.get_overlapped());
}

void cppcoro::net::socket_disconnect_operation_impl::get_result(
	cppcoro::detail::win32_overlapped_operation_base& operation)
{
	if (operation.m_errorCode != ERROR_SUCCESS)
	{
		if (operation.m_errorCode == ERROR_OPERATION_ABORTED)
		{
			throw operation_cancelled{};
		}

		throw std::system_error{
			static_cast<int>(operation.m_errorCode),
			std::system_category(),
			"Disconnect operation failed: DisconnectEx"
		};
	}
}
#elif CPPCORO_OS_LINUX
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netinet/udp.h>

bool cppcoro::net::socket_disconnect_operation_impl::try_start(
	cppcoro::detail::linux_async_operation_base& operation) noexcept
{
	operation.m_completeFunc = [operation, this]() {
		operation.m_mq->remove_fd_watch(m_socket.native_handle());
		int res = m_socket.close();
		return res;
	};
	operation.m_mq->add_fd_watch(m_socket.native_handle(), reinterpret_cast<void*>(&operation), EPOLLOUT);
	return true;
}

void cppcoro::net::socket_disconnect_operation_impl::cancel(
	cppcoro::detail::linux_async_operation_base& operation) noexcept
{
	operation.m_mq->remove_fd_watch(m_socket.native_handle());
}

void cppcoro::net::socket_disconnect_operation_impl::get_result(
	cppcoro::detail::linux_async_operation_base& operation)
{
	if (operation.m_res < 0)
	{
		if (operation.m_res == -cppcoro::detail::error_operation_cancelled)
		{
			throw operation_cancelled{};
		}

		throw std::system_error{
			static_cast<int>(-operation.m_res),
			std::system_category(),
			"Disconnect operation failed: disconnect"
		};
	}
}
#endif
