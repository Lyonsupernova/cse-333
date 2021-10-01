/*
* Copyright ©2020 Hal Perkins.  All rights reserved.  Permission is
* hereby granted to students registered for University of Washington
* CSE 333 for use solely during Fall Quarter 2020 for purposes of
* the course.  No other use, copying, distribution, or modification
* is permitted without prior written consent. Copyrights for
* third-party components of this work must be honored.  Instructors
* interested in reusing these course materials should contact the
* author.
*/

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

#define LENG 1024

extern "C" {
  #include "libhw1/CSE333.h"
}


namespace hw4 {

  ServerSocket::ServerSocket(uint16_t port) {
    port_ = port;
    listen_sock_fd_ = -1;
  }

  ServerSocket::~ServerSocket() {
    // Close the listening socket if it's not zero.  The rest of this
    // class will make sure to zero out the socket if it is closed
    // elsewhere.
    if (listen_sock_fd_ != -1)
      close(listen_sock_fd_);
    listen_sock_fd_ = -1;
  }

  bool ServerSocket::BindAndListen(int ai_family, int *listen_fd) {
    // Use "getaddrinfo," "socket," "bind," and "listen" to
    // create a listening socket on port port_.  Return the
    // listening socket through the output parameter "listen_fd".

    // STEP 1:
    if (ai_family != AF_INET &&
        ai_family != AF_UNSPEC &&
        ai_family != AF_INET6) {
      std::cerr << "invalid ai_family input" << std::endl;
      return false;
    }

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    // the input ai_family
    hints.ai_family = ai_family;
    // set the reliable stream
    hints.ai_socktype = SOCK_STREAM;
    // use wildcard "in6addr_any" address
    hints.ai_flags = AI_PASSIVE;
    // use v4-mapped v6 if no v6 found
    hints.ai_flags |= AI_V4MAPPED;
    // tcp protocol
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;
    int res = getaddrinfo(nullptr,
    std::to_string(port_).c_str(),
    &hints,
    &result);
    // if getaddrinfo failed, print out the error message
    if (res != 0) {
      std::cerr << "getaddrinfo() failed: ";
      std::cerr << gai_strerror(res) << std::endl;
      return false;
    }
    // - ai_family: whether to create an IPv4, IPv6, or "either" listening
    //   socket.  To specify IPv4, customers pass in AF_INET.  To specify
    //   IPv6, customers pass in AF_INET6.  To specify "either" (which
    //   leaves it up to BindAndListen() to pick, in which case it will
    //   typically try IPv4 first), customers pass in AF_UNSPEC.  AF_INET6
    //   can handle IPv6 and IPv4 clients on POSIX systems, while AF_UNSPEC
    //   might pick IPv4 and not be able to accept IPv6 connections.
    int fd = -1;
    for (struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
      fd = socket(rp->ai_family,
      rp->ai_socktype,
      rp->ai_protocol);
      if (fd == -1) {
        // Creating this socket failed.  So, loop to the next returned
        // result and try again.
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        fd = 0;
        continue;
      }

      // Configure the socket; we're setting a socket "option."  In
      // particular, we set "SO_REUSEADDR", which tells the TCP stack
      // so make the port we bind to available again as soon as we
      // exit, rather than waiting for a few tens of seconds to recycle it.
      int optval = 1;
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
      &optval, sizeof(optval));

      // Try binding the socket to the address and port number returned
      // by getaddrinfo().
      if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
        break;
      }

      // The bind failed.  Close the socket, then loop back around and
      // try the next address/port returned by getaddrinfo().
      close(fd);
      fd = -1;
    }
    // Free the structure returned by getaddrinfo().
    freeaddrinfo(result);

    // If we failed to bind, return failure.
    if (fd == -1) {
      std::cerr << "Failed to bind any address." << std::endl;
      return false;
    }
    // Success. Tell the OS that we want this to be a listening socket.
    if (listen(fd, SOMAXCONN) != 0) {
      std::cerr << "Failed to mark socket as listening: ";
      std::cerr << strerror(errno) << std::endl;
      close(fd);
      return false;
    }

    // Return to the client the listening file descriptor.
    listen_sock_fd_ = fd;
    *listen_fd = fd;
    return true;
  }

  bool ServerSocket::Accept(int *accepted_fd,
  std::string *client_addr,
  uint16_t *client_port,
  std::string *client_dnsname,
  std::string *server_addr,
  std::string *server_dnsname) {
    // Accept a new connection on the listening socket listen_sock_fd_.
    // (Block until a new connection arrives.)  Return the newly accepted
    // socket, as well as information about both ends of the new connection,
    // through the various output parameters.

    // STEP 2:
    // client's address
    struct sockaddr_storage caddr;
    // client address length
    socklen_t caddr_len = sizeof(caddr);
    int client_fd;
    while (1) {
      client_fd = accept(listen_sock_fd_,
      reinterpret_cast<struct sockaddr *>(&caddr),
      &caddr_len);
      if (client_fd < 0) {
        if ((errno == EINTR)
          || (errno == EAGAIN)
        || (errno == EWOULDBLOCK))
        continue;

        std::cerr << "Failure on accept: " << strerror(errno) << std::endl;
        return false;
      }
      break;
    }
    *accepted_fd = client_fd;
    struct sockaddr * addr = reinterpret_cast<struct sockaddr *>(&caddr);
    // AF_INET6 represents IPv6 address, stores port information
    if (addr->sa_family == AF_INET6) {
      // Print out the IPV6 address and port
      char astring[INET6_ADDRSTRLEN];
      struct sockaddr_in6 *in6 =
      reinterpret_cast<struct sockaddr_in6 *>(addr);
      // convert IPv6 from binary to string format
      inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
      *client_addr = std::string(astring);
      *client_port = htons(in6->sin6_port);
    } else if (addr->sa_family == AF_INET) {
      // Print out the IPV4 address and port
      char astring[INET_ADDRSTRLEN];
      struct sockaddr_in *in4 = reinterpret_cast<struct sockaddr_in *>(addr);
      // convert IPv4 from binary to string format
      inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
      *client_addr = std::string(astring);
      *client_port = htons(in4->sin_port);
    } else {
      std::cerr << "Address and port get failed" << std::endl;
      return false;
    }

    // store client's DNS information
    char hostname[LENG];
    if (getnameinfo(addr, caddr_len, hostname, LENG, nullptr, 0, 0) != 0) {
      std::cerr << "Failure on reverse DNS look up" << std::endl;
    }

    char hname[LENG];
    hname[0] = '\0';
    if (sock_family_ == AF_INET) {
      // The server is using an IPv4 address.
      struct sockaddr_in srvr;
      socklen_t srvrlen = sizeof(srvr);
      char addrbuf[INET_ADDRSTRLEN];
      getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
      inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
      *server_addr = std::string(addrbuf);
      // Get the server's dns name, or return it's IP address as
      // a substitute if the dns lookup fails.
      getnameinfo((const struct sockaddr *) &srvr,
      srvrlen, hname, 1024, nullptr, 0, 0);
      *server_dnsname = std::string(hname);
    } else {
      // The server is using an IPv6 address.
      struct sockaddr_in6 srvr;
      socklen_t srvrlen = sizeof(srvr);
      char addrbuf[INET6_ADDRSTRLEN];
      getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
      inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
      *server_addr = std::string(addrbuf);
      // Get the server's dns name, or return it's IP address as
      // a substitute if the dns lookup fails.
      getnameinfo((const struct sockaddr *) &srvr,
      srvrlen, hname, 1024, nullptr, 0, 0);
      *server_dnsname = std::string(hname);
    }
    *client_dnsname = std::string(hostname);
    return true;
  }

}  // namespace hw4
