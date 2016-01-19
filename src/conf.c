/*
 * conf.c - parse config
 *
 * Copyright (C) 2014 - 2015, Xiaoxiao <i@xiaoxiao.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "conf.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#else
#  define PACKAGE "sans"
#  define PACKAGE_BUGREPORT "https://github.com/XiaoxiaoPu/sans/issues"
#  define VERSION "0.1.0"
#endif

#ifdef __MINGW32__
#  include "win.h"
#endif


#define LINE_MAX 1024


/*
 * @func help()
 * @desc print help message
 */
static void help(void)
{
    printf("usage: %s\n"
           "  -h, --help           show this help\n"
           "  -c, --config <file>  config file\n"
           "  -d, --daemon         daemonize after initialization\n"
           "  --pidfile <file>     PID file, default: /run/sans.pid\n"
           "  --logfile <file>     log file, default: /var/log/sans.log\n"
           "  -v, --verbose        verbose logging\n"
           "  -u, --nspresolver    use NoStandardPort UDP Resolver\n"
           "  -V, --version        print version and exit\n\n"
           "Bug report: <%s>.\n", PACKAGE, PACKAGE_BUGREPORT);
}


/*
 * @func my_strncpy()
 * @desc copy string
 */
#define my_strncpy(dest, src) _strncpy(dest, src, sizeof(dest))
static void _strncpy(char *dest, const char *src, size_t n)
{
    char *end = dest + n;
    while ((dest < end) && ((*dest = *src) != '\0'))
    {
        dest++;
        src++;
    }
    *(end - 1) = '\0';
}


/*
 * @func  read_conf()
 * @desc  read config file
 */
static int read_conf(const char *file, conf_t *conf)
{
    FILE *f = fopen(file, "rb");
    if (f == NULL)
    {
        fprintf(stderr, "failed to open config file\n");
        return -1;
    }

    int line_num = 0;
    char buf[LINE_MAX];

    while (!feof(f))
    {
        char *line = fgets(buf, LINE_MAX, f);
        if (line == NULL)
        {
            break;
        }
        line_num++;
        // 跳过行首空白符
        while (isspace(*line))
        {
            line++;
        }
        // 去除行尾的空白符
        char *end = line + strlen(line) - 1;
        while ((end >= line) && (isspace(*end)))
        {
            *end = '\0';
            end--;
        }
        // 跳过注释和空白行
        if ((*line == '#') || (*line == '\0'))
        {
            continue;
        }
        // 开始解析
        char *p = strchr(line, '=');
        if (p == NULL)
        {
            fprintf(stderr, "parse config file failed at line: %d\n", line_num);
            fclose(f);
            return -1;
        }
        *p = '\0';
        char *key = line;
        char *value = p + 1;
        if (strcmp(key, "user") == 0)
        {
            my_strncpy(conf->user, value);
        }
        else if (strcmp(key, "listen") == 0)
        {
            p = strrchr(value, ':');
            if (p == NULL)
            {
                fprintf(stderr, "parse config file failed at line: %d\n", line_num);
                fclose(f);
                return -1;
            }
            *p = '\0';
            my_strncpy(conf->listen.addr, value);
            my_strncpy(conf->listen.port, p + 1);
        }
        else if (strcmp(key, "test_server") == 0)
        {
            p = strrchr(value, ':');
            if (p == NULL)
            {
                fprintf(stderr, "parse config file failed at line: %d\n", line_num);
                fclose(f);
                return -1;
            }
            *p = '\0';
            my_strncpy(conf->test_server.addr, value);
            my_strncpy(conf->test_server.port, p + 1);
        }
        else if (strcmp(key, "cn_server") == 0)
        {
            p = strrchr(value, ':');
            if (p == NULL)
            {
                fprintf(stderr, "parse config file failed at line: %d\n", line_num);
                fclose(f);
                return -1;
            }
            *p = '\0';
            my_strncpy(conf->cn_server.addr, value);
            my_strncpy(conf->cn_server.port, p + 1);
        }
        else if (strcmp(key, "server") == 0)
        {
            p = strrchr(value, ':');
            if (p == NULL)
            {
                fprintf(stderr, "parse config file failed at line: %d\n", line_num);
                fclose(f);
                return -1;
            }
            *p = '\0';
            my_strncpy(conf->server.addr, value);
            my_strncpy(conf->server.port, p + 1);
        }
        else if (strcmp(key, "socks5") == 0)
        {
            p = strrchr(value, ':');
            if (p == NULL)
            {
                fprintf(stderr, "parse config file failed at line: %d\n", line_num);
                fclose(f);
                return -1;
            }
            *p = '\0';
            my_strncpy(conf->socks5.addr, value);
            my_strncpy(conf->socks5.port, p + 1);
        }
    }
    fclose(f);

    return 0;
}


/*
 * @func parse_args()
 * @desc parse command line parameters
 */
int parse_args(int argc, char **argv, conf_t *conf)
{
    const char *conf_file = NULL;

    bzero(conf, sizeof(conf_t));

    for (int i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
            help();
            return -1;
        }
        else if ((strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--config") == 0))
        {
            if (i + 2 > argc)
            {
                fprintf(stderr, "missing filename after '%s'\n", argv[i]);
                return 1;
            }
            conf_file = argv[i + 1];
            i++;
        }
        else if ((strcmp(argv[i], "-d") == 0) || (strcmp(argv[i], "--daemon") == 0))
        {
            conf->daemon = 1;
        }
        else if (strcmp(argv[i], "--pidfile") == 0)
        {
            if (i + 2 > argc)
            {
                fprintf(stderr, "missing filename after '%s'\n", argv[i]);
                return 1;
            }
            my_strncpy(conf->pidfile, argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "--logfile") == 0)
        {
            if (i + 2 > argc)
            {
                fprintf(stderr, "missing filename after '%s'\n", argv[i]);
                return 1;
            }
            my_strncpy(conf->logfile, argv[i + 1]);
            i++;
        }
        else if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--verbose") == 0))
        {
            conf->verbose = 1;
        }
        else if ((strcmp(argv[i], "-u") == 0) || (strcmp(argv[i], "--nspresolver") == 0))
        {
            conf->nspresolver = 1;
        }
        else if ((strcmp(argv[i], "-V") == 0) || (strcmp(argv[i], "--version") == 0))
        {
            printf("%s %s\n", PACKAGE, VERSION);
            return -1;
        }
        else
        {
            fprintf(stderr, "invalid option: %s\n", argv[i]);
            return -1;
        }
    }
    if (conf_file != NULL)
    {
        if (read_conf(conf_file, conf) != 0)
        {
            return -1;
        }
    }

    if (conf->pidfile[0] == '\0')
    {
        strcpy(conf->pidfile, "/run/sans.pid");
    }
    if (conf->logfile[0] == '\0')
    {
        strcpy(conf->logfile, "/var/log/sans.log");
    }
    if (conf->listen.addr[0] == '\0')
    {
        strcpy(conf->listen.addr, "127.0.0.1");
    }
    if (conf->listen.port[0] == '\0')
    {
        strcpy(conf->listen.port, "53");
    }
    if (conf->test_server.addr[0] == '\0')
    {
        strcpy(conf->test_server.addr, "8.8.8.8");
    }
    if (conf->test_server.port[0] == '\0')
    {
        strcpy(conf->test_server.port, "53");
    }
    if (conf->cn_server.addr[0] == '\0')
    {
        strcpy(conf->cn_server.addr, "114.114.114.114");
    }
    if (conf->cn_server.port[0] == '\0')
    {
        strcpy(conf->cn_server.port, "53");
    }
    if (conf->server.addr[0] == '\0')
    {
        strcpy(conf->server.addr, "8.8.4.4");
    }
    if (conf->server.port[0] == '\0')
    {
        strcpy(conf->server.port, "53");
    }
    return 0;
}
