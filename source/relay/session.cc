//
// Aspia Project
// Copyright (C) 2020 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "relay/session.h"

#include "base/location.h"
#include "base/logging.h"
#include "base/strings/unicode.h"

#include <asio/write.hpp>

namespace relay {

Session::Session(std::pair<asio::ip::tcp::socket, asio::ip::tcp::socket>&& sockets)
    : socket_{ std::move(sockets.first), std::move(sockets.second) }
{
    for (size_t i = 0; i < kNumberOfSides; ++i)
        std::fill(buffer_[i].begin(), buffer_[i].end(), 0);
}

Session::~Session()
{
    stop();
}

void Session::start(Delegate* delegate)
{
    LOG(LS_INFO) << "Starting peers session";

    start_time_ = std::chrono::high_resolution_clock::now();
    delegate_ = delegate;

    for (int i = 0; i < kNumberOfSides; ++i)
        Session::doReadSome(this, i);
}

void Session::stop()
{
    if (!delegate_)
        return;

    delegate_ = nullptr;

    std::error_code ignored_code;
    for (int i = 0; i < kNumberOfSides; ++i)
    {
        socket_[i].cancel(ignored_code);
        socket_[i].close(ignored_code);
    }

    LOG(LS_INFO) << "Session stopped";
}

std::chrono::seconds Session::duration() const
{
    std::chrono::time_point<std::chrono::high_resolution_clock> current_time =
        std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time_);
}

int64_t Session::bytesTransferred() const
{
    return bytes_transferred_;
}

// static
void Session::doReadSome(Session* session, int source)
{
    session->socket_[source].async_read_some(
        asio::buffer(session->buffer_[source].data(), session->buffer_[source].size()),
        [session, source](const std::error_code& error_code, size_t bytes_transferred)
    {
        if (error_code)
        {
            if (error_code != asio::error::operation_aborted)
                session->onErrorOccurred(FROM_HERE, error_code);
        }
        else
        {
            session->bytes_transferred_ += bytes_transferred;

            asio::async_write(
                session->socket_[(source + kNumberOfSides - 1) % kNumberOfSides],
                asio::const_buffer(session->buffer_[source].data(), bytes_transferred),
                [session, source](const std::error_code& error_code, size_t bytes_transferred)
            {
                if (error_code)
                {
                    if (error_code != asio::error::operation_aborted)
                        session->onErrorOccurred(FROM_HERE, error_code);
                }
                else
                {
                    doReadSome(session, source);
                }
            });
        }
    });
}

void Session::onErrorOccurred(const base::Location& location, const std::error_code& error_code)
{
    LOG(LS_ERROR) << "Connection error: " << base::utf16FromLocal8Bit(error_code.message())
                  << " (" << location.toString() << ")";
    if (delegate_)
        delegate_->onSessionFinished(this);

    stop();
}

} // namespace relay
