/*  Copyright 2007 P.F. Chimento
 *  This file is part of GNOME Inform 7.
 * 
 *  GNOME Inform 7 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  GNOME Inform 7 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNOME Inform 7; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gnome.h>

#include "error.h"
#include "spawn.h"

#define BUFSIZE 1024

/*
 * The following three functions are thanks to Tim-Philipp Mueller's example
 * From http://scentric.net/tmp/spawn-async-with-pipes-gtk.c 
 */

/* Runs a command (in argv[0]) with working directory wd, and pipes the output
to a GtkTextBuffer */
GPid run_command(const gchar *wd, gchar **argv, GtkTextBuffer *output) {
    GError *err = NULL;
    GPid child_pid;
    gint stdout_fd, stderr_fd;
    
    if (!g_spawn_async_with_pipes(
      wd,           /* working directory */
      argv,         /* command and arguments */
      NULL,         /* do not change environment */
      (GSpawnFlags) G_SPAWN_SEARCH_PATH   /* look for command in $PATH  */
      | G_SPAWN_DO_NOT_REAP_CHILD,  /* we'll check the exit status ourself */
      NULL,         /* child setup function */
      NULL,         /* child setup func data argument */
      &child_pid,   /* where to store the child's PID */
      NULL,         /* default stdin = /dev/null */
      output? &stdout_fd : NULL,   /* where to put stdout file descriptor */
      output? &stderr_fd : NULL,   /* where to put stderr file descriptor */
      &err)) {
        error_dialog(NULL, err, "Could not spawn process: ");
        return (GPid)0;
    }
    
    /* Now use GIOChannels to monitor stdout and stderr */
    if(output != NULL) {
        set_up_io_channel(stdout_fd, output);
        set_up_io_channel(stderr_fd, output);
    }
    
    return child_pid;
}

/* Set up an IO channel from a file descriptor to a GtkTextBuffer */
void set_up_io_channel(gint fd, GtkTextBuffer *output) {
    GIOChannel *ioc = g_io_channel_unix_new(fd);
    g_io_channel_set_encoding(ioc, NULL, NULL); /* enc. NULL = binary data? */
    g_io_channel_set_buffered(ioc, FALSE);
    g_io_channel_set_close_on_unref(ioc, TRUE);
    g_io_add_watch(ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, 
      write_channel_to_buffer, (gpointer)output);
	g_io_channel_unref (ioc);
}

/* The callback for writing data from the IO channel to the buffer */
gboolean write_channel_to_buffer(GIOChannel *ioc, GIOCondition cond,
gpointer buffer) {
    /* data for us to read? */
    if(cond & (G_IO_IN | G_IO_PRI)) {
        GIOStatus result;
        gchar scratch[BUFSIZE];
        gsize chars_read = 0;
        
        memset(scratch, 0, BUFSIZE); /* clear the buffer */
        result = g_io_channel_read_chars(ioc, scratch, BUFSIZE, &chars_read,
          NULL);
        
        if (chars_read <= 0 || result != G_IO_STATUS_NORMAL)
            return FALSE;
        
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter((GtkTextBuffer *)buffer, &iter);
        gtk_text_buffer_insert((GtkTextBuffer *)buffer, &iter, scratch,
          chars_read);
    }
    
    if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
        return FALSE;
    return TRUE;
}