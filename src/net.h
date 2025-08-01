#pragma once

#include "arch.h"
#include "config.h"
#include "event.h"
#include "iobuf.h"
#include "net_builtin.h"
#include "str.h"
#include "timer.h"

struct mg_dns {
  const char *url;          // DNS server URL
  struct mg_connection *c;  // DNS server connection
};

struct mg_addr {
  uint8_t ip[16];    // Holds IPv4 or IPv6 address, in network byte order
  uint16_t port;     // TCP or UDP port in network byte order
  uint8_t scope_id;  // IPv6 scope ID
  bool is_ip6;       // True when address is IPv6 address
};

struct mg_mgr {
  struct mg_connection *conns;  // List of active connections
  struct mg_dns dns4;           // DNS for IPv4
  struct mg_dns dns6;           // DNS for IPv6
  int dnstimeout;               // DNS resolve timeout in milliseconds
  bool use_dns6;                // Use DNS6 server by default, see #1532
  unsigned long nextid;         // Next connection ID
  void *userdata;               // Arbitrary user data pointer
  void *tls_ctx;                // TLS context shared by all TLS sessions
  uint16_t mqtt_id;             // MQTT IDs for pub/sub
  void *active_dns_requests;    // DNS requests in progress
  struct mg_timer *timers;      // Active timers
  int epoll_fd;                 // Used when MG_EPOLL_ENABLE=1
  struct mg_tcpip_if *ifp;      // Builtin TCP/IP stack only. Interface pointer
  size_t extraconnsize;         // Builtin TCP/IP stack only. Extra space
  MG_SOCKET_TYPE pipe;          // Socketpair end for mg_wakeup()
#if MG_ENABLE_FREERTOS_TCP
  SocketSet_t ss;  // NOTE(lsm): referenced from socket struct
#endif
};

struct mg_connection {
  struct mg_connection *next;     // Linkage in struct mg_mgr :: connections
  struct mg_mgr *mgr;             // Our container
  struct mg_addr loc;             // Local address
  struct mg_addr rem;             // Remote address
  void *fd;                       // Connected socket, or LWIP data
  unsigned long id;               // Auto-incrementing unique connection ID
  struct mg_iobuf recv;           // Incoming data
  struct mg_iobuf send;           // Outgoing data
  struct mg_iobuf prof;           // Profile data enabled by MG_ENABLE_PROFILE
  struct mg_iobuf rtls;           // TLS only. Incoming encrypted data
  mg_event_handler_t fn;          // User-specified event handler function
  void *fn_data;                  // User-specified function parameter
  mg_event_handler_t pfn;         // Protocol-specific handler function
  void *pfn_data;                 // Protocol-specific function parameter
  char data[MG_DATA_SIZE];        // Arbitrary connection data
  void *tls;                      // TLS specific data
  unsigned is_listening : 1;      // Listening connection
  unsigned is_client : 1;         // Outbound (client) connection
  unsigned is_accepted : 1;       // Accepted (server) connection
  unsigned is_resolving : 1;      // Non-blocking DNS resolution is in progress
  unsigned is_arplooking : 1;     // Non-blocking ARP resolution is in progress
  unsigned is_connecting : 1;     // Non-blocking connect is in progress
  unsigned is_tls : 1;            // TLS-enabled connection
  unsigned is_tls_hs : 1;         // TLS handshake is in progress
  unsigned is_udp : 1;            // UDP connection
  unsigned is_websocket : 1;      // WebSocket connection
  unsigned is_mqtt5 : 1;          // For MQTT connection, v5 indicator
  unsigned is_hexdumping : 1;     // Hexdump in/out traffic
  unsigned is_draining : 1;       // Send remaining data, then close and free
  unsigned is_closing : 1;        // Close and free the connection immediately
  unsigned is_full : 1;           // Stop reads, until cleared
  unsigned is_tls_throttled : 1;  // Last TLS write: MG_SOCK_PENDING() was true
  unsigned is_resp : 1;           // Response is still being generated
  unsigned is_readable : 1;       // Connection is ready to read
  unsigned is_writable : 1;       // Connection is ready to write
};

void mg_mgr_poll(struct mg_mgr *, int ms);
void mg_mgr_init(struct mg_mgr *);
void mg_mgr_free(struct mg_mgr *);

struct mg_connection *mg_listen(struct mg_mgr *, const char *url,
                                mg_event_handler_t fn, void *fn_data);
struct mg_connection *mg_connect(struct mg_mgr *, const char *url,
                                 mg_event_handler_t fn, void *fn_data);
struct mg_connection *mg_wrapfd(struct mg_mgr *mgr, int fd,
                                mg_event_handler_t fn, void *fn_data);
void mg_connect_resolved(struct mg_connection *);
bool mg_send(struct mg_connection *, const void *, size_t);
size_t mg_printf(struct mg_connection *, const char *fmt, ...);
size_t mg_vprintf(struct mg_connection *, const char *fmt, va_list *ap);
bool mg_aton(struct mg_str str, struct mg_addr *addr);

// These functions are used to integrate with custom network stacks
struct mg_connection *mg_alloc_conn(struct mg_mgr *);
void mg_close_conn(struct mg_connection *c);
bool mg_open_listener(struct mg_connection *c, const char *url);

// Utility functions
bool mg_wakeup(struct mg_mgr *, unsigned long id, const void *buf, size_t len);
bool mg_wakeup_init(struct mg_mgr *);
struct mg_timer *mg_timer_add(struct mg_mgr *mgr, uint64_t milliseconds,
                              unsigned flags, void (*fn)(void *), void *arg);
struct mg_connection *mg_connect_svc(struct mg_mgr *mgr, const char *url,
                                     mg_event_handler_t fn, void *fn_data,
                                     mg_event_handler_t pfn, void *pfn_data);
