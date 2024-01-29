/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2019 Francis Deslauriers <francis.deslauriers@efficios.com>
 * Copyright 2016 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <glib.h>
#include <stdint.h>
#include <stdio.h>

#include <babeltrace2/babeltrace.h>

#include "common/common.h"
#include "compat/endian.h" /* IWYU pragma: keep  */

#include "data-stream.hpp"
#include "lttng-live.hpp"
#include "lttng-viewer-abi.hpp"
#include "metadata.hpp"
#include "viewer-connection.hpp"

#define viewer_handle_send_recv_status(_status, _action, _msg_str)                                 \
    do {                                                                                           \
        switch (_status) {                                                                         \
        case LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED:                                                 \
            break;                                                                                 \
        case LTTNG_LIVE_VIEWER_STATUS_ERROR:                                                       \
            BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,                                \
                                         "Error " _action " " _msg_str);                           \
            break;                                                                                 \
        default:                                                                                   \
            bt_common_abort();                                                                     \
        }                                                                                          \
    } while (0)

#define viewer_handle_send_status(_status, _msg_str)                                               \
    viewer_handle_send_recv_status(_status, "sending", _msg_str)

#define viewer_handle_recv_status(_status, _msg_str)                                               \
    viewer_handle_send_recv_status(_status, "receiving", _msg_str)

#define LTTNG_LIVE_CPPLOGE_APPEND_CAUSE_ERRNO(_msg, _fmt, ...)                                     \
    do {                                                                                           \
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, _msg ": {}" _fmt,                  \
                                     bt_socket_errormsg(), ##__VA_ARGS__);                         \
    } while (0)

static inline enum lttng_live_iterator_status
viewer_status_to_live_iterator_status(enum lttng_live_viewer_status viewer_status)
{
    switch (viewer_status) {
    case LTTNG_LIVE_VIEWER_STATUS_OK:
        return LTTNG_LIVE_ITERATOR_STATUS_OK;
    case LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED:
        return LTTNG_LIVE_ITERATOR_STATUS_AGAIN;
    case LTTNG_LIVE_VIEWER_STATUS_ERROR:
        return LTTNG_LIVE_ITERATOR_STATUS_ERROR;
    }

    bt_common_abort();
}

static inline enum ctf_msg_iter_medium_status
viewer_status_to_ctf_msg_iter_medium_status(enum lttng_live_viewer_status viewer_status)
{
    switch (viewer_status) {
    case LTTNG_LIVE_VIEWER_STATUS_OK:
        return CTF_MSG_ITER_MEDIUM_STATUS_OK;
    case LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED:
        return CTF_MSG_ITER_MEDIUM_STATUS_AGAIN;
    case LTTNG_LIVE_VIEWER_STATUS_ERROR:
        return CTF_MSG_ITER_MEDIUM_STATUS_ERROR;
    }

    bt_common_abort();
}

static inline void viewer_connection_close_socket(struct live_viewer_connection *viewer_connection)
{
    int ret = bt_socket_close(viewer_connection->control_sock);
    if (ret == -1) {
        BT_CPPLOGW_ERRNO_SPEC(viewer_connection->logger,
                              "Error closing viewer connection socket: ", ".");
    }

    viewer_connection->control_sock = BT_INVALID_SOCKET;
}

/*
 * This function receives a message from the Relay daemon.
 * If it received the entire message, it returns _OK,
 * If it's interrupted, it returns _INTERRUPTED,
 * otherwise, it returns _ERROR.
 */
static enum lttng_live_viewer_status
lttng_live_recv(struct live_viewer_connection *viewer_connection, void *buf, size_t len)
{
    ssize_t received;
    size_t total_received = 0, to_receive = len;
    struct lttng_live_msg_iter *lttng_live_msg_iter = viewer_connection->lttng_live_msg_iter;
    enum lttng_live_viewer_status status;
    BT_SOCKET sock = viewer_connection->control_sock;

    /*
     * Receive a message from the Relay.
     */
    do {
        received = bt_socket_recv(sock, (char *) buf + total_received, to_receive, 0);
        if (received == BT_SOCKET_ERROR) {
            if (bt_socket_interrupted()) {
                if (lttng_live_graph_is_canceled(lttng_live_msg_iter)) {
                    /*
                     * This interruption was due to a
                     * SIGINT and the graph is being torn
                     * down.
                     */
                    status = LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED;
                    lttng_live_msg_iter->was_interrupted = true;
                    goto end;
                } else {
                    /*
                     * A signal was received, but the graph
                     * is not being torn down. Carry on.
                     */
                    continue;
                }
            } else {
                /*
                 * For any other types of socket error, close
                 * the socket and return an error.
                 */
                LTTNG_LIVE_CPPLOGE_APPEND_CAUSE_ERRNO("Error receiving from Relay", ".");

                viewer_connection_close_socket(viewer_connection);
                status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
                goto end;
            }
        } else if (received == 0) {
            /*
             * The recv() call returned 0. This means the
             * connection was orderly shutdown from the other peer.
             * If that happens when we are trying to receive
             * a message from it, it means something when wrong.
             * Close the socket and return an error.
             */
            BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                         "Remote side has closed connection");
            viewer_connection_close_socket(viewer_connection);
            status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
            goto end;
        }

        BT_ASSERT(received <= to_receive);
        total_received += received;
        to_receive -= received;

    } while (to_receive > 0);

    BT_ASSERT(total_received == len);
    status = LTTNG_LIVE_VIEWER_STATUS_OK;

end:
    return status;
}

/*
 * This function sends a message to the Relay daemon.
 * If it send the message, it returns _OK,
 * If it's interrupted, it returns _INTERRUPTED,
 * otherwise, it returns _ERROR.
 */
static enum lttng_live_viewer_status
lttng_live_send(struct live_viewer_connection *viewer_connection, const void *buf, size_t len)
{
    enum lttng_live_viewer_status status;
    struct lttng_live_msg_iter *lttng_live_msg_iter = viewer_connection->lttng_live_msg_iter;
    BT_SOCKET sock = viewer_connection->control_sock;
    size_t to_send = len;
    ssize_t total_sent = 0;

    do {
        ssize_t sent = bt_socket_send_nosigpipe(sock, (char *) buf + total_sent, to_send);
        if (sent == BT_SOCKET_ERROR) {
            if (bt_socket_interrupted()) {
                if (lttng_live_graph_is_canceled(lttng_live_msg_iter)) {
                    /*
                     * This interruption was a SIGINT and
                     * the graph is being teared down.
                     */
                    status = LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED;
                    lttng_live_msg_iter->was_interrupted = true;
                    goto end;
                } else {
                    /*
                     * A signal was received, but the graph
                     * is not being teared down. Carry on.
                     */
                    continue;
                }
            } else {
                /*
                 * For any other types of socket error, close
                 * the socket and return an error.
                 */
                LTTNG_LIVE_CPPLOGE_APPEND_CAUSE_ERRNO("Error sending to Relay", ".");

                viewer_connection_close_socket(viewer_connection);
                status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
                goto end;
            }
        }

        BT_ASSERT(sent <= to_send);
        total_sent += sent;
        to_send -= sent;

    } while (to_send > 0);

    BT_ASSERT(total_sent == len);
    status = LTTNG_LIVE_VIEWER_STATUS_OK;

end:
    return status;
}

static int parse_url(struct live_viewer_connection *viewer_connection)
{
    char error_buf[256] = {0};
    struct bt_common_lttng_live_url_parts lttng_live_url_parts = {};
    int ret = -1;

    if (viewer_connection->url.empty()) {
        goto end;
    }

    lttng_live_url_parts = bt_common_parse_lttng_live_url(viewer_connection->url.c_str(), error_buf,
                                                          sizeof(error_buf));
    if (!lttng_live_url_parts.proto) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Invalid LTTng live URL format: {}",
                                     error_buf);
        goto end;
    }
    viewer_connection->proto.reset(lttng_live_url_parts.proto);
    lttng_live_url_parts.proto = NULL;

    viewer_connection->relay_hostname.reset(lttng_live_url_parts.hostname);
    lttng_live_url_parts.hostname = NULL;

    if (lttng_live_url_parts.port >= 0) {
        viewer_connection->port = lttng_live_url_parts.port;
    } else {
        viewer_connection->port = LTTNG_DEFAULT_NETWORK_VIEWER_PORT;
    }

    viewer_connection->target_hostname.reset(lttng_live_url_parts.target_hostname);
    lttng_live_url_parts.target_hostname = NULL;

    if (lttng_live_url_parts.session_name) {
        viewer_connection->session_name.reset(lttng_live_url_parts.session_name);
        lttng_live_url_parts.session_name = NULL;
    }

    ret = 0;

end:
    bt_common_destroy_lttng_live_url_parts(&lttng_live_url_parts);
    return ret;
}

static enum lttng_live_viewer_status
lttng_live_handshake(struct live_viewer_connection *viewer_connection)
{
    struct lttng_viewer_cmd cmd;
    struct lttng_viewer_connect connect;
    enum lttng_live_viewer_status status;
    const size_t cmd_buf_len = sizeof(cmd) + sizeof(connect);
    char cmd_buf[cmd_buf_len];

    BT_CPPLOGD_SPEC(viewer_connection->logger,
                    "Handshaking with the relay daemon: cmd={}, major-version={}, minor-version={}",
                    LTTNG_VIEWER_CONNECT, LTTNG_LIVE_MAJOR, LTTNG_LIVE_MINOR);

    cmd.cmd = htobe32(LTTNG_VIEWER_CONNECT);
    cmd.data_size = htobe64((uint64_t) sizeof(connect));
    cmd.cmd_version = htobe32(0);

    connect.viewer_session_id = -1ULL; /* will be set on recv */
    connect.major = htobe32(LTTNG_LIVE_MAJOR);
    connect.minor = htobe32(LTTNG_LIVE_MINOR);
    connect.type = htobe32(LTTNG_VIEWER_CLIENT_COMMAND);

    /*
     * Merge the cmd and connection request to prevent a write-write
     * sequence on the TCP socket. Otherwise, a delayed ACK will prevent the
     * second write to be performed quickly in presence of Nagle's algorithm
     */
    memcpy(cmd_buf, &cmd, sizeof(cmd));
    memcpy(cmd_buf + sizeof(cmd), &connect, sizeof(connect));

    status = lttng_live_send(viewer_connection, &cmd_buf, cmd_buf_len);
    if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_send_status(status, "viewer connect command");
        goto end;
    }

    status = lttng_live_recv(viewer_connection, &connect, sizeof(connect));
    if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(status, "viewer connect reply");
        goto end;
    }

    BT_CPPLOGI_SPEC(viewer_connection->logger, "Received viewer session ID : {}",
                    (uint64_t) be64toh(connect.viewer_session_id));
    BT_CPPLOGI_SPEC(viewer_connection->logger, "Relayd version : {}.{}", be32toh(connect.major),
                    be32toh(connect.minor));

    if (LTTNG_LIVE_MAJOR != be32toh(connect.major)) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Incompatible lttng-relayd protocol");
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto end;
    }
    /* Use the smallest protocol version implemented. */
    if (LTTNG_LIVE_MINOR > be32toh(connect.minor)) {
        viewer_connection->minor = be32toh(connect.minor);
    } else {
        viewer_connection->minor = LTTNG_LIVE_MINOR;
    }
    viewer_connection->major = LTTNG_LIVE_MAJOR;

    status = LTTNG_LIVE_VIEWER_STATUS_OK;

    goto end;

end:
    return status;
}

static enum lttng_live_viewer_status
lttng_live_connect_viewer(struct live_viewer_connection *viewer_connection)
{
    struct hostent *host;
    struct sockaddr_in server_addr;
    enum lttng_live_viewer_status status;

    if (parse_url(viewer_connection)) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Failed to parse URL");
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto error;
    }

    BT_CPPLOGD_SPEC(
        viewer_connection->logger,
        "Connecting to hostname : {}, port : {}, target hostname : {}, session name : {}, proto : {}",
        viewer_connection->relay_hostname->str, viewer_connection->port,
        !viewer_connection->target_hostname ? "<none>" : viewer_connection->target_hostname->str,
        !viewer_connection->session_name ? "<none>" : viewer_connection->session_name->str,
        viewer_connection->proto->str);

    host = gethostbyname(viewer_connection->relay_hostname->str);
    if (!host) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Cannot lookup hostname: hostname=\"{}\"",
                                     viewer_connection->relay_hostname->str);
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto error;
    }

    if ((viewer_connection->control_sock = socket(AF_INET, SOCK_STREAM, 0)) == BT_INVALID_SOCKET) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Socket creation failed: {}",
                                     bt_socket_errormsg());
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto error;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(viewer_connection->port);
    server_addr.sin_addr = *((struct in_addr *) host->h_addr);
    memset(&(server_addr.sin_zero), 0, 8);

    if (connect(viewer_connection->control_sock, (struct sockaddr *) &server_addr,
                sizeof(struct sockaddr)) == BT_SOCKET_ERROR) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Connection failed: {}",
                                     bt_socket_errormsg());
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto error;
    }

    status = lttng_live_handshake(viewer_connection);

    /*
     * Only print error and append cause in case of error. not in case of
     * interruption.
     */
    if (status == LTTNG_LIVE_VIEWER_STATUS_ERROR) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Viewer handshake failed");
        goto error;
    } else if (status == LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED) {
        goto end;
    }

    goto end;

error:
    if (viewer_connection->control_sock != BT_INVALID_SOCKET) {
        if (bt_socket_close(viewer_connection->control_sock) == BT_SOCKET_ERROR) {
            BT_CPPLOGW_SPEC(viewer_connection->logger, "Error closing socket: {}.",
                            bt_socket_errormsg());
        }
    }
    viewer_connection->control_sock = BT_INVALID_SOCKET;
end:
    return status;
}

static void lttng_live_disconnect_viewer(struct live_viewer_connection *viewer_connection)
{
    if (viewer_connection->control_sock == BT_INVALID_SOCKET) {
        return;
    }
    if (bt_socket_close(viewer_connection->control_sock) == BT_SOCKET_ERROR) {
        BT_CPPLOGW_SPEC(viewer_connection->logger, "Error closing socket: {}",
                        bt_socket_errormsg());
        viewer_connection->control_sock = BT_INVALID_SOCKET;
    }
}

static int list_update_session(bt_value *results, const struct lttng_viewer_session *session,
                               bool *_found, struct live_viewer_connection *viewer_connection)
{
    int ret = 0;
    uint64_t i, len;
    bt_value *map = NULL;
    bt_value *hostname = NULL;
    bt_value *session_name = NULL;
    bt_value *btval = NULL;
    bool found = false;

    len = bt_value_array_get_length(results);
    for (i = 0; i < len; i++) {
        const char *hostname_str = NULL;
        const char *session_name_str = NULL;

        map = bt_value_array_borrow_element_by_index(results, i);
        hostname = bt_value_map_borrow_entry_value(map, "target-hostname");
        if (!hostname) {
            BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                         "Error borrowing \"target-hostname\" entry.");
            ret = -1;
            goto end;
        }
        session_name = bt_value_map_borrow_entry_value(map, "session-name");
        if (!session_name) {
            BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                         "Error borrowing \"session-name\" entry.");
            ret = -1;
            goto end;
        }
        hostname_str = bt_value_string_get(hostname);
        session_name_str = bt_value_string_get(session_name);

        if (strcmp(session->hostname, hostname_str) == 0 &&
            strcmp(session->session_name, session_name_str) == 0) {
            int64_t val;
            uint32_t streams = be32toh(session->streams);
            uint32_t clients = be32toh(session->clients);

            found = true;

            btval = bt_value_map_borrow_entry_value(map, "stream-count");
            if (!btval) {
                BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                             "Error borrowing \"stream-count\" entry.");
                ret = -1;
                goto end;
            }
            val = bt_value_integer_unsigned_get(btval);
            /* sum */
            val += streams;
            bt_value_integer_unsigned_set(btval, val);

            btval = bt_value_map_borrow_entry_value(map, "client-count");
            if (!btval) {
                BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                             "Error borrowing \"client-count\" entry.");
                ret = -1;
                goto end;
            }
            val = bt_value_integer_unsigned_get(btval);
            /* max */
            val = bt_max_t(int64_t, clients, val);
            bt_value_integer_unsigned_set(btval, val);
        }

        if (found) {
            break;
        }
    }
end:
    *_found = found;
    return ret;
}

static int list_append_session(bt_value *results, const std::string& base_url,
                               const struct lttng_viewer_session *session,
                               struct live_viewer_connection *viewer_connection)
{
    int ret = 0;
    bt_value_map_insert_entry_status insert_status;
    bt_value_array_append_element_status append_status;
    bt_value *map = NULL;
    bool found = false;

    /*
     * If the session already exists, add the stream count to it,
     * and do max of client counts.
     */
    ret = list_update_session(results, session, &found, viewer_connection);
    if (ret || found) {
        goto end;
    }

    map = bt_value_map_create();
    if (!map) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Error creating map value.");
        ret = -1;
        goto end;
    }

    if (base_url.empty()) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Error: base_url length smaller than 1.");
        ret = -1;
        goto end;
    }

    /*
     * key = "url",
     * value = <string>,
     */
    insert_status = bt_value_map_insert_string_entry(
        map, "url",
        fmt::format("{}/host/{}/{}", base_url, session->hostname, session->session_name).c_str());
    if (insert_status != BT_VALUE_MAP_INSERT_ENTRY_STATUS_OK) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Error inserting \"url\" entry.");
        ret = -1;
        goto end;
    }

    /*
     * key = "target-hostname",
     * value = <string>,
     */
    insert_status = bt_value_map_insert_string_entry(map, "target-hostname", session->hostname);
    if (insert_status != BT_VALUE_MAP_INSERT_ENTRY_STATUS_OK) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Error inserting \"target-hostname\" entry.");
        ret = -1;
        goto end;
    }

    /*
     * key = "session-name",
     * value = <string>,
     */
    insert_status = bt_value_map_insert_string_entry(map, "session-name", session->session_name);
    if (insert_status != BT_VALUE_MAP_INSERT_ENTRY_STATUS_OK) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Error inserting \"session-name\" entry.");
        ret = -1;
        goto end;
    }

    /*
     * key = "timer-us",
     * value = <integer>,
     */
    {
        uint32_t live_timer = be32toh(session->live_timer);

        insert_status = bt_value_map_insert_unsigned_integer_entry(map, "timer-us", live_timer);
        if (insert_status != BT_VALUE_MAP_INSERT_ENTRY_STATUS_OK) {
            BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                         "Error inserting \"timer-us\" entry.");
            ret = -1;
            goto end;
        }
    }

    /*
     * key = "stream-count",
     * value = <integer>,
     */
    {
        uint32_t streams = be32toh(session->streams);

        insert_status = bt_value_map_insert_unsigned_integer_entry(map, "stream-count", streams);
        if (insert_status != BT_VALUE_MAP_INSERT_ENTRY_STATUS_OK) {
            BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                         "Error inserting \"stream-count\" entry.");
            ret = -1;
            goto end;
        }
    }

    /*
     * key = "client-count",
     * value = <integer>,
     */
    {
        uint32_t clients = be32toh(session->clients);

        insert_status = bt_value_map_insert_unsigned_integer_entry(map, "client-count", clients);
        if (insert_status != BT_VALUE_MAP_INSERT_ENTRY_STATUS_OK) {
            BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                         "Error inserting \"client-count\" entry.");
            ret = -1;
            goto end;
        }
    }

    append_status = bt_value_array_append_element(results, map);
    if (append_status != BT_VALUE_ARRAY_APPEND_ELEMENT_STATUS_OK) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Error appending map to results.");
        ret = -1;
    }

end:
    BT_VALUE_PUT_REF_AND_RESET(map);
    return ret;
}

/*
 * Data structure returned:
 *
 * {
 *   <array> = {
 *     [n] = {
 *       <map> = {
 *         {
 *           key = "url",
 *           value = <string>,
 *         },
 *         {
 *           key = "target-hostname",
 *           value = <string>,
 *         },
 *         {
 *           key = "session-name",
 *           value = <string>,
 *         },
 *         {
 *           key = "timer-us",
 *           value = <integer>,
 *         },
 *         {
 *           key = "stream-count",
 *           value = <integer>,
 *         },
 *         {
 *           key = "client-count",
 *           value = <integer>,
 *         },
 *       },
 *     }
 *   }
 */

bt_component_class_query_method_status
live_viewer_connection_list_sessions(struct live_viewer_connection *viewer_connection,
                                     const bt_value **user_result)
{
    bt_component_class_query_method_status status = BT_COMPONENT_CLASS_QUERY_METHOD_STATUS_OK;
    bt_value *result = NULL;
    enum lttng_live_viewer_status viewer_status;
    struct lttng_viewer_cmd cmd;
    struct lttng_viewer_list_sessions list;
    uint32_t i, sessions_count;

    result = bt_value_array_create();
    if (!result) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Error creating array");
        status = BT_COMPONENT_CLASS_QUERY_METHOD_STATUS_MEMORY_ERROR;
        goto error;
    }

    BT_CPPLOGD_SPEC(viewer_connection->logger, "Requesting list of sessions: cmd={}",
                    LTTNG_VIEWER_LIST_SESSIONS);

    cmd.cmd = htobe32(LTTNG_VIEWER_LIST_SESSIONS);
    cmd.data_size = htobe64((uint64_t) 0);
    cmd.cmd_version = htobe32(0);

    viewer_status = lttng_live_send(viewer_connection, &cmd, sizeof(cmd));
    if (viewer_status == LTTNG_LIVE_VIEWER_STATUS_ERROR) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Error sending list sessions command");
        status = BT_COMPONENT_CLASS_QUERY_METHOD_STATUS_ERROR;
        goto error;
    } else if (viewer_status == LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED) {
        status = BT_COMPONENT_CLASS_QUERY_METHOD_STATUS_AGAIN;
        goto error;
    }

    viewer_status = lttng_live_recv(viewer_connection, &list, sizeof(list));
    if (viewer_status == LTTNG_LIVE_VIEWER_STATUS_ERROR) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Error receiving session list");
        status = BT_COMPONENT_CLASS_QUERY_METHOD_STATUS_ERROR;
        goto error;
    } else if (viewer_status == LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED) {
        status = BT_COMPONENT_CLASS_QUERY_METHOD_STATUS_AGAIN;
        goto error;
    }

    sessions_count = be32toh(list.sessions_count);
    for (i = 0; i < sessions_count; i++) {
        struct lttng_viewer_session lsession;

        viewer_status = lttng_live_recv(viewer_connection, &lsession, sizeof(lsession));
        if (viewer_status == LTTNG_LIVE_VIEWER_STATUS_ERROR) {
            BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Error receiving session:");
            status = BT_COMPONENT_CLASS_QUERY_METHOD_STATUS_ERROR;
            goto error;
        } else if (viewer_status == LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED) {
            status = BT_COMPONENT_CLASS_QUERY_METHOD_STATUS_AGAIN;
            goto error;
        }

        lsession.hostname[LTTNG_VIEWER_HOST_NAME_MAX - 1] = '\0';
        lsession.session_name[LTTNG_VIEWER_NAME_MAX - 1] = '\0';
        if (list_append_session(result, viewer_connection->url, &lsession, viewer_connection)) {
            BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Error appending session");
            status = BT_COMPONENT_CLASS_QUERY_METHOD_STATUS_ERROR;
            goto error;
        }
    }

    *user_result = result;
    goto end;
error:
    BT_VALUE_PUT_REF_AND_RESET(result);
end:
    return status;
}

static enum lttng_live_viewer_status
lttng_live_query_session_ids(struct lttng_live_msg_iter *lttng_live_msg_iter)
{
    struct lttng_viewer_cmd cmd;
    struct lttng_viewer_list_sessions list;
    struct lttng_viewer_session lsession;
    uint32_t i, sessions_count;
    uint64_t session_id;
    enum lttng_live_viewer_status status;
    struct live_viewer_connection *viewer_connection = lttng_live_msg_iter->viewer_connection;

    BT_CPPLOGD_SPEC(viewer_connection->logger,
                    "Asking the relay daemon for the list of sessions: cmd={}",
                    LTTNG_VIEWER_LIST_SESSIONS);

    cmd.cmd = htobe32(LTTNG_VIEWER_LIST_SESSIONS);
    cmd.data_size = htobe64((uint64_t) 0);
    cmd.cmd_version = htobe32(0);

    status = lttng_live_send(viewer_connection, &cmd, sizeof(cmd));
    if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_send_status(status, "list sessions command");
        goto end;
    }

    status = lttng_live_recv(viewer_connection, &list, sizeof(list));
    if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(status, "session list reply");
        goto end;
    }

    sessions_count = be32toh(list.sessions_count);
    for (i = 0; i < sessions_count; i++) {
        status = lttng_live_recv(viewer_connection, &lsession, sizeof(lsession));
        if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
            viewer_handle_recv_status(status, "session reply");
            goto end;
        }
        lsession.hostname[LTTNG_VIEWER_HOST_NAME_MAX - 1] = '\0';
        lsession.session_name[LTTNG_VIEWER_NAME_MAX - 1] = '\0';
        session_id = be64toh(lsession.id);

        BT_CPPLOGI_SPEC(viewer_connection->logger,
                        "Adding session to internal list: "
                        "session-id={}, hostname=\"{}\", session-name=\"{}\"",
                        session_id, lsession.hostname, lsession.session_name);

        if ((strncmp(lsession.session_name, viewer_connection->session_name->str,
                     LTTNG_VIEWER_NAME_MAX) == 0) &&
            (strncmp(lsession.hostname, viewer_connection->target_hostname->str,
                     LTTNG_VIEWER_HOST_NAME_MAX) == 0)) {
            if (lttng_live_add_session(lttng_live_msg_iter, session_id, lsession.hostname,
                                       lsession.session_name)) {
                BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                             "Failed to add live session");
                status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
                goto end;
            }
        }
    }

    status = LTTNG_LIVE_VIEWER_STATUS_OK;

end:
    return status;
}

enum lttng_live_viewer_status
lttng_live_create_viewer_session(struct lttng_live_msg_iter *lttng_live_msg_iter)
{
    struct lttng_viewer_cmd cmd;
    struct lttng_viewer_create_session_response resp;
    enum lttng_live_viewer_status status;
    struct live_viewer_connection *viewer_connection = lttng_live_msg_iter->viewer_connection;

    BT_CPPLOGD_SPEC(viewer_connection->logger, "Creating a viewer session: cmd={}",
                    LTTNG_VIEWER_CREATE_SESSION);

    cmd.cmd = htobe32(LTTNG_VIEWER_CREATE_SESSION);
    cmd.data_size = htobe64((uint64_t) 0);
    cmd.cmd_version = htobe32(0);

    status = lttng_live_send(viewer_connection, &cmd, sizeof(cmd));
    if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_send_status(status, "create session command");
        goto end;
    }

    status = lttng_live_recv(viewer_connection, &resp, sizeof(resp));
    if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(status, "create session reply");
        goto end;
    }

    if (be32toh(resp.status) != LTTNG_VIEWER_CREATE_SESSION_OK) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Error creating viewer session");
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto end;
    }

    status = lttng_live_query_session_ids(lttng_live_msg_iter);
    if (status == LTTNG_LIVE_VIEWER_STATUS_ERROR) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Failed to query live viewer session ids");
        goto end;
    } else if (status == LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED) {
        goto end;
    }

end:
    return status;
}

static enum lttng_live_viewer_status receive_streams(struct lttng_live_session *session,
                                                     uint32_t stream_count,
                                                     bt_self_message_iterator *self_msg_iter)
{
    uint32_t i;
    struct lttng_live_msg_iter *lttng_live_msg_iter = session->lttng_live_msg_iter;
    enum lttng_live_viewer_status status;
    struct live_viewer_connection *viewer_connection = lttng_live_msg_iter->viewer_connection;

    BT_CPPLOGI_SPEC(viewer_connection->logger, "Getting {} new streams", stream_count);
    for (i = 0; i < stream_count; i++) {
        struct lttng_viewer_stream stream;
        struct lttng_live_stream_iterator *live_stream;
        uint64_t stream_id;
        uint64_t ctf_trace_id;

        status = lttng_live_recv(viewer_connection, &stream, sizeof(stream));
        if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
            viewer_handle_recv_status(status, "stream reply");
            goto end;
        }
        stream.path_name[LTTNG_VIEWER_PATH_MAX - 1] = '\0';
        stream.channel_name[LTTNG_VIEWER_NAME_MAX - 1] = '\0';
        stream_id = be64toh(stream.id);
        ctf_trace_id = be64toh(stream.ctf_trace_id);

        if (stream.metadata_flag) {
            BT_CPPLOGI_SPEC(viewer_connection->logger, "    metadata stream {} : {}/{}", stream_id,
                            stream.path_name, stream.channel_name);
            if (lttng_live_metadata_create_stream(session, ctf_trace_id, stream_id)) {
                BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                             "Error creating metadata stream");
                status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
                goto end;
            }
            session->lazy_stream_msg_init = true;
        } else {
            BT_CPPLOGI_SPEC(viewer_connection->logger, "    stream {} : {}/{}", stream_id,
                            stream.path_name, stream.channel_name);
            live_stream =
                lttng_live_stream_iterator_create(session, ctf_trace_id, stream_id, self_msg_iter);
            if (!live_stream) {
                BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Error creating stream");
                status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
                goto end;
            }
        }
    }
    status = LTTNG_LIVE_VIEWER_STATUS_OK;

end:
    return status;
}

enum lttng_live_viewer_status lttng_live_session_attach(struct lttng_live_session *session,
                                                        bt_self_message_iterator *self_msg_iter)
{
    struct lttng_viewer_cmd cmd;
    enum lttng_live_viewer_status status;
    struct lttng_viewer_attach_session_request rq;
    struct lttng_viewer_attach_session_response rp;
    struct lttng_live_msg_iter *lttng_live_msg_iter = session->lttng_live_msg_iter;
    struct live_viewer_connection *viewer_connection = lttng_live_msg_iter->viewer_connection;
    uint64_t session_id = session->id;
    uint32_t streams_count;
    const size_t cmd_buf_len = sizeof(cmd) + sizeof(rq);
    char cmd_buf[cmd_buf_len];

    BT_CPPLOGD_SPEC(viewer_connection->logger,
                    "Attaching to session: cmd={}, session-id={}, seek={}",
                    LTTNG_VIEWER_ATTACH_SESSION, session_id, LTTNG_VIEWER_SEEK_LAST);

    cmd.cmd = htobe32(LTTNG_VIEWER_ATTACH_SESSION);
    cmd.data_size = htobe64((uint64_t) sizeof(rq));
    cmd.cmd_version = htobe32(0);

    memset(&rq, 0, sizeof(rq));
    rq.session_id = htobe64(session_id);
    // TODO: add cmd line parameter to select seek beginning
    // rq.seek = htobe32(LTTNG_VIEWER_SEEK_BEGINNING);
    rq.seek = htobe32(LTTNG_VIEWER_SEEK_LAST);

    /*
     * Merge the cmd and connection request to prevent a write-write
     * sequence on the TCP socket. Otherwise, a delayed ACK will prevent the
     * second write to be performed quickly in presence of Nagle's algorithm.
     */
    memcpy(cmd_buf, &cmd, sizeof(cmd));
    memcpy(cmd_buf + sizeof(cmd), &rq, sizeof(rq));
    status = lttng_live_send(viewer_connection, &cmd_buf, cmd_buf_len);
    if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_send_status(status, "attach session command");
        goto end;
    }

    status = lttng_live_recv(viewer_connection, &rp, sizeof(rp));
    if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(status, "attach session reply");
        goto end;
    }

    streams_count = be32toh(rp.streams_count);
    switch (be32toh(rp.status)) {
    case LTTNG_VIEWER_ATTACH_OK:
        break;
    case LTTNG_VIEWER_ATTACH_UNK:
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Session id {} is unknown",
                                     session_id);
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto end;
    case LTTNG_VIEWER_ATTACH_ALREADY:
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "There is already a viewer attached to this session");
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto end;
    case LTTNG_VIEWER_ATTACH_NOT_LIVE:
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Not a live session");
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto end;
    case LTTNG_VIEWER_ATTACH_SEEK_ERR:
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Wrong seek parameter");
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto end;
    default:
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Unknown attach return code {}",
                                     be32toh(rp.status));
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto end;
    }

    /* We receive the initial list of streams. */
    status = receive_streams(session, streams_count, self_msg_iter);
    switch (status) {
    case LTTNG_LIVE_VIEWER_STATUS_OK:
        break;
    case LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED:
        goto end;
    case LTTNG_LIVE_VIEWER_STATUS_ERROR:
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Error receiving streams");
        goto end;
    default:
        bt_common_abort();
    }

    session->attached = true;
    session->new_streams_needed = false;

end:
    return status;
}

enum lttng_live_viewer_status lttng_live_session_detach(struct lttng_live_session *session)
{
    struct lttng_viewer_cmd cmd;
    enum lttng_live_viewer_status status;
    struct lttng_viewer_detach_session_request rq;
    struct lttng_viewer_detach_session_response rp;
    struct lttng_live_msg_iter *lttng_live_msg_iter = session->lttng_live_msg_iter;
    struct live_viewer_connection *viewer_connection = lttng_live_msg_iter->viewer_connection;
    uint64_t session_id = session->id;
    const size_t cmd_buf_len = sizeof(cmd) + sizeof(rq);
    char cmd_buf[cmd_buf_len];

    /*
     * The session might already be detached and the viewer socket might
     * already been closed. This happens when calling this function when
     * tearing down the graph after an error.
     */
    if (!session->attached || viewer_connection->control_sock == BT_INVALID_SOCKET) {
        return LTTNG_LIVE_VIEWER_STATUS_OK;
    }

    BT_CPPLOGD_SPEC(viewer_connection->logger, "Detaching from session: cmd={}, session-id={}",
                    LTTNG_VIEWER_DETACH_SESSION, session_id);

    cmd.cmd = htobe32(LTTNG_VIEWER_DETACH_SESSION);
    cmd.data_size = htobe64((uint64_t) sizeof(rq));
    cmd.cmd_version = htobe32(0);

    memset(&rq, 0, sizeof(rq));
    rq.session_id = htobe64(session_id);

    /*
     * Merge the cmd and connection request to prevent a write-write
     * sequence on the TCP socket. Otherwise, a delayed ACK will prevent the
     * second write to be performed quickly in presence of Nagle's algorithm.
     */
    memcpy(cmd_buf, &cmd, sizeof(cmd));
    memcpy(cmd_buf + sizeof(cmd), &rq, sizeof(rq));
    status = lttng_live_send(viewer_connection, &cmd_buf, cmd_buf_len);
    if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_send_status(status, "detach session command");
        goto end;
    }

    status = lttng_live_recv(viewer_connection, &rp, sizeof(rp));
    if (status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(status, "detach session reply");
        goto end;
    }

    switch (be32toh(rp.status)) {
    case LTTNG_VIEWER_DETACH_SESSION_OK:
        break;
    case LTTNG_VIEWER_DETACH_SESSION_UNK:
        BT_CPPLOGW_SPEC(viewer_connection->logger, "Session id {} is unknown", session_id);
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto end;
    case LTTNG_VIEWER_DETACH_SESSION_ERR:
        BT_CPPLOGW_SPEC(viewer_connection->logger, "Error detaching session id {}", session_id);
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto end;
    default:
        BT_CPPLOGE_SPEC(viewer_connection->logger, "Unknown detach return code {}",
                        be32toh(rp.status));
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto end;
    }

    session->attached = false;

    status = LTTNG_LIVE_VIEWER_STATUS_OK;

end:
    return status;
}

enum lttng_live_get_one_metadata_status
lttng_live_get_one_metadata_packet(struct lttng_live_trace *trace, std::vector<char>& buf)
{
    uint64_t len = 0;
    enum lttng_live_get_one_metadata_status status;
    enum lttng_live_viewer_status viewer_status;
    struct lttng_viewer_cmd cmd;
    struct lttng_viewer_get_metadata rq;
    struct lttng_viewer_metadata_packet rp;
    std::vector<char> data;
    struct lttng_live_session *session = trace->session;
    struct lttng_live_msg_iter *lttng_live_msg_iter = session->lttng_live_msg_iter;
    struct lttng_live_metadata *metadata = trace->metadata.get();
    struct live_viewer_connection *viewer_connection = lttng_live_msg_iter->viewer_connection;
    const size_t cmd_buf_len = sizeof(cmd) + sizeof(rq);
    char cmd_buf[cmd_buf_len];

    BT_CPPLOGD_SPEC(viewer_connection->logger,
                    "Requesting new metadata for trace:"
                    "cmd={}, trace-id={}, metadata-stream-id={}",
                    LTTNG_VIEWER_GET_METADATA, trace->id, metadata->stream_id);

    rq.stream_id = htobe64(metadata->stream_id);
    cmd.cmd = htobe32(LTTNG_VIEWER_GET_METADATA);
    cmd.data_size = htobe64((uint64_t) sizeof(rq));
    cmd.cmd_version = htobe32(0);

    /*
     * Merge the cmd and connection request to prevent a write-write
     * sequence on the TCP socket. Otherwise, a delayed ACK will prevent the
     * second write to be performed quickly in presence of Nagle's algorithm.
     */
    memcpy(cmd_buf, &cmd, sizeof(cmd));
    memcpy(cmd_buf + sizeof(cmd), &rq, sizeof(rq));
    viewer_status = lttng_live_send(viewer_connection, &cmd_buf, cmd_buf_len);
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_send_status(viewer_status, "get metadata command");
        status = (enum lttng_live_get_one_metadata_status) viewer_status;
        goto end;
    }

    viewer_status = lttng_live_recv(viewer_connection, &rp, sizeof(rp));
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(viewer_status, "get metadata reply");
        status = (enum lttng_live_get_one_metadata_status) viewer_status;
        goto end;
    }

    switch (be32toh(rp.status)) {
    case LTTNG_VIEWER_METADATA_OK:
        BT_CPPLOGD_SPEC(viewer_connection->logger, "Received get_metadata response: ok");
        break;
    case LTTNG_VIEWER_NO_NEW_METADATA:
        BT_CPPLOGD_SPEC(viewer_connection->logger, "Received get_metadata response: no new");
        status = LTTNG_LIVE_GET_ONE_METADATA_STATUS_END;
        goto end;
    case LTTNG_VIEWER_METADATA_ERR:
        /*
             * The Relayd cannot find this stream id. Maybe its
             * gone already. This can happen in short lived UST app
             * in a per-pid session.
             */
        BT_CPPLOGD_SPEC(viewer_connection->logger, "Received get_metadata response: error");
        status = LTTNG_LIVE_GET_ONE_METADATA_STATUS_CLOSED;
        goto end;
    default:
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Received get_metadata response: unknown");
        status = LTTNG_LIVE_GET_ONE_METADATA_STATUS_ERROR;
        goto end;
    }

    len = be64toh(rp.len);
    if (len == 0) {
        /*
         * We received a `LTTNG_VIEWER_METADATA_OK` with a packet
         * length of 0. This means we must try again. This scenario
         * arises when a clear command is performed on an lttng session.
         */
        BT_CPPLOGD_SPEC(
            viewer_connection->logger,
            "Expecting a metadata packet of size 0. Retry to get a packet from the relay.");
        goto empty_metadata_packet_retry;
    }

    BT_CPPLOGD_SPEC(viewer_connection->logger, "Writing {} bytes to metadata", len);
    if (len <= 0) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Erroneous response length");
        status = LTTNG_LIVE_GET_ONE_METADATA_STATUS_ERROR;
        goto end;
    }

    data.resize(len);

    viewer_status = lttng_live_recv(viewer_connection, data.data(), len);
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(viewer_status, "get metadata packet");
        status = (enum lttng_live_get_one_metadata_status) viewer_status;
        goto end;
    }

    /*
     * Write the metadata to the file handle.
     */
    buf.insert(buf.end(), data.begin(), data.end());

empty_metadata_packet_retry:
    status = LTTNG_LIVE_GET_ONE_METADATA_STATUS_OK;

end:
    return status;
}

/*
 * Assign the fields from a lttng_viewer_index to a packet_index.
 */
static void lttng_index_to_packet_index(struct lttng_viewer_index *lindex,
                                        struct packet_index *pindex)
{
    BT_ASSERT(lindex);
    BT_ASSERT(pindex);

    pindex->offset = be64toh(lindex->offset);
    pindex->packet_size = be64toh(lindex->packet_size);
    pindex->content_size = be64toh(lindex->content_size);
    pindex->ts_cycles.timestamp_begin = be64toh(lindex->timestamp_begin);
    pindex->ts_cycles.timestamp_end = be64toh(lindex->timestamp_end);
    pindex->events_discarded = be64toh(lindex->events_discarded);
}

static void lttng_live_need_new_streams(struct lttng_live_msg_iter *lttng_live_msg_iter)
{
    for (const auto& session : lttng_live_msg_iter->sessions) {
        BT_CPPLOGD_SPEC(lttng_live_msg_iter->logger,
                        "Marking session as needing new streams: "
                        "session-id={}",
                        session->id);
        session->new_streams_needed = true;
    }
}

enum lttng_live_iterator_status
lttng_live_get_next_index(struct lttng_live_msg_iter *lttng_live_msg_iter,
                          struct lttng_live_stream_iterator *stream, struct packet_index *index)
{
    struct lttng_viewer_cmd cmd;
    struct lttng_viewer_get_next_index rq;
    enum lttng_live_viewer_status viewer_status;
    struct lttng_viewer_index rp;
    enum lttng_live_iterator_status status;
    struct live_viewer_connection *viewer_connection = lttng_live_msg_iter->viewer_connection;
    struct lttng_live_trace *trace = stream->trace;
    const size_t cmd_buf_len = sizeof(cmd) + sizeof(rq);
    char cmd_buf[cmd_buf_len];
    uint32_t flags, rp_status;

    BT_CPPLOGD_SPEC(viewer_connection->logger,
                    "Requesting next index for stream: cmd={}, "
                    "viewer-stream-id={}",
                    LTTNG_VIEWER_GET_NEXT_INDEX, stream->viewer_stream_id);
    cmd.cmd = htobe32(LTTNG_VIEWER_GET_NEXT_INDEX);
    cmd.data_size = htobe64((uint64_t) sizeof(rq));
    cmd.cmd_version = htobe32(0);

    memset(&rq, 0, sizeof(rq));
    rq.stream_id = htobe64(stream->viewer_stream_id);

    /*
     * Merge the cmd and connection request to prevent a write-write
     * sequence on the TCP socket. Otherwise, a delayed ACK will prevent the
     * second write to be performed quickly in presence of Nagle's algorithm.
     */
    memcpy(cmd_buf, &cmd, sizeof(cmd));
    memcpy(cmd_buf + sizeof(cmd), &rq, sizeof(rq));

    viewer_status = lttng_live_send(viewer_connection, &cmd_buf, cmd_buf_len);
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_send_status(viewer_status, "get next index command");
        goto error;
    }

    viewer_status = lttng_live_recv(viewer_connection, &rp, sizeof(rp));
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(viewer_status, "get next index reply");
        goto error;
    }

    flags = be32toh(rp.flags);
    rp_status = be32toh(rp.status);

    BT_CPPLOGD_SPEC(
        viewer_connection->logger, "Received response from relay daemon: cmd=%s, response={}",
        LTTNG_VIEWER_GET_NEXT_INDEX, static_cast<lttng_viewer_next_index_return_code>(rp_status));

    if (flags & LTTNG_VIEWER_FLAG_NEW_STREAM) {
        BT_CPPLOGD_SPEC(viewer_connection->logger,
                        "Marking all sessions as possibly needing new streams: "
                        "response={}, response-flag=NEW_STREAM",
                        static_cast<lttng_viewer_next_index_return_code>(rp_status));
        lttng_live_need_new_streams(lttng_live_msg_iter);
    }

    switch (rp_status) {
    case LTTNG_VIEWER_INDEX_INACTIVE:
    {
        uint64_t ctf_stream_class_id;

        memset(index, 0, sizeof(struct packet_index));
        index->ts_cycles.timestamp_end = be64toh(rp.timestamp_end);
        stream->current_inactivity_ts = index->ts_cycles.timestamp_end;
        ctf_stream_class_id = be64toh(rp.stream_id);
        if (stream->ctf_stream_class_id.is_set) {
            BT_ASSERT(stream->ctf_stream_class_id.value == ctf_stream_class_id);
        } else {
            stream->ctf_stream_class_id.value = ctf_stream_class_id;
            stream->ctf_stream_class_id.is_set = true;
        }
        lttng_live_stream_iterator_set_state(stream, LTTNG_LIVE_STREAM_QUIESCENT);
        status = LTTNG_LIVE_ITERATOR_STATUS_OK;
        break;
    }
    case LTTNG_VIEWER_INDEX_OK:
    {
        uint64_t ctf_stream_class_id;

        lttng_index_to_packet_index(&rp, index);
        ctf_stream_class_id = be64toh(rp.stream_id);
        if (stream->ctf_stream_class_id.is_set) {
            BT_ASSERT(stream->ctf_stream_class_id.value == ctf_stream_class_id);
        } else {
            stream->ctf_stream_class_id.value = ctf_stream_class_id;
            stream->ctf_stream_class_id.is_set = true;
        }
        lttng_live_stream_iterator_set_state(stream, LTTNG_LIVE_STREAM_ACTIVE_DATA);

        if (flags & LTTNG_VIEWER_FLAG_NEW_METADATA) {
            BT_CPPLOGD_SPEC(viewer_connection->logger,
                            "Marking trace as needing new metadata: "
                            "response={}, response-flag=NEW_METADATA, trace-id={}",
                            static_cast<lttng_viewer_next_index_return_code>(rp_status), trace->id);
            trace->metadata_stream_state = LTTNG_LIVE_METADATA_STREAM_STATE_NEEDED;
        }
        status = LTTNG_LIVE_ITERATOR_STATUS_OK;
        break;
    }
    case LTTNG_VIEWER_INDEX_RETRY:
        memset(index, 0, sizeof(struct packet_index));
        lttng_live_stream_iterator_set_state(stream, LTTNG_LIVE_STREAM_ACTIVE_NO_DATA);
        status = LTTNG_LIVE_ITERATOR_STATUS_AGAIN;
        goto end;
    case LTTNG_VIEWER_INDEX_HUP:
        memset(index, 0, sizeof(struct packet_index));
        index->offset = EOF;
        lttng_live_stream_iterator_set_state(stream, LTTNG_LIVE_STREAM_EOF);
        stream->has_stream_hung_up = true;
        status = LTTNG_LIVE_ITERATOR_STATUS_END;
        break;
    case LTTNG_VIEWER_INDEX_ERR:
        memset(index, 0, sizeof(struct packet_index));
        lttng_live_stream_iterator_set_state(stream, LTTNG_LIVE_STREAM_ACTIVE_NO_DATA);
        status = LTTNG_LIVE_ITERATOR_STATUS_ERROR;
        goto end;
    default:
        BT_CPPLOGD_SPEC(viewer_connection->logger,
                        "Received get_next_index response: unknown value");
        memset(index, 0, sizeof(struct packet_index));
        lttng_live_stream_iterator_set_state(stream, LTTNG_LIVE_STREAM_ACTIVE_NO_DATA);
        status = LTTNG_LIVE_ITERATOR_STATUS_ERROR;
        goto end;
    }

    goto end;

error:
    status = viewer_status_to_live_iterator_status(viewer_status);
end:
    return status;
}

enum ctf_msg_iter_medium_status
lttng_live_get_stream_bytes(struct lttng_live_msg_iter *lttng_live_msg_iter,
                            struct lttng_live_stream_iterator *stream, uint8_t *buf,
                            uint64_t offset, uint64_t req_len, uint64_t *recv_len)
{
    enum ctf_msg_iter_medium_status status;
    enum lttng_live_viewer_status viewer_status;
    struct lttng_viewer_trace_packet rp;
    struct lttng_viewer_cmd cmd;
    struct lttng_viewer_get_packet rq;
    struct live_viewer_connection *viewer_connection = lttng_live_msg_iter->viewer_connection;
    struct lttng_live_trace *trace = stream->trace;
    const size_t cmd_buf_len = sizeof(cmd) + sizeof(rq);
    char cmd_buf[cmd_buf_len];
    uint32_t flags, rp_status;

    BT_CPPLOGD_SPEC(viewer_connection->logger,
                    "Requesting data from stream: cmd={}, "
                    "offset={}, request-len={}",
                    LTTNG_VIEWER_GET_PACKET, offset, req_len);

    cmd.cmd = htobe32(LTTNG_VIEWER_GET_PACKET);
    cmd.data_size = htobe64((uint64_t) sizeof(rq));
    cmd.cmd_version = htobe32(0);

    memset(&rq, 0, sizeof(rq));
    rq.stream_id = htobe64(stream->viewer_stream_id);
    rq.offset = htobe64(offset);
    rq.len = htobe32(req_len);

    /*
     * Merge the cmd and connection request to prevent a write-write
     * sequence on the TCP socket. Otherwise, a delayed ACK will prevent the
     * second write to be performed quickly in presence of Nagle's algorithm.
     */
    memcpy(cmd_buf, &cmd, sizeof(cmd));
    memcpy(cmd_buf + sizeof(cmd), &rq, sizeof(rq));

    viewer_status = lttng_live_send(viewer_connection, &cmd_buf, cmd_buf_len);
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_send_status(viewer_status, "get data packet command");
        goto error_convert_status;
    }

    viewer_status = lttng_live_recv(viewer_connection, &rp, sizeof(rp));
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(viewer_status, "get data packet reply");
        goto error_convert_status;
    }

    flags = be32toh(rp.flags);
    rp_status = be32toh(rp.status);

    BT_CPPLOGD_SPEC(
        viewer_connection->logger, "Received response from relay daemon: cmd={}, response={}",
        LTTNG_VIEWER_GET_PACKET, static_cast<lttng_viewer_get_packet_return_code>(rp_status));
    switch (rp_status) {
    case LTTNG_VIEWER_GET_PACKET_OK:
        req_len = be32toh(rp.len);
        BT_CPPLOGD_SPEC(viewer_connection->logger,
                        "Got packet from relay daemon: response={}, packet-len={}",
                        static_cast<lttng_viewer_get_packet_return_code>(rp_status), req_len);
        break;
    case LTTNG_VIEWER_GET_PACKET_RETRY:
        /* Unimplemented by relay daemon */
        status = CTF_MSG_ITER_MEDIUM_STATUS_AGAIN;
        goto end;
    case LTTNG_VIEWER_GET_PACKET_ERR:
        if (flags & LTTNG_VIEWER_FLAG_NEW_METADATA) {
            BT_CPPLOGD_SPEC(viewer_connection->logger,
                            "Marking trace as needing new metadata: "
                            "response={}, response-flag=NEW_METADATA, trace-id={}",
                            static_cast<lttng_viewer_get_packet_return_code>(rp_status), trace->id);
            trace->metadata_stream_state = LTTNG_LIVE_METADATA_STREAM_STATE_NEEDED;
        }
        if (flags & LTTNG_VIEWER_FLAG_NEW_STREAM) {
            BT_CPPLOGD_SPEC(viewer_connection->logger,
                            "Marking all sessions as possibly needing new streams: "
                            "response={}, response-flag=NEW_STREAM",
                            static_cast<lttng_viewer_get_packet_return_code>(rp_status));
            lttng_live_need_new_streams(lttng_live_msg_iter);
        }
        if (flags & (LTTNG_VIEWER_FLAG_NEW_METADATA | LTTNG_VIEWER_FLAG_NEW_STREAM)) {
            status = CTF_MSG_ITER_MEDIUM_STATUS_AGAIN;
            BT_CPPLOGD_SPEC(viewer_connection->logger,
                            "Reply with any one flags set means we should retry: response={}",
                            static_cast<lttng_viewer_get_packet_return_code>(rp_status));
            goto end;
        }
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Received get_data_packet response: error");
        status = CTF_MSG_ITER_MEDIUM_STATUS_ERROR;
        goto end;
    case LTTNG_VIEWER_GET_PACKET_EOF:
        status = CTF_MSG_ITER_MEDIUM_STATUS_EOF;
        goto end;
    default:
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Received get_data_packet response: unknown ({})", rp_status);
        status = CTF_MSG_ITER_MEDIUM_STATUS_ERROR;
        goto end;
    }

    if (req_len == 0) {
        status = CTF_MSG_ITER_MEDIUM_STATUS_ERROR;
        goto end;
    }

    viewer_status = lttng_live_recv(viewer_connection, buf, req_len);
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(viewer_status, "get data packet");
        goto error_convert_status;
    }
    *recv_len = req_len;

    status = CTF_MSG_ITER_MEDIUM_STATUS_OK;
    goto end;

error_convert_status:
    status = viewer_status_to_ctf_msg_iter_medium_status(viewer_status);
end:
    return status;
}

/*
 * Request new streams for a session.
 */
enum lttng_live_iterator_status
lttng_live_session_get_new_streams(struct lttng_live_session *session,
                                   bt_self_message_iterator *self_msg_iter)
{
    enum lttng_live_iterator_status status = LTTNG_LIVE_ITERATOR_STATUS_OK;
    struct lttng_viewer_cmd cmd;
    struct lttng_viewer_new_streams_request rq;
    struct lttng_viewer_new_streams_response rp;
    struct lttng_live_msg_iter *lttng_live_msg_iter = session->lttng_live_msg_iter;
    enum lttng_live_viewer_status viewer_status;
    struct live_viewer_connection *viewer_connection = lttng_live_msg_iter->viewer_connection;
    uint32_t streams_count;
    const size_t cmd_buf_len = sizeof(cmd) + sizeof(rq);
    char cmd_buf[cmd_buf_len];

    if (!session->new_streams_needed) {
        status = LTTNG_LIVE_ITERATOR_STATUS_OK;
        goto end;
    }

    BT_CPPLOGD_SPEC(viewer_connection->logger,
                    "Requesting new streams for session: cmd={}, session-id={}",
                    LTTNG_VIEWER_GET_NEW_STREAMS, session->id);

    cmd.cmd = htobe32(LTTNG_VIEWER_GET_NEW_STREAMS);
    cmd.data_size = htobe64((uint64_t) sizeof(rq));
    cmd.cmd_version = htobe32(0);

    memset(&rq, 0, sizeof(rq));
    rq.session_id = htobe64(session->id);

    /*
     * Merge the cmd and connection request to prevent a write-write
     * sequence on the TCP socket. Otherwise, a delayed ACK will prevent the
     * second write to be performed quickly in presence of Nagle's algorithm.
     */
    memcpy(cmd_buf, &cmd, sizeof(cmd));
    memcpy(cmd_buf + sizeof(cmd), &rq, sizeof(rq));

    viewer_status = lttng_live_send(viewer_connection, &cmd_buf, cmd_buf_len);
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_send_status(viewer_status, "get new streams command");
        status = viewer_status_to_live_iterator_status(viewer_status);
        goto end;
    }

    viewer_status = lttng_live_recv(viewer_connection, &rp, sizeof(rp));
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(viewer_status, "get new streams reply");
        status = viewer_status_to_live_iterator_status(viewer_status);
        goto end;
    }

    streams_count = be32toh(rp.streams_count);

    switch (be32toh(rp.status)) {
    case LTTNG_VIEWER_NEW_STREAMS_OK:
        session->new_streams_needed = false;
        break;
    case LTTNG_VIEWER_NEW_STREAMS_NO_NEW:
        session->new_streams_needed = false;
        goto end;
    case LTTNG_VIEWER_NEW_STREAMS_HUP:
        session->new_streams_needed = false;
        session->closed = true;
        status = LTTNG_LIVE_ITERATOR_STATUS_END;
        goto end;
    case LTTNG_VIEWER_NEW_STREAMS_ERR:
        BT_CPPLOGD_SPEC(viewer_connection->logger, "Received get_new_streams response: error");
        status = LTTNG_LIVE_ITERATOR_STATUS_ERROR;
        goto end;
    default:
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Received get_new_streams response: Unknown:"
                                     "return code {}",
                                     be32toh(rp.status));
        status = LTTNG_LIVE_ITERATOR_STATUS_ERROR;
        goto end;
    }

    viewer_status = receive_streams(session, streams_count, self_msg_iter);
    if (viewer_status != LTTNG_LIVE_VIEWER_STATUS_OK) {
        viewer_handle_recv_status(viewer_status, "new streams");
        status = viewer_status_to_live_iterator_status(viewer_status);
        goto end;
    }

    status = LTTNG_LIVE_ITERATOR_STATUS_OK;
end:
    return status;
}

enum lttng_live_viewer_status live_viewer_connection_create(
    const char *url, bool in_query, struct lttng_live_msg_iter *lttng_live_msg_iter,
    const bt2c::Logger& parentLogger, struct live_viewer_connection **viewer)
{
    enum lttng_live_viewer_status status;

    live_viewer_connection *viewer_connection = new live_viewer_connection {parentLogger};

    if (bt_socket_init(viewer_connection->logger) != 0) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger, "Failed to init socket");
        status = LTTNG_LIVE_VIEWER_STATUS_ERROR;
        goto error;
    }

    viewer_connection->control_sock = BT_INVALID_SOCKET;
    viewer_connection->port = -1;
    viewer_connection->in_query = in_query;
    viewer_connection->lttng_live_msg_iter = lttng_live_msg_iter;
    viewer_connection->url = url;

    BT_CPPLOGD_SPEC(viewer_connection->logger, "Establishing connection to url \"{}\"...", url);
    status = lttng_live_connect_viewer(viewer_connection);
    /*
     * Only print error and append cause in case of error. not in case of
     * interruption.
     */
    if (status == LTTNG_LIVE_VIEWER_STATUS_ERROR) {
        BT_CPPLOGE_APPEND_CAUSE_SPEC(viewer_connection->logger,
                                     "Failed to establish connection: "
                                     "url=\"{}\"",
                                     url);
        goto error;
    } else if (status == LTTNG_LIVE_VIEWER_STATUS_INTERRUPTED) {
        goto error;
    }
    BT_CPPLOGD_SPEC(viewer_connection->logger, "Connection to url \"{}\" is established", url);

    *viewer = viewer_connection;
    status = LTTNG_LIVE_VIEWER_STATUS_OK;
    goto end;

error:
    if (viewer_connection) {
        live_viewer_connection_destroy(viewer_connection);
    }
end:
    return status;
}

void live_viewer_connection_destroy(struct live_viewer_connection *viewer_connection)
{
    if (!viewer_connection) {
        goto end;
    }

    BT_CPPLOGD_SPEC(viewer_connection->logger, "Closing connection to relay: relay-url=\"{}\"",
                    viewer_connection->url);

    lttng_live_disconnect_viewer(viewer_connection);

    delete viewer_connection;

    bt_socket_fini();

end:
    return;
}
