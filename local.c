#include "socket_wrap.h"
#include "socks.h"

static void ss_accept_handle(void *s, int fd, void *data, int mask)
{
	int conn_fd;
	struct conn_info conn_info;
	struct ss_conn_ctx *nc;

	conn_fd = ss_accept(fd, conn_info.ip, &conn_info.port);
	if (conn_fd < 0) {
		debug_print("ss_accetp failed!");
		return;
	}
	nc = ss_server_add_conn(s, conn_fd, AE_READABLE, &conn_info, data);
	if (nc == NULL) {
		debug_print("ss_server_add_conn failed!");
		return;
	}
}

static void echo_func(struct ss_conn_ctx *conn)
{
	int ret;

	debug_print("%s", conn->msg->data);
	ret = ss_send_msg_conn(conn, 0);
	if (ret < 0)
		debug_print("ss_send_msg_conn return %d", ret);
}

static void ss_io_handle(void *conn, int fd, void *data, int mask)
{
	struct ss_conn_ctx *conn_ptr = conn;

	switch (conn_ptr->ss_conn_state) {
	case OPENING:
		ss_handshake_handle(conn_ptr);
		break;
	case CONNECTING:
		ss_msg_handle(conn_ptr, echo_func);
		break;
	default:
		debug_print("unknow state!");
	}
}

int main(int argc, char **argv)
{
	struct ss_server_ctx *lo_s;
	struct io_event s_event;
	struct io_event c_event;

	lo_s = ss_create_server(1080);
	if (lo_s == NULL)
		DIE("ss_create_server failed!");
	memset(&s_event, 0, sizeof(s_event));
	s_event.rfileproc = ss_accept_handle;
	memset(&c_event, 0, sizeof(c_event));
	c_event.rfileproc = ss_io_handle;
	s_event.para = malloc(sizeof(c_event));
	memcpy(s_event.para, &c_event, sizeof(c_event));
	memcpy(&lo_s->io_proc, &s_event, sizeof(s_event));

	ss_loop(lo_s);
	ss_release_server(lo_s);
	return 0;
}
