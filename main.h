#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <xmmsclient/xmmsclient.h>

struct xplaylist_args_t {

	const char *playlist_name; /* -p */
	const char *output_type; /* -t */
	const char *output_filename;
	FILE *out_file;
} xplaylist_args;

static const char *xplaylist_opt_string = "p:t:";

static const struct option xplaylist_long_opts[] =
{
	{"playlist", required_argument, NULL, 'p'},
	{"type", required_argument, NULL, 't'},
	{"list", no_argument, 0, 'l'},
	{"help", no_argument, NULL, 'h'},
	{NULL, no_argument, 0, 0}
};

/* Simple list to append URL's to. */
struct medialib_info_node {
	char *url;
	char *title;
	char *artist;
	int length;
	struct medialib_info_node *next;
};

void usage();

char* get_media_string(xmmsc_connection_t*, int, const char*);
int get_media_int(xmmsc_connection_t*, int, const char*);

bool start_playlist_m3u();
bool add_playlist_m3u(int, const char*, const char*, const char*);
bool end_playlist_m3u();

bool start_playlist_pls(int);
bool add_playlist_pls(int, const char*, const char *);
bool end_playlist_pls();
