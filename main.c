#include "gsoap/stdsoap2.h"
#include "github/argparse.h"
#include "core/se_std.h"
#include "core/se_conf.h"
#include "core/se_service.h"
#if defined(_MSC_VER)
#include <conio.h>
#if defined(_DEBUG )
#include <vld.h>
#endif
#endif

#include "se_handler.h"


bool SE_test_pgconn(const struct SE_CONF *conf) {
	char **keys = NULL, **vals = NULL;
	PGconn *conn = NULL;
	int32_t i = 0;
	//≤‚ ‘ ˝æ›ø‚¡¥Ω”
	for (i = 0; i < conf->dbs->count; ++i) {
		SE_throw(SE_dbconf_conn(conf->name, conf->dbs->item[i], &keys, &vals));
		conn = PQconnectdbParams((const char *const *)keys, (const char *const *)vals, 1);
		if (CONNECTION_OK != PQstatus(conn)) {
			fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
			goto SE_ERROR_CLEAR;
		}
		SE_PQfinish(conn);
		SE_dbconf_conn_free(&keys, &vals);
	}
	return true;
SE_ERROR_CLEAR:
	SE_PQfinish(conn);
	SE_dbconf_conn_free(&keys, &vals);
	return false;
}

#if defined(_MSC_VER )
void ser_run(const struct SE_CONF *const conf) {
	SE_process process = user_process;
	if (SE_start(conf, process)) {
		while (1) {
			int ch = _getch();
			if (27 == ch)
				break;
		}
	}
	SE_stop();
}
#else
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

static bool SE_isrun = true;

void signal_handler(int32_t sig) {
	switch (sig) {
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		SE_isrun = false;
		break;
	}
}
void ser_run(const struct SE_CONF *const conf) {
	void(*prev_handler1)(int32_t), (*prev_handler2)(int32_t), (*prev_handler3)(int32_t);
	SE_process process = user_process;

	prev_handler1 = signal(SIGINT, signal_handler); 
	prev_handler2 = signal(SIGTERM, signal_handler);
	prev_handler3 = signal(SIGQUIT, signal_handler); 
	if (SIG_ERR == prev_handler1 || SIG_ERR == prev_handler2 || SIG_ERR == prev_handler3)
		exit(EXIT_FAILURE);

	if (SE_start(conf, process)) {
		while (SE_isrun) {
			sleep(500);
		}
	}
	SE_stop();
	fprintf(stdout, "exit now\n");
}
#endif

int32_t SE_cmd(struct argparse *self, const struct argparse_option *option) {
	struct SE_CONF *conf = NULL;
	SE_throw(SE_conf_create(&conf));
	SE_throw(SE_test_pgconn(conf));
	ser_run(conf);
	SE_conf_free(&conf);
	return 0;
SE_ERROR_CLEAR:
	SE_conf_free(&conf);
	return 1;
}

static const char *const usage[] = {
	"gsoap server [options]",
	NULL,
};

int main(int argc, char** argv) {
	char *data = NULL;
	struct argparse argparse;
	struct argparse_option options[] = {
		OPT_HELP(),
		OPT_STRING('c', "command", &data, "Command-line mode runs",SE_cmd),
		OPT_END(),
	};
	argparse_init(&argparse, options, usage, 0);
	//argparse_describe(&argparse, "\nA brief description of what the program does and how it works.", "\nAdditional description of the program after the description of the arguments.");
	argc = argparse_parse(&argparse, argc, (const char **)argv);
	if (SE_PTR_ISNULL(data))
		argparse_usage(&argparse);
	exit(EXIT_SUCCESS);
}

struct Namespace namespaces[] = { { NULL, NULL } };