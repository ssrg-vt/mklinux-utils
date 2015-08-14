
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

extern FILE* main_my_log;
ngx_int_t
ngx_daemon(ngx_log_t *log)
{
    int  fd;
    FILE* my_log;

    fprintf(main_my_log, "%s starting pid %d\n", __func__, getpid()); 
    switch (fork()) {
    case -1:
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "fork() failed");
        return NGX_ERROR;

    case 0:
	//my_log= fopen("/my_log.txt","w");
	main_my_log= fopen("/log.txt","a+");
	my_log= main_my_log;
	fprintf(my_log, "%s after fork %d child\n", __func__, getpid());
	fflush(my_log);
        break;

    default:
	my_log= main_my_log;
	fprintf(my_log, "%s after fork %d parent\n", __func__, getpid());
        exit(0);
    }

    fprintf(my_log, "%s after fork %d\n", __func__, getpid());

    ngx_pid = ngx_getpid();

    if (setsid() == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "setsid() failed");
        return NGX_ERROR;
    }

    umask(0);

    fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "open(\"/dev/null\") failed");
        return NGX_ERROR;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "dup2(STDIN) failed");
        return NGX_ERROR;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "dup2(STDOUT) failed");
        return NGX_ERROR;
    }

#if 0
    if (dup2(fd, STDERR_FILENO) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "dup2(STDERR) failed");
        return NGX_ERROR;
    }
#endif

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "close() failed");
            return NGX_ERROR;
        }
    }

    fprintf(my_log, "%s ending %d\n", __func__, getpid());
    fflush(my_log);
    return NGX_OK;
}
