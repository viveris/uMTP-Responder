int inotify_handler_init( mtp_ctx * ctx );
int inotify_handler_deinit( mtp_ctx * ctx );

int inotify_handler_addwatch( mtp_ctx * ctx, char * path );
int inotify_handler_rmwatch( mtp_ctx * ctx, int wd );
