/**
 * @file eraft_vdb_server.cc
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-06-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <boost/asio.hpp>

#include <array>

#include "thread_pool.h"


using boost::asio::ip::tcp;

class TCPConnection : public std::enable_shared_from_this<TCPConnection> {

 public:
  TCPConnection(boost::asio::io_service &io_service)
      : socket_(io_service), strand_(io_service) {}

  tcp::socket &socket() {
    return socket_;
  }

  void Start() {
    doProcessRW();
  }

 private:
  void doProcessRW() {
    auto self = shared_from_this();
    socket_.async_read_some(
        boost::asio::buffer(r_buffer_, r_buffer_.size()),
        strand_.wrap([this, self](boost::system::error_code ec,
                                  std::size_t               bytes_transferred) {
          if (!ec) {
            auto self = shared_from_this();

            boost::asio::async_write(
                socket_,
                boost::asio::buffer("+OK"),
                strand_.wrap([this, self](boost::system::error_code ec,
                                          std::size_t bytes_transferred) {
                  if (!ec) {
                    doProcessRW();
                  }
                }));
          }
        }));
  }

 private:
  tcp::socket                     socket_;
  boost::asio::io_service::strand strand_;
  std::array<char, 8192>          r_buffer_;
};

class ERaftVdbServer {

 public:
  ERaftVdbServer(boost::asio::io_service &io_service, unsigned short port)
      : io_service_(io_service)
      , acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
    doAccept();
  }

  void doAccept() {
    auto conn = std::make_shared<TCPConnection>(io_service_);

    acceptor_.async_accept(conn->socket(),
                           [this, conn](boost::system::error_code ec) {
                             if (!ec) {
                               conn->Start();
                             }
                             this->doAccept();
                           });
  }

 private:
  boost::asio::io_service &io_service_;
  tcp::acceptor            acceptor_;
};

int main(int argc, char *argv[]) {

  AsioThreadPool pool(8);

  unsigned short port = 6379;

  ERaftVdbServer server(pool.GetIOService(), port);

  pool.Stop();

  return 0;
}