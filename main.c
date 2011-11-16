/* Author: 	Bradlee Speice
 * Program:	XMMS2 playlist creator
 * Purpose:	Given a playlist name (current playlist if none is given)
		output a file playlist with the URL's of all media in the
		XMMS2 playlist.
		Filetypes supported: M3U, PLS
 */


#include "main.h"

int main(int argc, char **argv)
{

	/* Let's get everything set up.
	 * Parse the command line, and find out
	 * what exactly it is that we're doing.
	 */
	int opt = 0;
	int long_index = 0;
	xplaylist_args.playlist_name = NULL;
	xplaylist_args.output_type = NULL;
	xplaylist_args.output_filename = NULL;
	xplaylist_args.out_file = NULL;

	struct medialib_info_node *head_node, *current_node;
	current_node = head_node = NULL;
	int playlist_length = 0;

	while(opt != -1)
	{
		opt = getopt_long(argc, argv, xplaylist_opt_string, xplaylist_long_opts, &long_index);
		switch(opt)
		{
			case 0:
				fprintf(stdout, "Unknown option %s: %s\n", xplaylist_long_opts[long_index].name, optarg); break;
			case 'p':
				xplaylist_args.playlist_name = optarg;
				break;
			case 't':
				xplaylist_args.output_type = optarg;
				break;
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			case '?':
				usage();
				return(EXIT_FAILURE);
		}
	}

	/* And then we need the output file_name */
	xplaylist_args.output_filename = argv[optind];

	if ((argc - optind) > 1){
		optind++;
		fprintf(stdout, "Extra input: ");
		for (optind; optind < argc; optind++){
			fprintf(stdout, "%s", argv[optind]);
		}
		fprintf(stdout, "\n");
	}

	if (xplaylist_args.output_type == NULL)
		xplaylist_args.output_type = "pls";

	/* End argument parsing */

	/* Connection and basic I/O variables */
	xmmsc_connection_t *connection;
	xmmsc_result_t *result;
	xmmsv_t *return_value;
	const char *err_buf;

	/* Other variables */

	xmmsv_list_iter_t *iterator;
	

	/* Now we start the fun stuff */

	/* Set up the connection */
	connection = xmmsc_init("xplaylist_creator");
	if (!connection) {
		fprintf(stderr, "Connection failed: %s\n",
			xmmsc_get_last_error(connection));
		exit (EXIT_FAILURE);
	}

	if (!xmmsc_connect (connection, getenv("XMMS_PATH"))) {
		fprintf(stderr, "Connection failed: %s\n",
			xmmsc_get_last_error(connection));
		exit (EXIT_FAILURE);
	}

	
	/* Get the playlist */
	result = xmmsc_playlist_list_entries(connection, xplaylist_args.playlist_name);
	xmmsc_result_wait(result);
	return_value = xmmsc_result_get_value(result);

	if (xmmsv_get_error(return_value, &err_buf)){
		fprintf(stderr, "Couldn't get the playlist. Are you that playlist \"%s\" exists?\nError: %s\n",
			xplaylist_args.playlist_name, err_buf);
		exit(EXIT_FAILURE);
	}

	if (!xmmsv_get_list_iter (return_value, &iterator)){
		fprintf(stderr, "Couldn't get the list of elements in the playlist.\n");
		exit(EXIT_FAILURE);
	}

	/* For every element in the playlist, add it to the list, and
	   keep a count of how many items we have */
	int playlist_size = 0;

	char* media_result;
	for (; xmmsv_list_iter_valid (iterator); xmmsv_list_iter_next(iterator)){
		/* Get the id of the entry */
		int entry_id;
		xmmsv_t *list_entry;

		xmmsv_list_iter_entry(iterator, &list_entry);
		xmmsv_get_int(list_entry, &entry_id);


		if (head_node == NULL){
			/* Start our list */
			head_node = malloc(sizeof(struct medialib_info_node));
			current_node = head_node;

			media_result = get_media_string(connection, entry_id, "url");
			current_node->url = malloc(sizeof(char) * strlen(media_result) + 1);
			strcpy(current_node->url, media_result);

			media_result = get_media_string(connection, entry_id, "title");
			current_node->title = malloc(sizeof(char) * strlen(media_result) + 1);
			strcpy(current_node->title, media_result);

			media_result = get_media_string(connection, entry_id, "artist");
			current_node->artist = malloc(strlen(media_result) + 1);
			strcpy(current_node->artist, media_result);

			current_node->length = get_media_int(connection, entry_id, "duration");
			current_node->length = current_node->length / 1000;

			current_node->next = NULL;
			playlist_size++;
		} else {
			current_node->next = malloc(sizeof(struct medialib_info_node));
			current_node = current_node->next;

			media_result = get_media_string(connection, entry_id, "url");
			current_node->url = malloc(strlen(media_result) + 1);
			strcpy(current_node->url, media_result);

			media_result = get_media_string(connection, entry_id, "title");
			current_node->title = malloc(strlen(media_result) + 1);
			strcpy(current_node->title, media_result);

			media_result = get_media_string(connection, entry_id, "artist");
			current_node->artist = malloc(strlen(media_result) + 1);
			strcpy(current_node->artist, media_result);

			current_node->length = get_media_int(connection, entry_id, "duration");
			current_node->length = current_node->length / 1000;		

			current_node->next = NULL;
			playlist_size++;
		}
		
	}
	bool use_pls = !strcmp(xplaylist_args.output_type, "pls");
	
	current_node = head_node;
	struct medialib_info_node *temp_node;
	int i = 0;
	for (i; i < playlist_size; i++){

		if (current_node == head_node){
			if (use_pls)
				start_playlist_pls(playlist_size);
			else
				start_playlist_m3u();
		}
		if (use_pls)
			add_playlist_pls(current_node->length, current_node->title, current_node->url);
		else
			add_playlist_m3u(current_node->length, current_node->title, current_node->url, current_node->artist);


		temp_node = current_node;
		current_node = current_node->next;
		free(temp_node->url);
		free(temp_node->title);
		free(temp_node->artist);
		free(temp_node);
	}

	if (use_pls)
		end_playlist_pls();
	else
		end_playlist_m3u();

	xmmsc_result_unref(result);
	return 0;
	//return EXIT_SUCCESS;
	
}

void usage(){
	fprintf(stdout, "Welcome to the XMMS2 playlist creator!\n");
	fprintf(stdout, "This program will output the content of an\n");
	fprintf(stdout, "XMMS2 playlist into something usable by other programs.\n");
	fprintf(stdout, "\nNot that you'd want to use other programs.");
	fprintf(stdout, "\nUsage:\n");
	fprintf(stdout, "\txplaylist_creator [-t <output type>] [-p <playlist_name>] <output_file_name>\n");
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t[-p, --playlist]: Playlist name inside XMMS2. Current playlist is used if not specified.\n");
	fprintf(stdout, "\t[-t, --type]: Type of playlist to output. Acceptable values are:\n");
	fprintf(stdout, "\t\tm3u, pls\n");
	fprintf(stdout, "\nAnd that should be it. If there are any issues, feel free to let the XMMS2 team know.\n");
}

char* get_media_string(xmmsc_connection_t *connection, int entry_id, const char* information)
{
	xmmsc_result_t *result;
	xmmsv_t *return_value;
	const char *err_buf;

	xmmsv_t *dict_entry;
	xmmsv_t *infos;
	const char* val;
	char *return_val;

	result = xmmsc_medialib_get_info(connection, entry_id);

	xmmsc_result_wait(result);
	return_value = xmmsc_result_get_value(result);

	if (xmmsv_get_error(return_value, &err_buf)) {
		fprintf(stderr, "Couldn't get %s information for song id %i\n", information, entry_id);
		fprintf(stderr, "Error: %s\n", err_buf);
	}

	infos = xmmsv_propdict_to_dict(return_value, NULL);
	if (!xmmsv_dict_get(infos, information, &dict_entry) ||
		!xmmsv_get_string(dict_entry, &val)) {
		val = "#INVALID ENTRY";
	}

	int string_length = strlen(val);
	return_val = malloc(string_length + 1);

	strncpy(return_val, val, string_length + 1);

	xmmsv_unref(infos);
	xmmsc_result_unref(result);

	/* free(return_val); */
	return return_val;
}

int get_media_int(xmmsc_connection_t* connection, int entry_id, const char* information)
{
	xmmsc_result_t *result;
	xmmsv_t *return_value;
	const char *err_buf;

	xmmsv_t *dict_entry;
	xmmsv_t *infos;
	int val;

	result = xmmsc_medialib_get_info(connection, entry_id);

	xmmsc_result_wait(result);
	return_value = xmmsc_result_get_value(result);

	if (xmmsv_get_error(return_value, &err_buf)) {
		fprintf(stderr, "Couldn't get %s information for song id %i\n", information, entry_id);
		fprintf(stderr, "Error: %s\n", err_buf);
	}

	infos = xmmsv_propdict_to_dict(return_value, NULL);
	if (!xmmsv_dict_get(infos, information, &dict_entry) ||
		!xmmsv_get_int(dict_entry, &val)) {
		val = -2; /* Note that -2 is used since a value of -1 represents a stream - indefinite length */
	}

	xmmsv_unref(infos);
	xmmsc_result_unref(result);

	return val;
}

bool start_playlist_m3u(){
	printf("#EXTM3U\n");
	return true;
}

bool add_playlist_m3u(int length, const char* title, const char *url, const char *artist){
	printf("#EXTINF: %i,%s - %s\n%s\n", length, artist, title, url);
	return true;
}

bool end_playlist_m3u(int playlist_size){
	return true;
}


bool start_playlist_pls(int size){
	printf("[playlist]\nNumberOfEntries=%i\n", size);
	return true;
}

bool add_playlist_pls(int length, const char* title, const char *url){
	static int entry_no;
	entry_no++;
	printf("File%i=%s\n", entry_no, url);
	printf("Title%i=%s\n", entry_no, title);
	printf("Length%i=%i\n", entry_no, length);
	return true;
}

bool end_playlist_pls(){
	printf("Version=2\n");
	return true;
}


