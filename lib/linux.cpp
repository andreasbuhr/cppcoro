///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Microsoft
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#include <cppcoro/detail/linux.hpp>
#include <system_error>
#include <unistd.h>
#include <cstring>
#include <cassert>

namespace cppcoro
{
	namespace detail
	{
		namespace linux
		{
			message_queue::message_queue()
			{
				if (pipe2(m_pipefd, O_NONBLOCK) == -1) {
					throw std::system_error
					{
						static_cast<int>(errno),
						std::system_category(),
						"Error creating io_service: failed creating pipe"
					};
				}
				m_epollfd = safe_fd{create_epoll_fd()};
				m_ev.data.fd = m_pipefd[0];
				m_ev.events = EPOLLIN;
				add_fd_watch(m_pipefd[0], reinterpret_cast<void*>(m_pipefd[0]), EPOLLIN);
			}

			message_queue::~message_queue()
			{
				assert(close(m_pipefd[0]) == 0);
				assert(close(m_pipefd[1]) == 0);
			}

			void message_queue::add_fd_watch(int fd, void* cb, uint32_t events){
				struct epoll_event ev = {0};
				ev.data.ptr = cb;
				ev.events = events;
				if(epoll_ctl(m_epollfd.fd(), EPOLL_CTL_ADD, fd, &ev) == -1)
				{
					if (errno == EPERM) {
						// epoll returns EPERM on regular files because they are
						// always ready for read/write, we can just queue the callback to run
						enqueue_message(cb, CALLBACK_TYPE);
					} else {
						throw std::system_error
						{
							static_cast<int>(errno),
							std::system_category(),
							"message_queue: add_fd_watch failed"
						};
					}
				}
			}
			void message_queue::remove_fd_watch(int fd){
				if(epoll_ctl(m_epollfd.fd(), EPOLL_CTL_DEL, fd, NULL) == -1)
				{
					if (errno != EPERM) {
						throw std::system_error
						{
							static_cast<int>(errno),
							std::system_category(),
							"message_queue: remove_fd_watch failed"
						};
					}
				}
			}

			bool message_queue::enqueue_message(void* msg, message_type type)
			{
				message qmsg;
				std::memset(&qmsg, 0, sizeof(qmsg));
				qmsg.m_type = type;
				qmsg.m_ptr = msg;
				int status = write(m_pipefd[1], (const char*)&qmsg, sizeof(message));
				return status==-1?false:true;
			}

			bool message_queue::dequeue_message(void*& msg, message_type& type, bool wait)
			{
				struct epoll_event ev = {0};
				int nfds = epoll_wait(m_epollfd.fd(), &ev, 1, wait?-1:0);

				if(nfds == -1)
				{
					if (errno == EINTR || errno == EAGAIN) {
						return false;
					}
					throw std::system_error
					{
						static_cast<int>(errno),
						std::system_category(),
						"Error in epoll_wait run loop"
					};
				}

				if(nfds == 0 && !wait)
				{
					return false;
				}

				if(nfds == 0 && wait)
				{
					throw std::system_error
					{
						static_cast<int>(errno),
						std::system_category(),
						"Error in epoll_wait run loop"
					};
				}

				if (ev.data.fd == m_pipefd[0]) {
					message qmsg;
					ssize_t status = read(m_pipefd[0], (char*)&qmsg, sizeof(message));

					if(status == -1)
					{
						if (errno == EINTR || errno == EAGAIN) {
							return false;
						}
						throw std::system_error
						{
							static_cast<int>(errno),
							std::system_category(),
							"Error retrieving message from message queue: mq_receive"
						};
					}

					msg = qmsg.m_ptr;
					type = qmsg.m_type;
					return true;
				} else {
					msg = ev.data.ptr;
					type = CALLBACK_TYPE;
					return true;
				}
			}

			safe_fd create_event_fd()
			{
				int fd = eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK | EFD_CLOEXEC);

				if(fd == -1)
				{
					throw std::system_error
					{
						static_cast<int>(errno),
						std::system_category(),
						"Error creating io_service: event fd create"
					};
				}

				return safe_fd{fd};
			}

			safe_fd create_timer_fd()
			{
				int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

				if(fd == -1)
				{
					throw std::system_error
					{
						static_cast<int>(errno),
						std::system_category(),
						"Error creating io_service: timer fd create"
					};
				}

				return safe_fd{fd};
			}

			safe_fd create_epoll_fd()
			{
				int fd = epoll_create1(EPOLL_CLOEXEC);

				if(fd == -1)
				{
					throw std::system_error
					{
						static_cast<int>(errno),
						std::system_category(),
						"Error creating timer thread: epoll create"
					};
				}

				return safe_fd{fd};
			}

			void safe_fd::close() noexcept
			{
				if(m_fd != -1)
				{
					::close(m_fd);
					m_fd = -1;
				}
			}
		}
	}
}
